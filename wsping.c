#define WIN32_LEAN_AND_MEAN
#define WIN32_NO_STATUS
#include <Windows.h>
#include <WinSock2.h>
#include <WS2tcpip.h>
#include <iphlpapi.h>
#include <IcmpAPI.h>
#include <assert.h>

#include "wsping.h"

// Linker libraries for Visual Studio,
// add -lws2_32 and -liphlpapi linker flags
// for MinGW/MSYS2 user
#pragma comment(lib, "ws2_32.lib")
#pragma comment(lib, "iphlpapi.lib")

// WinApi stuffs
static WSADATA wsa_data;
static int wsa_status;
static HANDLE hicmp_file = INVALID_HANDLE_VALUE;
static int family = AF_UNSPEC;
static PADDRINFOW target = NULL;
static PCWSTR target_name = NULL;
static WCHAR address[46];
static WCHAR canon_name[NI_MAXHOST];
static IP_OPTION_INFORMATION ip_options;

// WSPing stuffs
static wsping_options_t options;
static wsping_errfunc_t err_cb;
static void* userdata;

// Ping statistics
static uint32_t rtt_max = 0;
static uint32_t rtt_min = 0;
static uint32_t rtt_total = 0;
static uint32_t echos_sent = 0;
static uint32_t echos_received = 0;
static uint32_t echos_successful = 0;
static int data_size = 0;
static int ttl = 0;
static uint32_t reply_time = 0;
static const char* status = "";

// Macro for set default value
#define wsping_defval(param, def) ((param) != 0) ? (param) : (def)

// A wise way to prevent Visual Studio's sprintf warning is using fixed
// buffer size, instead of defining _CRT_SECURE_NO_WARNINGS
#ifdef _MSC_VER
#define wsping_sprintf(dst, fmt, ...) sprintf_s(dst, WSPING_BUF_SIZE, fmt, __VA_ARGS__)
#else
#define wsping_sprintf(dst, fmt, ...) sprintf(dst, fmt, __VA_ARGS__)
#endif

enum 
{
	WSPING_BUF_SIZE = 1024,
	ICMP_ERROR_SIZE = 8,
	IO_STATUS_BLOCK = 8,
	DEFAULT_TIMEOUT = 1000,
	MAX_SEND_SIZE = 65500
};

// ASCII to Unicode converter
static wchar_t* utf8_to_utf16(const char* src)
{
	int num_bytes = MultiByteToWideChar(CP_UTF8, 0, src, -1, NULL, 0);
	wchar_t* dst = (wchar_t*)malloc(num_bytes * sizeof(wchar_t));
	if (!dst) {
		return NULL;
	}
	MultiByteToWideChar(CP_UTF8, 0, src, -1, dst, num_bytes);
	dst[num_bytes] = 0;
	return dst;
}

// Unicode to ASCII converter
static char* utf16_to_utf8(const wchar_t* src)
{
	int num_bytes = WideCharToMultiByte(CP_UTF8, 0, src, -1, NULL, 0, NULL, NULL);
	char* dst = (char*)malloc(num_bytes * sizeof(char));
	if (!dst) {
		return NULL;
	}
	WideCharToMultiByte(CP_UTF8, 0, src, -1, dst, num_bytes, NULL, NULL);
	dst[num_bytes] = 0;
	return dst;
}

