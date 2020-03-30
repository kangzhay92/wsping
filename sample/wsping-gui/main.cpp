/**
 * WSPing GUI...
 *
 * Example usage of wsping with ImGui and Viper Framework.
 */

// IMPORTANT NOTES: - Add VP_DECL and VP_DLL to compiler defines
//                    if you using dynamic library version of 
//                    viper framework.
//
//                  - Add -ld3d11, -ldxgi, and -ldxguid linker flags
//                    for MinGW/MSYS2 users

// Uncomment this if you want to disable console in windows
#define VP_GUI

// Viper includes
#include <viper/app.h>
#include <viper/gfx.h>
#include <viper/time.h>
#include <viper/main.h>

// Venom includes
#include <imgui/imgui.h>
#include <venom/imgui.h>

#ifndef _MSC_VER
#include <iostream>
#endif

#include "wsping.h"

#ifdef _WIN32
static constexpr int VP_APP_ICON = 101;   // Our application icon id from .rc file
#endif

// Main Application State
class AppState
{
public:
	AppState() {}
	~AppState() {}

	vp_app* create_app(const vapp_prop* prop);
	void init();
	void event(const vapp_event* e);
	void update();
	void render();
	
	vp_app*   app()   { return _app; }
	vp_gfx*   gfx()   { return _gfx; }
	vn_imgui* imgui() { return _imgui; }

private:
	static void wsping_error(void* udata, const char* msg);

	void make_gui();
	void create_help_marker(const char* desc);
	void reset_stats();

	// Viper common objects
	vp_app* _app = nullptr;
	vp_gfx* _gfx = nullptr;
	vp_time* _tm = nullptr;
	vn_imgui* _imgui = nullptr;
	vgfx_pass_action _pass = {};
	uint64_t _last_time = 0;

	// Ping options
	bool _resolve_address = false;
	int _timeout = 2000;
	int _request_size = 32;
	int _ttl = 128;
	bool _ping_started = false;
	char _target_site[256] = "";

	// Ping stats
	// Common info
	const char* status = "Ping Stopped";
	const char* site = "";
	const char* ip = "";
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

	const char* errormsg = "";

	// Variable to ensure we refresh ping every 1 second
	bool refresh_now = false;
	uint64_t last_refresh_time = 0;
};

static AppState state;

// Platform specific stuffs
#if VP_APP_D3D11_BACKEND
static const void* d3d11_get_rtv()
{
	return state.app()->d3d11_get_render_target_view();
}

static const void* d3d11_get_dsv()
{
	return state.app()->d3d11_get_depth_stencil_view();
}
#endif

vp_app* AppState::create_app(const vapp_prop* prop)
{
	_app = vp_app::create(prop);
	return _app;
}

void AppState::wsping_error(void* obj, const char* msg)
{
	AppState* st = (AppState*)obj;
	st->errormsg = msg;
}

void AppState::init()
{
	// Initialize wsping
	wsping_init(wsping_error, this);

	// Enable copy and paste,
	// press Ctrl+V in application to
	// paste text from external sources.
	_app->enable_clipboard();

	// ViperGFX initialization
	vgfx_prop gprop = {};
#if VP_APP_D3D11_BACKEND
	gprop.d3d11_device = _app->d3d11_get_device();
	gprop.d3d11_device_context = _app->d3d11_get_device_context();
	gprop.d3d11_render_target_view_cb = d3d11_get_rtv;
	gprop.d3d11_depth_stencil_view_cb = d3d11_get_dsv;
#endif
	_gfx = vp_gfx::create(&gprop, _app);

	// ViperTime initialization
	_tm = vp_time::create();

	// Venom ImGui initialization
	vimgui_prop iprop = {};
	_imgui = vn_imgui::create(&iprop, _app, _gfx);

	// ViperGFX pass action initialization.
	// This is for set the app's clear color
	_pass.colors[0].action = vgfx_action_clear;
	_pass.colors[0].val[0] = 0.5f;   // Red
	_pass.colors[0].val[1] = 0.25f;  // Green
	_pass.colors[0].val[2] = 0.75f;  // Blue
	_pass.colors[0].val[3] = 1.0f;   // Alpha
}

void AppState::event(const vapp_event* e)
{
	if (e->type == vapp_event_key_up) {
		if (e->key_code == vapp_key_escape) {
			// If Escape key was pressed, close the application
			_app->quit();
		} else if (e->modifiers == vapp_kmod_ctrl && e->key_code == vapp_key_c) {
			// Like original ping application Ctrl+C = stop pinging
			_ping_started = false;
		}
	}

	// Passing Viper events to Venom ImGUI
	_imgui->handle_event(e);
}

