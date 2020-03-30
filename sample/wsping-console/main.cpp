/**
 * Interactive Ping Console...
 *
 * Example basic usage of wsping with STD C++ library. 
 */

#define WIN32_LEAN_AND_MEAN
#include <Windows.h>

#include <iostream>
#include <string>
#include <sstream>

#include "wsping.h"

// Main Application State
class AppState
{
public:
	AppState();
	~AppState();

	int run();
	static AppState instance;

private:
	static BOOL WINAPI ConsoleCtrlHandler(DWORD ctrltype);
	static void wsping_error(void* udata, const char* msg);

	void init();
	void start_pinging();
	void stop_pinging();
	void print_stats();
	void ping_once();

	// Ping options
	int _timeout = 4000;
	int _request_size = 32;
	int _ttl = 128;
	bool _ping_started = false;
	std::string _target_site;

	// Ping stats
	// Common info
	std::string status;
	std::string site;
	std::string ip;
	int data_size = 0;
	int ttl = 0;
	int reply_time = 0;
	// Packets
	uint32_t sent = 0;
	uint32_t received = 0;
	uint32_t lost = 0;
	uint32_t percent_lost = 0;
	// Round trip time
	uint32_t rt_min = 0;
	uint32_t rt_max = 0;
	uint32_t rt_avg = 0;

	// Native handle for Stdout console, its recommended
	// for using static variable for all Winapi stuff if we use C++
	// to prevent runtime errors.
	static HANDLE hStdout;

	// Some colorful text value for console output
	static constexpr WORD GREEN_TEXT  = FOREGROUND_GREEN | FOREGROUND_INTENSITY;
	static constexpr WORD RED_TEXT    = FOREGROUND_RED | FOREGROUND_INTENSITY;
	static constexpr WORD YELLOW_TEXT = FOREGROUND_RED | FOREGROUND_GREEN | FOREGROUND_INTENSITY;
	static constexpr WORD CYAN_TEXT   = FOREGROUND_GREEN | FOREGROUND_BLUE | FOREGROUND_INTENSITY;
	static constexpr WORD NORMAL_TEXT = FOREGROUND_RED | FOREGROUND_BLUE | FOREGROUND_GREEN | FOREGROUND_INTENSITY;
};

// Static initializers
AppState AppState::instance;
HANDLE   AppState::hStdout;

// WSPing error callback
void AppState::wsping_error(void* udata, const char* msg)
{
	std::stringstream buf;
	buf << "WSPing Error: " << msg << std::endl;
	throw std::runtime_error(buf.str());
}

// We can put WSPing initializer and deinitializer
// in class constructor and destructor too
AppState::AppState()
{
	instance = *this;

	if (!wsping_init(wsping_error, this)) {
		exit(1);
	}	
}

AppState::~AppState()
{
	wsping_shutdown();
}

// Macro for set default value from stdin, if the user
// skip input value.
#define std_cin_default_prompt(str, var, def, prompt) \
	std::cout << (prompt); \
	std::getline(std::cin, (str)); \
	if (!(str).empty()) { \
		std::istringstream s{(str)}; \
		s >> (var); \
	} else { \
		(var) = (def); \
	}

// Callback function for Ctrl key press
BOOL WINAPI AppState::ConsoleCtrlHandler(DWORD ctype)
{
	if (!instance._ping_started) {
		return FALSE;
	}

	switch (ctype) {
		case CTRL_C_EVENT:
		case CTRL_CLOSE_EVENT:
			// Ctrl+C = Stop pinging and print stats
			instance.stop_pinging();
			instance.print_stats();
			return FALSE;

		case CTRL_BREAK_EVENT:
			// Ctrl+Break = Print stats, but continue pinging the site
			instance.print_stats();
			return TRUE;
	}

	return FALSE;
}

// Main initialization function
void AppState::init()
{
	std::stringstream msgbuf;

	// Get the handle for stdout console
	hStdout = GetStdHandle(STD_OUTPUT_HANDLE);
	if (hStdout == INVALID_HANDLE_VALUE) {
		throw std::runtime_error("Console error");
	}

	// Set application title
	SetConsoleTitle("WSPing Interactive Console");

	// Set console's Ctrl key handler
	if (!SetConsoleCtrlHandler(AppState::ConsoleCtrlHandler, TRUE)) {
		msgbuf << "Failed to set Ctrl handler: " << GetLastError();
		throw std::runtime_error(msgbuf.str());
	}

	// Show application console
	SetConsoleTextAttribute(hStdout, CYAN_TEXT);
	std::cout << "*************************************" << std::endl;
	std::cout << "* WSPing Interactive Console v0.0.1 *" << std::endl;
	std::cout << "*************************************" << std::endl;
	std::cout << std::endl;

	SetConsoleTextAttribute(hStdout, NORMAL_TEXT);
	std::cout << "Hint: Press Ctrl+C to quit, Ctrl+Break to show statistics." << std::endl;
	std::cout << std::endl;
	std::cout << "Ping Configuration (Press enter to skip)" << std::endl;

	std::string input;

	// Parse the variables from stdin
	std_cin_default_prompt(input, _target_site,  "www.google.com", ">> Target site (default www.google.com): ");
	std_cin_default_prompt(input, _timeout,      4000,             ">> Timeout in milliseconds (default 4000): ");
	std_cin_default_prompt(input, _request_size, 32,               ">> Send buffer size (default 32): ");
	std_cin_default_prompt(input, _ttl,          128,              ">> TTL (default 128): ");

	// Start wsping library
	wsping_options_t opts = {};
	opts.target_site = _target_site.c_str();
	opts.resolve_address = false;
	opts.ip_version = wsping_ipv4;
	opts.timeout = _timeout;
	opts.request_size = _request_size;
	opts.ttl = _ttl;
	_ping_started = wsping_start(&opts);
}