// Get current target site info, target site ip addres,
// and IP family that we use
static bool resolve_target(const wchar_t* target_name)
{
	ADDRINFOW hints = {0};
	hints.ai_family = family;
	hints.ai_flags = AI_NUMERICHOST;
	char err[WSPING_BUF_SIZE] = {0};

	int status = GetAddrInfoW(target_name, NULL, &hints, &target);
	if (status != 0) {
		hints.ai_flags = AI_CANONNAME;
		status = GetAddrInfoW(target_name, NULL, &hints, &target);
		if (status != 0) {
			const char* tgt = utf16_to_utf8(target_name);
			wsping_sprintf(err, "Could not find host %s. Please check the name and try again", tgt);
			err_cb(userdata, err);
			return false;
		}
#ifdef _MSC_VER
		wcsncpy_s(canon_name, NI_MAXHOST, target->ai_canonname, wcslen(target->ai_canonname));
#else
		wcsncpy(canon_name, target->ai_canonname, wcslen(target->ai_canonname));
#endif
	} else if (options.resolve_address) {
		status = GetNameInfoW(target->ai_addr, target->ai_addrlen, canon_name, _countof(canon_name), NULL, 0, NI_NAMEREQD);
		if (status != 0) {
			wsping_sprintf(err, "GetNameInfo failed: %d", WSAGetLastError());
			err_cb(userdata, err);
			return false;
		}
	}

	family = target->ai_family;
	return true;
}

// Reset all stats
void wsping_reset()
{
	memset(address, 0, sizeof(address));
	memset(canon_name, 0, sizeof(canon_name));
	rtt_max = 0;
	rtt_min = 0;
	rtt_total = 0;
	echos_sent = 0;
	echos_received = 0;
	echos_successful = 0;
	data_size = 0;
	ttl = 0;
	reply_time = 0;
	status = "Ping Stopped";
}