void AppState::update()
{
	// Update ImGUI frame
	const int width = _app->get_width();
	const int height = _app->get_height();
	const double dt = _tm->seconds(_tm->delta_time(&_last_time));
	_imgui->new_frame(width, height, dt);

	// Reload wsping stats
	if (_ping_started && refresh_now) {
		wsping_refresh();
		refresh_now = false;
	}

	// Ensure to reload wsping every 1 second
	if (_tm->seconds(_tm->now() - last_refresh_time) >= 1.0) {
		last_refresh_time = _tm->now();
		refresh_now = true;
	}

	// Create the GUI
	make_gui();
}

void AppState::render()
{
	// Render the screen
	_gfx->begin_default_pass(&_pass, _app->get_width(), _app->get_height());
	_imgui->render();
	_gfx->end_pass();
	_gfx->commit();
}

void AppState::reset_stats()
{
	// Reset application and wsping stats
	status = "Ping Stopped";
	site = "";
	ip = "";
	data_size = 0;
	ttl = 0;
	reply_time = 0;
	sent = 0;
	received = 0;
	lost = 0;
	percent_lost = 0;
	rt_min = 0;
	rt_max = 0;
	rt_avg = 0;
	wsping_reset();
}

void AppState::create_help_marker(const char* desc)
{
	// Add tooltip widget into application,
	// this lines of code is from ImGui official demo
	ImGui::TextDisabled("(?)");
	if (ImGui::IsItemHovered()) {
		ImGui::BeginTooltip();
		ImGui::PushTextWrapPos(ImGui::GetFontSize() * 35.0f);
		ImGui::TextUnformatted(desc);
		ImGui::PopTextWrapPos();
		ImGui::EndTooltip();
	}
}

void AppState::make_gui()
{
	// Set ImGui window's size same as with the our application window's size
	ImGui::SetNextWindowSize({420, 180}, ImGuiCond_FirstUseEver);
	ImGui::SetNextWindowPos({10, 10}, ImGuiCond_FirstUseEver);
	int winflags = ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoMove;

	// Build main window
	if (ImGui::Begin("Ping Options", nullptr, winflags)) {
		static bool resolve_address = false;
		if (ImGui::Checkbox("Resolve addresses", &resolve_address)) { _resolve_address = resolve_address; }
		
		ImGui::SliderInt("Timeout (ms)", &_timeout, 1000, 4000); 
		ImGui::SameLine();
		create_help_marker("To prevent GUI's 'Request Timed Out' lagging\nbug, set timeout value to 1000-2000 ms");
		
		ImGui::SliderInt("Send buffer size", &_request_size, 0, 65500);
		ImGui::SliderInt("TTL", &_ttl, 1, UCHAR_MAX);
		ImGui::Separator();
		ImGui::InputText("Target Site", _target_site, IM_ARRAYSIZE(_target_site));
		ImGui::Separator();
		if (!_ping_started) {
			if (ImGui::Button("Start Pinging")) {
				// Reset the stats first
				reset_stats();

				// Start the wsping
				wsping_options_t opts = {};
				opts.target_site = _target_site;
				opts.resolve_address = _resolve_address;
				opts.timeout = _timeout;
				opts.request_size = _request_size;
				opts.ip_version = wsping_ipv4;
				opts.ttl = _ttl;
				_ping_started = wsping_start(&opts);

				// Set starting refresh time
				last_refresh_time = _tm->now();
				refresh_now = true;
			}
		} else {
			if (ImGui::Button("Stop Pinging")) {
				_ping_started = false;
			}
		}
	}
	ImGui::End();

	ImGui::SetNextWindowSize({420, 250}, ImGuiCond_FirstUseEver);
	ImGui::SetNextWindowPos({10, 200}, ImGuiCond_FirstUseEver);

	// Build statistics window
	if (ImGui::Begin("Statistics", nullptr, winflags)) {
		// Get ping datas from wsping
		if (_ping_started) {
			status = wsping_get_status();
			site = wsping_get_target_canonical_name();
			ip = wsping_get_target_ip_address();
			data_size = wsping_get_data_size();
			ttl = wsping_get_ttl();
			reply_time = wsping_get_reply_time();
			sent = wsping_get_data_sent();
			received = wsping_get_data_received();
			lost = sent - received;
			percent_lost = (ULONG)((lost / (double)sent) * 100.0);
			rt_min = wsping_get_rtt_min();
			rt_max = wsping_get_rtt_max();
			if (wsping_get_data_successful() != 0) { 
				// To prevent divide by zero error
				rt_avg = wsping_get_rtt_total() / wsping_get_data_successful();
			}
		}

		// Then update the GUI
		ImGui::Text("Status: %s", status);
		ImGui::Text("Target site: %s", site);
		ImGui::Text("Target IP address: %s", ip);
		ImGui::Text("Data size: %d", data_size);
		ImGui::Text("Time to live: %d", ttl);
		ImGui::Text("Reply time: %lums", reply_time);
		ImGui::Separator();
		ImGui::Text("Packets sent: %d", sent);
		ImGui::Text("Packets received: %d", received);
		ImGui::Text("Packets lost: %d (%d%% loss)", lost, percent_lost);
		ImGui::Separator();
		ImGui::Text("Round trip time minimum: %lums", rt_min);
		ImGui::Text("Round trip time maximum: %lums", rt_max);
		ImGui::Text("Average round trip time: %lums", rt_avg);
	}
	ImGui::End();

	// Build error dialog box
	if (ImGui::BeginPopupModal("Error", nullptr, winflags)) {
		ImGui::Text(errormsg);
		ImGui::Separator();
		if (ImGui::Button("OK", { 120, 0 })) {
			errormsg = "";
			ImGui::CloseCurrentPopup();
		}
		ImGui::EndPopup();
	}
	if (strlen(errormsg) != 0) {
		ImGui::OpenPopup("Error");
	}
}