// Send ping request to target site once
void AppState::ping_once()
{
	// Send a request to the target site
	wsping_refresh();

	// Update the stats
	status = wsping_get_status();
	data_size = wsping_get_data_size();
	reply_time = wsping_get_reply_time();
	ttl = wsping_get_ttl();
	sent = wsping_get_data_sent();
	received = wsping_get_data_received();
	lost = sent - received;
	percent_lost = (uint32_t)((lost / (double)sent) * 100.0);
	rt_min = wsping_get_rtt_min();
	rt_max = wsping_get_rtt_max();
	if (wsping_get_data_successful() > 0) {
		rt_avg = wsping_get_rtt_total() / wsping_get_data_successful();
	}
	
	if (status == "OK") {
		// Pinging the target site was successful, 
		// print the reply message in green text
		SetConsoleTextAttribute(hStdout, GREEN_TEXT);
		std::cout << "Reply from " << ip << ":";
		std::cout << " bytes=" << data_size;
		if (reply_time == 1) {
			std::cout << " time<1ms";
		} else {
			std::cout << " time=" << reply_time << "ms";
		}
		std::cout << " TTL=" << ttl << std::endl;
	} else { 
		// Something wrong happened, 
		// print status message in red text
		SetConsoleTextAttribute(hStdout, RED_TEXT);
		std::cout << status << "." << std::endl;
	}
}

// This function called after initialization was successful
void AppState::start_pinging()
{
	// Retrieve information about site's name and ip address
	site = wsping_get_target_canonical_name();
	ip   = wsping_get_target_ip_address();
	
	// Print the initial site info
	SetConsoleTextAttribute(hStdout, YELLOW_TEXT);
	if (!site.empty()) {
		std::cout << "\nPinging " << site << " [" << ip << "] ";
	} else {
		std::cout << "\nPinging " << ip << " ";
	}
	std::cout << "with " << _request_size << " bytes of data:" << std::endl << std::endl;

	// Loop until ping stopped
	while (true) {
		if (_ping_started) {
			// Pinging the site once
			ping_once();			
		} else {
			break;
		}

		// Ensure we only send a request once per second, 
		// we won't do a DDoS attack to the target site...
		// it's illegal XD
		Sleep(1000); 
	}

	print_stats();
}

// This function called when user press close button or Ctrl+C
void AppState::stop_pinging()
{
	_ping_started = false;
}

// Print ping detail results to stdout
void AppState::print_stats()
{
	// Change text to yellow
	SetConsoleTextAttribute(hStdout, YELLOW_TEXT);
	
	// Echos sent
	std::cout << std::endl;
	std::cout << "Ping statistics for " << ip << ":" << std::endl;
	std::cout << "\tPackets: Sent = " << sent 
		      << ", Received = " << received 
		      << ", Lost = " << lost << " (" << percent_lost << "% loss)" 
		      << std::endl;

	// Round trip time
	std::cout << "Approximate round-trip time in milliseconds:" << std::endl;
	std::cout << "\tMinimum = " << rt_min << "ms" 
		      << ", Maximum = " << rt_max << "ms"
		      << ", Average = " << rt_avg << "ms" << std::endl;

	// Then falling back to normal color if we close the application
	if (!_ping_started) {
		SetConsoleTextAttribute(hStdout, NORMAL_TEXT);
	}
}

// Main application loop
int AppState::run()
{
	// Initialize everything 
	init();
	
	// Start pinging the target
	if (_ping_started) {
		start_pinging();
	}

	return 0;
}

// Entry point
int main(int argc, char** argv)
{
	int ret = 0;

	// Run the application with exception handling
	try {
		ret = AppState::instance.run();
	} catch (std::exception& e) {
		std::cerr << e.what() << std::endl;
		ret = 1;
	}

	return ret; 
}