// Get reply from target site
void wsping_refresh()
{
	LPVOID reply_buffer = NULL;
	LPVOID send_buffer = NULL;
	DWORD reply_size = 0;
	DWORD reply_status;
	char buffer[WSPING_BUF_SIZE] = {0};

	if (options.request_size != 0) {
		send_buffer = malloc(options.request_size);
		if (!send_buffer) {
			status = "Ping Error";
			err_cb(userdata, "Not enough resources available");
			return;
		}
		memset(send_buffer, 0, options.request_size);
	}

	if (family == AF_INET6) {
		reply_size += sizeof(ICMPV6_ECHO_REPLY);
	} else {
#ifdef _WIN64
		reply_size += sizeof(ICMP_ECHO_REPLY32);
#else
		reply_size += sizeof(ICMP_ECHO_REPLY);
#endif
	}

	reply_size += options.request_size + ICMP_ERROR_SIZE + IO_STATUS_BLOCK;
	reply_buffer = malloc(reply_size);
	if (!reply_buffer) {
		free(send_buffer);
		status = "Ping Error";
		err_cb(userdata, "Not enough resources available");
		return;
	}

	memset(reply_buffer, 0, reply_size);
	echos_sent++;

	if (family == AF_INET6) {
		struct sockaddr_in6 source = {0};
		source.sin6_family = AF_INET6;
		reply_status = Icmp6SendEcho2(
			hicmp_file,                            // IcmpHandle
			NULL,                                  // Event
			NULL,                                  // ApcRoutine
	    	NULL,                                  // ApcContext
			&source,                               // SourceAddress
			(struct sockaddr_in6*)target->ai_addr, // DestinationAddress
			send_buffer,                           // RequestData
			(USHORT)options.request_size,          // RequestSize
			&ip_options,                           // RequestOptions
			reply_buffer,                          // ReplyBuffer
			reply_size,                            // ReplySize
			options.timeout                        // Timeout
		);
	} else {
		reply_status = IcmpSendEcho2(
			hicmp_file,                                        // IcmpHandle
			NULL,                                              // Event
			NULL,                                              // ApcRoutine
			NULL,                                              // ApcContext
			((PSOCKADDR_IN)target->ai_addr)->sin_addr.s_addr,  // DestinationAddress
			send_buffer,                                       // RequestData
			(USHORT)options.request_size,                      // RequestSize
			&ip_options,                                       // RequestOptions
			reply_buffer,                                      // ReplyBuffer
			reply_size,                                        // ReplySize 
			options.timeout                                    // Timeout
		);
	}

	free(send_buffer);

	if (reply_status == 0) {
		reply_status = GetLastError();
		switch (reply_status) {
			case IP_REQ_TIMED_OUT:
				// RTO
				status = "Request timed out";
				return;
			default:
				// Unhandled error
				wsping_sprintf(buffer, "Transmit failed. (Code %u)", reply_status);
				status = buffer;
				return;
		}
	} else {
		SOCKADDR_IN6 sock_addr_in6 = {0};
		SOCKADDR_IN sock_addr_in = {0};
		PSOCKADDR sock_addr;
		socklen_t sz;

		echos_received++;

		if (family == AF_INET6) {   // IPv6
			PICMPV6_ECHO_REPLY p_echo_reply = (PICMPV6_ECHO_REPLY)reply_buffer;
			PIPV6_ADDRESS_EX ipv6_addr = (PIPV6_ADDRESS_EX)&p_echo_reply->Address;
			sock_addr_in6.sin6_family = AF_INET6;
			CopyMemory(sock_addr_in6.sin6_addr.u.Word, ipv6_addr->sin6_addr, sizeof(sock_addr_in6.sin6_addr));
			
			sock_addr = (PSOCKADDR)&sock_addr_in6;
			sz = sizeof(SOCKADDR_IN6);
			GetNameInfoW(
				sock_addr,      // pSockAddr
				sz,             // SockaddrLength
				address,        // pNodeBuffer
				NI_MAXHOST,     // NodeBufferSize
				NULL,           // pServiceBuffer
				0,              // ServiceBufferSize
				NI_NUMERICHOST  // Flags
			);

			switch (p_echo_reply->Status) {
				case IP_SUCCESS: {   
					// The target site was replied
					status = "OK";
					echos_successful++;
					if (p_echo_reply->RoundTripTime == 0) {
						reply_time = 1;
					} else {
						reply_time = p_echo_reply->RoundTripTime;
					}
					break;
				}
				case IP_DEST_NET_UNREACHABLE:  
					// Network unreachable
					status = "Destination network unreachable";
					break;
				case IP_DEST_HOST_UNREACHABLE: 
					// Network host uncreachable
					status =  "Destination host unreachable";
					break;
				case IP_TTL_EXPIRED_TRANSIT:   
					// TTL expired
					status = "TTL expired in transit";
					break;
				default:                       
					// Another reply
					wsping_sprintf(buffer, "Echo reply returned %lu", p_echo_reply->Status);
					status = buffer;
					break;
			}
		} else {  // IPv4
#ifdef _WIN64
			PICMP_ECHO_REPLY32 p_echo_reply = (PICMPV6_ECHO_REPLY32)reply_buffer;
#else
			PICMP_ECHO_REPLY p_echo_reply = (PICMP_ECHO_REPLY)reply_buffer;
#endif
			IPAddr* ip4_addr = &p_echo_reply->Address;
			sock_addr_in.sin_family = AF_INET;
			sock_addr_in.sin_addr.S_un.S_addr = *ip4_addr;
			
			sock_addr = (PSOCKADDR)&sock_addr_in;
			sz = sizeof(SOCKADDR_IN);
			GetNameInfoW(
				sock_addr,
				sz,
				address,
				NI_MAXHOST,
				NULL,
				0,
				NI_NUMERICHOST
			);

			switch (p_echo_reply->Status) {
				case IP_SUCCESS: {   
					// The target site was replied
					status = "OK";
					echos_successful++;
					data_size = p_echo_reply->DataSize;
					if (p_echo_reply->RoundTripTime == 0) {
						reply_time = 1;
					} else {
						reply_time = p_echo_reply->RoundTripTime;
					}
					ttl = p_echo_reply->Options.Ttl;
					if (p_echo_reply->RoundTripTime < rtt_min || rtt_min == 0) {
						rtt_min = p_echo_reply->RoundTripTime;
					}
					if (p_echo_reply->RoundTripTime > rtt_max || rtt_max == 0) {
						rtt_max = p_echo_reply->RoundTripTime;
					}
					rtt_total += p_echo_reply->RoundTripTime;
					break;
				}
				case IP_DEST_NET_UNREACHABLE:
					// Network unreachable
					status = "Destination network unreachable";
					break;
				case IP_DEST_HOST_UNREACHABLE:
					// Network host uncreachable
					status = "Destination host unreachable";
					break;
				case IP_TTL_EXPIRED_TRANSIT:
					// TTL expired
					status = "TTL expired in transit";
					break;
				default:
					// Another reply
					wsping_sprintf(buffer, "Echo reply returned %lu", p_echo_reply->Status);
					status = buffer;
					break;
			}
		}
	}
	
	free(reply_buffer);
}