#pragma region Viper Framework Callback Functions
// Viper init function callback
// Initialize your application in this function
void init()
{
	state.init();
}

// Viper frame function callback
// Handle screen rendering here
void frame()
{
	state.update();
	state.render();
}

// Viper event function callback
// Handle the application events here, like key press or mouse click
void do_input(const vapp_event* e)
{
	state.event(e);
}

// Viper cleanup function callback
// Uninitialize your application here
//
// NOTE: Viper framework using singleton instance for initialization,
//       it will auto-deallocate himself. Don't use C++ delete, C free
//       function or Smart Pointer for deallocate Viper and Venom objects, or your
//       application will crash.
void cleanup()
{
	// Shutdown wsping here instead of on state's destructor
	wsping_shutdown();

	// Then destruct ImGui and ViperGFX
	state.imgui()->shutdown();
	state.gfx()->shutdown();
}

// Viper fail function callback
// You can use standard output here
void app_error(const char* msg)
{
	// Special for Visual C++ users, you can output the errors to 
	// Visual Studio console
#if (defined(_MSC_VER) && defined(_DEBUG))
	OutputDebugString(msg);
	OutputDebugString("\n");
#else
	// Output to standard output elsewhere
	std::cerr << msg << std::endl;
#endif
}

#ifdef _DEBUG
// Viper log function callback
// You can use this function for debugging purpose
void app_log(const char* msg)
{
	// Special for Visual C++ users, you can output the errors to 
	// Visual Studio console
#if (defined(_MSC_VER) && defined(_DEBUG))
	OutputDebugString(msg);
	OutputDebugString("\n");
#else
	// Output to standard output elsewhere
	std::cout << msg << std::endl;
#endif
}
#endif
#pragma endregion

// Viper application entry point.
// Use this main function if you include <viper/main.h> header,
// don't use standard main function, or this file won't compile
// because of redefinition behavior.
vp_app* vp_main(int argc, char** argv)
{
#ifdef _WIN32
	// Load icon from RC file
	HICON hIcon = LoadIcon(GetModuleHandle(0), MAKEINTRESOURCE(VP_APP_ICON));
#endif

	// Viper application properties
	vapp_prop prop = {};
	prop.width = 440;                   // Application width
	prop.height = 460;                  // Application height
	prop.title = "WSPing GUI";            // Application title
#ifdef _WIN32
	prop.icon = hIcon;                  // Application icon
#endif
	prop.auto_center = true;            // Set window position to center screen when started

	// Application callback functions
	prop.init_cb = init;
	prop.frame_cb = frame;
	prop.cleanup_cb = cleanup;
	prop.event_cb = do_input;
	prop.fail_cb = app_error;
#ifdef _DEBUG
	prop.log_cb = app_log;
#endif

	// Then initialize the application
	state = AppState{};
	return state.create_app(&prop);
}
