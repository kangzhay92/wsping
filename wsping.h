#pragma once

/*********************************************************
 * Simple pinging library for Windows using WinSock2 API *
 *********************************************************/

#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef enum _wsping_ip_version
{
	wsping_ipv4,
	wsping_ipv6
}
wsping_ip_version_t;

// Error output function callback
typedef void (*wsping_errfunc_t)(void*, const char*);

typedef struct _wsping_options
{
	uint32_t timeout;
	uint32_t request_size;
	uint8_t ttl;
	bool resolve_address;
	const char* target_site;
	wsping_ip_version_t ip_version;
} 
wsping_options_t;

// Ping initialization / destruction
bool wsping_init(wsping_errfunc_t err_func, void* udata);
void wsping_shutdown();

// Ping operations
bool wsping_start(const wsping_options_t* opt);
void wsping_reset();
void wsping_refresh();

// Stats getter
const char* wsping_get_status();
const char* wsping_get_target_ip_address();
const char* wsping_get_target_canonical_name();
int wsping_get_data_size();
int wsping_get_ttl();
uint32_t wsping_get_reply_time();
uint32_t wsping_get_rtt_min();
uint32_t wsping_get_rtt_max();
uint32_t wsping_get_rtt_total();
uint32_t wsping_get_data_sent();
uint32_t wsping_get_data_received();
uint32_t wsping_get_data_successful();

#ifdef __cplusplus
}
#endif