bool wsping_init(wsping_errfunc_t err_func, void* udata)
{
	err_cb = err_func;
	userdata = udata;

	wsa_status = WSAStartup(MAKEWORD(2, 2), &wsa_data);
	if (wsa_status != 0) {
		char err[WSPING_BUF_SIZE] = {0};
		wsping_sprintf(err, "Failed to initialize WinSock: %i", wsa_status);
		err_cb(userdata, err);
		return false;
	}

	return true;
}

void wsping_shutdown()
{
	if (hicmp_file) {
		IcmpCloseHandle(hicmp_file);
	}
	if (target) {
		FreeAddrInfoW(target);
	}
	if (wsa_status == 0) {
		WSACleanup();
	}
}

bool wsping_start(const wsping_options_t* opt)
{
	assert(opt);
	options = *opt;
	options.timeout = wsping_defval(options.timeout, 4000);
	options.request_size = wsping_defval(options.request_size, 32);
	options.resolve_address = wsping_defval(options.resolve_address, false);
	options.ttl = wsping_defval(options.ttl, 128);

	if (strlen(options.target_site) == 0) {
		err_cb(userdata,"Target address must be specified");
		return false;
	}

	ip_options.Ttl = options.ttl;
	if (options.ip_version == wsping_ipv4) {
		family = AF_INET;
	} else if (options.ip_version == wsping_ipv6) {
		family = AF_INET6;
	}

	if (!resolve_target(utf8_to_utf16(options.target_site))) {
		return false;
	}

	DWORD addrlen = 46;
	char error[64] = {0};

	if (WSAAddressToStringW(target->ai_addr, (DWORD)target->ai_addrlen, NULL, address, &addrlen)) {
		wsping_sprintf(error, "WSAAddressToString failed: %d", WSAGetLastError());
		err_cb(userdata, error);
		return false;
	}

	if (family == AF_INET6) {
		hicmp_file = Icmp6CreateFile();
	} else {
		hicmp_file = IcmpCreateFile();
	}

	if (hicmp_file == INVALID_HANDLE_VALUE) {
		wsping_sprintf(error, "IcmpCreateFile failed: %lu", GetLastError());
		err_cb(userdata, error);
		return false;
	}

	status = "Ping Started";

	return true;
}

/*-------------------*
 | Ping Stats Getter |
 *-------------------*/

const char* wsping_get_status() 
{	
	return status;
}

const char* wsping_get_target_ip_address()
{
	return utf16_to_utf8(address);
}

const char* wsping_get_target_canonical_name()
{
	return utf16_to_utf8(canon_name);
}

int wsping_get_data_size()
{
	return data_size;
}

int wsping_get_ttl()
{
	return ttl;
}

uint32_t wsping_get_reply_time()
{
	return reply_time;
}

uint32_t wsping_get_rtt_min()
{
	return rtt_min;
}
uint32_t wsping_get_rtt_max()
{
	return rtt_max;
}

uint32_t wsping_get_rtt_total()
{
	return rtt_total;
}

uint32_t wsping_get_data_sent()
{
	return echos_sent;
}

uint32_t wsping_get_data_received()
{
	return echos_received;
}

uint32_t wsping_get_data_successful()
{
	return echos_successful;
}
