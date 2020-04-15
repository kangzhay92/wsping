#include "app.h"

#ifdef _WIN32
#include <ShellScalingApi.h>
#endif

#if VP_APP_D3D11_BACKEND
#include <d3d11.h>
#endif
#include <cassert>

#if VP_APP_D3D11_BACKEND
#pragma comment(lib, "d3d11.lib")
#pragma comment(lib, "dxgi.lib")
#pragma comment(lib, "dxguid.lib")
#endif

struct vp_app_impl : public vp_app
{
	constexpr vp_app_impl() {}
	vp_app_impl(const vapp_prop* prop);
	~vp_app_impl() {}

	void shutdown() final;
	void run() final;

	void enable_debugging_log() final;
	void log(vapp_log_message_type type, const char* msg) final;
	void fail(const char* msg) final;

	void enable_clipboard();
	void set_clipboard_size(int size) final;
	void set_clipboard_string(const char* str) final;
	const char* get_clipboard_string();

#if VP_APP_D3D11_BACKEND
	const void* d3d11_get_device() final;
	const void* d3d11_get_device_context() final;
	const void* d3d11_get_render_target_view() final;
	const void* d3d11_get_depth_stencil_view()  final;
#endif

	int get_width() final;
	int get_height() final;
	float get_dpi_scale() final;

	void show_keyboard(bool shown) final;
	bool is_keyboard_shown() final;

	void quit() final;
	void consume_event() final;

	static vp_app_impl instance;
	vapp_prop prop{};

	float _window_scale = 0.0f;
	float _content_scale = 0.0f;
	float _mouse_scale = 0.0f;
	int _clipboard_size = 0;
	char* _clipboard = nullptr;
	bool _clipboard_enabled = false;

	vapp_key_code _keycodes[VAPP_MAX_KEYCODES] = {};
	HWND _hwnd = nullptr;
	HDC _hdc = nullptr;

	bool _first_frame = false;
	bool _init_called = false;
	bool _cleanup_called = false;
	bool _iconified = false;
	bool _user_cursor = false;

	float _dpi_scale = 0.0f;
	float _mouse_x = 0.0f, _mouse_y = 0.0f;
	bool _mouse_tracked = false;
	bool _onscreen_keyboard_shown = false;
	int _window_width = 0, _window_height = 0;
	int _framebuffer_width = 0, _framebuffer_height = 0;
	bool _quit_ordered = false;
	const char* _window_title = nullptr;
	bool _in_window_creating = false;
	vapp_event _evt = {};
	uint64_t _frame_count = 0;
	bool _event_consumed = false;

	bool _debugging_log_enabled = false;

	// event calls
	void mouse_event(vapp_event_type type, vapp_mouse_button btn);
	void scroll_event(float x, float y);
	void key_event(vapp_event_type type, int vk, bool repeat);
	void char_event(unsigned c, bool repeat);
	void init_event(vapp_event_type type);
	void app_event(vapp_event_type type);
	void log_event(vapp_log_message_type type, const char* msg);
	void fail_event(const char* msg);

private:
	void init_keytable();
	void init_dpi();
	bool update_dimensions();
	void create_window();

	// function callback calls
	void call_init();
	void call_frame();
	void call_cleanup();
	bool call_event(const vapp_event* e);

	bool events_enabled();
	void do_frame();
};

vp_app_impl vp_app_impl::instance;

#if VP_APP_D3D11_BACKEND
static ID3D11Device* _d3d11_device = nullptr;
static ID3D11DeviceContext* _d3d11_device_context = nullptr;
static DXGI_SWAP_CHAIN_DESC _dxgi_swap_chain_desc;
static IDXGISwapChain* _dxgi_swap_chain = nullptr;
static ID3D11Texture2D* _d3d11_rt = nullptr;
static ID3D11RenderTargetView* _d3d11_rtv = nullptr;
static ID3D11Texture2D* _d3d11_ds = nullptr;
static ID3D11DepthStencilView* _d3d11_dsv = nullptr;

static void d3d11_create_device_and_swapchain()
{
	DXGI_SWAP_CHAIN_DESC* sc_desc = &_dxgi_swap_chain_desc;
	sc_desc->BufferDesc.Width = vp_app_impl::instance._framebuffer_width;
	sc_desc->BufferDesc.Height = vp_app_impl::instance._framebuffer_height;
	sc_desc->BufferDesc.Format = DXGI_FORMAT_B8G8R8A8_UNORM;
	sc_desc->BufferDesc.RefreshRate.Numerator = 60;
	sc_desc->BufferDesc.RefreshRate.Denominator = 1;
	sc_desc->OutputWindow = vp_app_impl::instance._hwnd;
	sc_desc->Windowed = TRUE;
	sc_desc->SwapEffect = DXGI_SWAP_EFFECT_DISCARD;
	sc_desc->BufferCount = 1;
	sc_desc->SampleDesc.Count = vp_app_impl::instance.prop.sample_count;
	sc_desc->SampleDesc.Quality = vp_app_impl::instance.prop.sample_count > 1 ? D3D11_STANDARD_MULTISAMPLE_PATTERN : 0;
	sc_desc->BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
	int create_flags = D3D11_CREATE_DEVICE_SINGLETHREADED | D3D11_CREATE_DEVICE_BGRA_SUPPORT;
#ifdef _DEBUG
	create_flags |= D3D11_CREATE_DEVICE_DEBUG;
#endif
	D3D_DRIVER_TYPE dTypes[] = {
		D3D_DRIVER_TYPE_HARDWARE,
		D3D_DRIVER_TYPE_WARP,
		D3D_DRIVER_TYPE_REFERENCE
	};
	int numDriverTypes = sizeof(dTypes) / sizeof(D3D_DRIVER_TYPE);
	D3D_FEATURE_LEVEL fLevels[] = {
		D3D_FEATURE_LEVEL_11_1,
		D3D_FEATURE_LEVEL_11_0,
		D3D_FEATURE_LEVEL_10_1,
		D3D_FEATURE_LEVEL_10_0
	};
	D3D_FEATURE_LEVEL featureLevel = D3D_FEATURE_LEVEL_11_0;
	int numFeatureLevels = sizeof(fLevels) / sizeof(D3D_FEATURE_LEVEL);
	HRESULT hr;
	for (int i = 0; i < numDriverTypes; i++) {
		auto driverType = dTypes[i];
		hr = D3D11CreateDeviceAndSwapChain(
			NULL,
			driverType,
			NULL,
			create_flags,
			fLevels,
			numFeatureLevels,
			D3D11_SDK_VERSION,
			sc_desc,
			&_dxgi_swap_chain,
			&_d3d11_device,
			&featureLevel,
			&_d3d11_device_context);
		if (hr == E_INVALIDARG) {
			hr = D3D11CreateDeviceAndSwapChain(
				NULL,
				driverType,
				NULL,
				create_flags,
				&fLevels[1],
				numFeatureLevels - 1,
				D3D11_SDK_VERSION,
				sc_desc,
				&_dxgi_swap_chain,
				&_d3d11_device,
				&featureLevel,
				&_d3d11_device_context
			);
		}
		if (SUCCEEDED(hr)) {
			break;
		}
	}
	if (FAILED(hr) || !_dxgi_swap_chain || !_d3d11_device) {
		throw std::runtime_error("Failed to initialize D3D11 device");
	}
}

static void d3d11_create_default_render_target()
{
	HRESULT hr = _dxgi_swap_chain->GetBuffer(0, IID_ID3D11Texture2D, (void**)&_d3d11_rt);
	if (FAILED(hr)) {
		throw std::runtime_error("Failed to get D3D11 buffer");
	}

	hr = _d3d11_device->CreateRenderTargetView((ID3D11Resource*)_d3d11_rt, NULL, &_d3d11_rtv);
	if (FAILED(hr)) {
		throw std::runtime_error("Failed to create D3D11 render target view");
	}

	D3D11_TEXTURE2D_DESC ds_desc = {};
	ds_desc.Width = vp_app_impl::instance._framebuffer_width;
	ds_desc.Height = vp_app_impl::instance._framebuffer_height;
	ds_desc.MipLevels = 1;
	ds_desc.ArraySize = 1;
	ds_desc.Format = DXGI_FORMAT_D24_UNORM_S8_UINT;
	ds_desc.SampleDesc = _dxgi_swap_chain_desc.SampleDesc;
	ds_desc.Usage = D3D11_USAGE_DEFAULT;
	ds_desc.BindFlags = D3D11_BIND_DEPTH_STENCIL;
	hr = _d3d11_device->CreateTexture2D(&ds_desc, NULL, &_d3d11_ds);
	if (FAILED(hr)) {
		throw std::runtime_error("Failed to create D3D11 2D Texture");
	}

	D3D11_DEPTH_STENCIL_VIEW_DESC dsv_desc = {};
	dsv_desc.Format = ds_desc.Format;
	dsv_desc.ViewDimension = vp_app_impl::instance.prop.sample_count > 1 ? D3D11_DSV_DIMENSION_TEXTURE2DMS : D3D11_DSV_DIMENSION_TEXTURE2D;
	hr = _d3d11_device->CreateDepthStencilView((ID3D11Resource*)_d3d11_ds, &dsv_desc, &_d3d11_dsv);
	if (FAILED(hr)) {
		throw std::runtime_error("Failed to create D3D11 depth stencil view");
	}
}

static void d3d11_resize_default_render_target()
{
	if (_dxgi_swap_chain) {
		if (_d3d11_rt)  _d3d11_rt->Release();
		if (_d3d11_rtv) _d3d11_rtv->Release();
		if (_d3d11_ds)  _d3d11_ds->Release();
		if (_d3d11_dsv) _d3d11_dsv->Release();
		_dxgi_swap_chain->ResizeBuffers(1, 
			vp_app_impl::instance._framebuffer_width, 
			vp_app_impl::instance._framebuffer_height, 
			DXGI_FORMAT_B8G8R8A8_UNORM, 
			0);
		d3d11_create_default_render_target();
	}
}

const void* vp_app_impl::d3d11_get_device()
{
	return _d3d11_device;
}

const void* vp_app_impl::d3d11_get_device_context()
{
	return _d3d11_device_context;
}

const void* vp_app_impl::d3d11_get_render_target_view()
{
	return _d3d11_rtv;
}

const void* vp_app_impl::d3d11_get_depth_stencil_view()
{
	return _d3d11_dsv;
}
#endif

static bool utf8_to_utf16(const char* src, wchar_t* dst, int num_bytes)
{
	memset(dst, 0, num_bytes);
	const int dst_chars = num_bytes / sizeof(wchar_t);
	const int dst_needed = MultiByteToWideChar(CP_UTF8, 0, src, -1, NULL, 0);
	if ((dst_needed > 0) && (dst_needed < dst_chars)) {
		MultiByteToWideChar(CP_UTF8, 0, src, -1, dst, dst_chars);
		return true;
	}
	return false;
}

static bool utf16_to_utf8(const wchar_t* src, char* dst, int num_bytes)
{
	memset(dst, 0, num_bytes);
	return WideCharToMultiByte(CP_UTF8, 0, src, -1, dst, num_bytes, NULL, NULL) != 0;
}

void vp_app_impl::call_init()
{
	if (prop.init_cb) {
		prop.init_cb();
	} else if (prop.init_user_data_cb) {
		prop.init_user_data_cb(prop.user_data);
	}
	_init_called = true;
}

void vp_app_impl::call_frame()
{
	if (_init_called && !_cleanup_called) {
		if (prop.frame_cb) {
			prop.frame_cb();
		} else if (prop.frame_user_data_cb) {
			prop.frame_user_data_cb(prop.user_data);
		}
	}
}

void vp_app_impl::call_cleanup()
{
	if (!_cleanup_called) {
		if (prop.cleanup_cb) {
			prop.cleanup_cb();
		} else if (prop.cleanup_user_data_cb) {
			prop.cleanup_user_data_cb(prop.user_data);
		}
		_cleanup_called = true;
	}
}

bool vp_app_impl::call_event(const vapp_event* e)
{
	if (!_cleanup_called) {
		if (prop.event_cb) {
			prop.event_cb(e);
		} else if (prop.event_user_data_cb) {
			prop.event_user_data_cb(e, prop.user_data);
		}
	}
	if (_event_consumed) {
		_event_consumed = false;
		return true;
	}
	return false;
}

void vp_app_impl::log_event(vapp_log_message_type type, const char* msg)
{
	char outmsg[2048];
	const char* log_msg = "";

	switch (type) {
		case vapp_log_message_type_debug:
			if (!_debugging_log_enabled) {
				return;
			}
			log_msg = "Debug:";
			break;
		case vapp_log_message_type_warning:
			log_msg = "Warning:";
			break;
		case vapp_log_message_type_error:
			log_msg = "Error:";
			break;
	}
#ifdef _MSC_VER
	sprintf_s(outmsg, "%s %s\n", log_msg, msg);
#else
	sprintf(outmsg, "%s %s\n", log_msg, msg);
#endif
	if (prop.log_cb) {
		prop.log_cb(msg);
	} else if (prop.log_user_data_cb) {
		prop.log_user_data_cb(msg, prop.user_data);
	} else {
		std::cout << msg << std::endl;
	}
}

void vp_app_impl::fail_event(const char* msg)
{
	char outmsg[2048];
#ifdef _MSC_VER
	sprintf_s(outmsg, "Fatal: %s\n", msg);
#else
	sprintf(outmsg, "%s %s\n", log_msg, msg);
#endif
	if (prop.fail_cb) {
		prop.fail_cb(msg);
	} else if (prop.fail_user_data_cb) {
		prop.fail_user_data_cb(msg, prop.user_data);
	} else {
		std::cerr << msg << std::endl;
	}
}

void vp_app_impl::do_frame()
{
	if (_first_frame) {
		_first_frame = false;
		call_init();
	}
	call_frame();
	_frame_count++;
}

void vp_app_impl::init_keytable()
{
	_keycodes[0x00B] = vapp_key_0;
	_keycodes[0x002] = vapp_key_1;
	_keycodes[0x003] = vapp_key_2;
	_keycodes[0x004] = vapp_key_3;
	_keycodes[0x005] = vapp_key_4;
	_keycodes[0x006] = vapp_key_5;
	_keycodes[0x007] = vapp_key_6;
	_keycodes[0x008] = vapp_key_7;
	_keycodes[0x009] = vapp_key_8;
	_keycodes[0x00A] = vapp_key_9;
	_keycodes[0x01E] = vapp_key_a;
	_keycodes[0x030] = vapp_key_b;
	_keycodes[0x02E] = vapp_key_c;
	_keycodes[0x020] = vapp_key_d;
	_keycodes[0x012] = vapp_key_e;
	_keycodes[0x021] = vapp_key_f;
	_keycodes[0x022] = vapp_key_g;
	_keycodes[0x023] = vapp_key_h;
	_keycodes[0x017] = vapp_key_i;
	_keycodes[0x024] = vapp_key_j;
	_keycodes[0x025] = vapp_key_k;
	_keycodes[0x026] = vapp_key_l;
	_keycodes[0x032] = vapp_key_m;
	_keycodes[0x031] = vapp_key_n;
	_keycodes[0x018] = vapp_key_o;
	_keycodes[0x019] = vapp_key_p;
	_keycodes[0x010] = vapp_key_q;
	_keycodes[0x013] = vapp_key_r;
	_keycodes[0x01F] = vapp_key_s;
	_keycodes[0x014] = vapp_key_t;
	_keycodes[0x016] = vapp_key_u;
	_keycodes[0x02F] = vapp_key_v;
	_keycodes[0x011] = vapp_key_w;
	_keycodes[0x02D] = vapp_key_x;
	_keycodes[0x015] = vapp_key_y;
	_keycodes[0x02C] = vapp_key_z;
	_keycodes[0x028] = vapp_key_apostrophe;
	_keycodes[0x02B] = vapp_key_backslash;
	_keycodes[0x033] = vapp_key_comma;
	_keycodes[0x00D] = vapp_key_equal;
	_keycodes[0x029] = vapp_key_grave_accent;
	_keycodes[0x01A] = vapp_key_left_bracket;
	_keycodes[0x00C] = vapp_key_minus;
	_keycodes[0x034] = vapp_key_period;
	_keycodes[0x01B] = vapp_key_right_bracket;
	_keycodes[0x027] = vapp_key_semicolon;
	_keycodes[0x035] = vapp_key_slash;
	_keycodes[0x056] = vapp_key_world2;
	_keycodes[0x00E] = vapp_key_backspace;
	_keycodes[0x153] = vapp_key_delete;
	_keycodes[0x14F] = vapp_key_end;
	_keycodes[0x01C] = vapp_key_enter;
	_keycodes[0x001] = vapp_key_escape;
	_keycodes[0x147] = vapp_key_home;
	_keycodes[0x152] = vapp_key_insert;
	_keycodes[0x15D] = vapp_key_menu;
	_keycodes[0x151] = vapp_key_pagedown;
	_keycodes[0x149] = vapp_key_pageup;
	_keycodes[0x045] = vapp_key_pause;
	_keycodes[0x146] = vapp_key_pause;
	_keycodes[0x039] = vapp_key_space;
	_keycodes[0x00F] = vapp_key_tab;
	_keycodes[0x03A] = vapp_key_caps_lock;
	_keycodes[0x145] = vapp_key_num_lock;
	_keycodes[0x046] = vapp_key_scroll_lock;
	_keycodes[0x03B] = vapp_key_f1;
	_keycodes[0x03C] = vapp_key_f2;
	_keycodes[0x03D] = vapp_key_f3;
	_keycodes[0x03E] = vapp_key_f4;
	_keycodes[0x03F] = vapp_key_f5;
	_keycodes[0x040] = vapp_key_f6;
	_keycodes[0x041] = vapp_key_f7;
	_keycodes[0x042] = vapp_key_f8;
	_keycodes[0x043] = vapp_key_f9;
	_keycodes[0x044] = vapp_key_f10;
	_keycodes[0x057] = vapp_key_f11;
	_keycodes[0x058] = vapp_key_f12;
	_keycodes[0x064] = vapp_key_f13;
	_keycodes[0x065] = vapp_key_f14;
	_keycodes[0x066] = vapp_key_f15;
	_keycodes[0x067] = vapp_key_f16;
	_keycodes[0x068] = vapp_key_f17;
	_keycodes[0x069] = vapp_key_f18;
	_keycodes[0x06A] = vapp_key_f19;
	_keycodes[0x06B] = vapp_key_f20;
	_keycodes[0x06C] = vapp_key_f21;
	_keycodes[0x06D] = vapp_key_f22;
	_keycodes[0x06E] = vapp_key_f23;
	_keycodes[0x076] = vapp_key_f24;
	_keycodes[0x038] = vapp_key_left_alt;
	_keycodes[0x01D] = vapp_key_left_control;
	_keycodes[0x02A] = vapp_key_left_shift;
	_keycodes[0x15B] = vapp_key_left_super;
	_keycodes[0x137] = vapp_key_print_screen;
	_keycodes[0x138] = vapp_key_right_alt;
	_keycodes[0x11D] = vapp_key_right_control;
	_keycodes[0x036] = vapp_key_right_shift;
	_keycodes[0x15C] = vapp_key_right_super;
	_keycodes[0x150] = vapp_key_down;
	_keycodes[0x14B] = vapp_key_left;
	_keycodes[0x14D] = vapp_key_right;
	_keycodes[0x148] = vapp_key_up;
	_keycodes[0x052] = vapp_key_kp_0;
	_keycodes[0x04F] = vapp_key_kp_1;
	_keycodes[0x050] = vapp_key_kp_2;
	_keycodes[0x051] = vapp_key_kp_3;
	_keycodes[0x04B] = vapp_key_kp_4;
	_keycodes[0x04C] = vapp_key_kp_5;
	_keycodes[0x04D] = vapp_key_kp_6;
	_keycodes[0x047] = vapp_key_kp_7;
	_keycodes[0x048] = vapp_key_kp_8;
	_keycodes[0x049] = vapp_key_kp_9;
	_keycodes[0x04E] = vapp_key_kp_add;
	_keycodes[0x053] = vapp_key_kp_decimal;
	_keycodes[0x135] = vapp_key_kp_divide;
	_keycodes[0x11C] = vapp_key_kp_enter;
	_keycodes[0x037] = vapp_key_kp_multiply;
	_keycodes[0x04A] = vapp_key_kp_subtract;
}

void vp_app_impl::init_dpi()
{
	using set_process_dpi_aware_t = BOOL(WINAPI*)(void);
	using set_process_dpi_awareness_t = HRESULT(WINAPI*)(PROCESS_DPI_AWARENESS);
	using get_dpi_for_monitor_t = HRESULT(WINAPI*)(HMONITOR, MONITOR_DPI_TYPE, UINT*, UINT*);

	set_process_dpi_aware_t win32_setprocessdpiaware = nullptr;
	set_process_dpi_awareness_t win32_setprocessdpiawareness = nullptr;
	get_dpi_for_monitor_t win32_getdpiformonitor = nullptr;

	auto user32 = LoadLibrary("user32.dll");
	if (user32) {
		win32_setprocessdpiaware = (set_process_dpi_aware_t)GetProcAddress(user32, "SetProcessDPIAware");
	}

	auto shcore = LoadLibrary("shcore.dll");
	if (shcore) {
		win32_setprocessdpiawareness = (set_process_dpi_awareness_t)GetProcAddress(shcore, "SetProcessDpiAwareness");
		win32_getdpiformonitor = (get_dpi_for_monitor_t)GetProcAddress(shcore, "GetDpiForMonitor");
	}

	bool dpiAware = false;
	if (win32_setprocessdpiawareness) {
		PROCESS_DPI_AWARENESS pda = PROCESS_SYSTEM_DPI_AWARE;
		dpiAware = true;
		if (!prop.high_dpi) {
			pda = PROCESS_DPI_UNAWARE;
			dpiAware = false;
		}
		win32_setprocessdpiawareness(pda);
	} else if (win32_setprocessdpiaware) {
		win32_setprocessdpiaware();
		dpiAware = true;
	}

	if (win32_getdpiformonitor && dpiAware) {
		POINT pt = { 1, 1 };
		HMONITOR hm = MonitorFromPoint(pt, MONITOR_DEFAULTTONEAREST);
		UINT dpix, dpiy;
		win32_getdpiformonitor(hm, MDT_EFFECTIVE_DPI, &dpix, &dpiy);
		_window_scale = static_cast<float>(dpix) / 96.0f;
	} else {
		_window_scale = 1.0f;
	}

	if (instance.prop.high_dpi) {
		_content_scale = _window_scale;
		_mouse_scale = 1.0f;
	} else {
		_content_scale = 1.0f;
		_mouse_scale = 1.0f / _window_scale;
	}

	_dpi_scale = _content_scale;
	if (user32) {
		FreeLibrary(user32);
	}
	if (shcore) {
		FreeLibrary(shcore);
	}
}

bool vp_app_impl::update_dimensions()
{
	RECT rect;
	if (GetClientRect(_hwnd, &rect)) {
		_window_width = static_cast<int>((float)(rect.right - rect.left) / _window_scale);
		_window_height = static_cast<int>((float)(rect.bottom - rect.top) / _window_scale);
		const int fb_width = static_cast<int>((float)_window_width * _content_scale);
		const int fb_height = static_cast<int>((float)_window_height * _content_scale);
		if ((fb_width != _framebuffer_width) || (fb_height != _framebuffer_height)) {
			_framebuffer_width = static_cast<int>((float)_window_width * _content_scale);
			_framebuffer_height = static_cast<int>((float)_window_height * _content_scale);
			if (_framebuffer_width == 0) {
				_framebuffer_width = 1;
			}
			if (_framebuffer_height == 0) {
				_framebuffer_height = 1;
			}
			return true;
		}
	} else {
		_window_width = _window_height = 1;
		_framebuffer_width = _framebuffer_height = 1;
	}
	return false;
}

bool vp_app_impl::events_enabled()
{
	return (prop.event_cb || prop.event_user_data_cb) && _init_called;
}

void vp_app_impl::init_event(vapp_event_type type)
{
	_evt.type = type;
	_evt.frame_count = _frame_count;
	_evt.mouse_button = vapp_mb_invalid;
	_evt.window_width = _window_width;
	_evt.window_height = _window_height;
	_evt.framebuffer_width = _framebuffer_width;
	_evt.framebuffer_height = _framebuffer_height;
}

void vp_app_impl::app_event(vapp_event_type type)
{
	if (events_enabled()) {
		init_event(type);
		call_event(&_evt);
	}
}

static UINT key_mods()
{
	UINT mods = 0;
	if (GetKeyState(VK_SHIFT) & (1 << 15)) {
		mods |= vapp_kmod_shift;
	}
	if (GetKeyState(VK_CONTROL) & (1 << 15)) {
		mods |= vapp_kmod_ctrl;
	}
	if (GetKeyState(VK_MENU) & (1 << 15)) {
		mods |= vapp_kmod_alt;
	}
	if ((GetKeyState(VK_LWIN) | GetKeyState(VK_RWIN)) & (1 << 15)) {
		mods |= vapp_kmod_super;
	}
	return mods;
}

void vp_app_impl::mouse_event(vapp_event_type type, vapp_mouse_button btn)
{
	if (events_enabled()) {
		init_event(type);
		_evt.modifiers = key_mods();
		_evt.mouse_button = btn;
		_evt.mouse_x = _mouse_x;
		_evt.mouse_y = _mouse_y;
		call_event(&_evt);
	}
}

void vp_app_impl::scroll_event(float x, float y)
{
	if (events_enabled()) {
		init_event(vapp_event_mouse_scroll);
		_evt.modifiers = key_mods();
		_evt.scroll_x = -x / 30.f;
		_evt.scroll_y = y / 30.f;
		call_event(&_evt);
	}
}

void vp_app_impl::key_event(vapp_event_type type, int vk, bool repeat)
{
	if (events_enabled() && (vk < VAPP_MAX_KEYCODES)) {
		init_event(type);
		_evt.modifiers = key_mods();
		_evt.key_code = _keycodes[vk];
		_evt.key_repeat = repeat;
		call_event(&_evt);
		if (_clipboard_enabled && (type == vapp_event_key_down) && (_evt.modifiers == vapp_kmod_ctrl) && (_evt.key_code == vapp_key_v)) {
			init_event(vapp_event_clipboard_pasted);
			call_event(&_evt);
		}
	}
}

void vp_app_impl::char_event(unsigned c, bool repeat)
{
	if (events_enabled() && (c >= 32)) {
		init_event(vapp_event_char);
		_evt.modifiers = key_mods();
		_evt.char_code = c;
		_evt.key_repeat = repeat;
		call_event(&_evt);
	}
}

LRESULT CALLBACK win32_app_proc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
	if (!vp_app_impl::instance._in_window_creating) {
		switch (msg) {
		case WM_CLOSE:
			vp_app_impl::instance._quit_ordered = true;
			return 0;
		case WM_SYSCOMMAND:
			switch (wParam & 0xFFF0) {
			case SC_SCREENSAVE:
			case SC_MONITORPOWER:
				if (vp_app_impl::instance.prop.full_screen) {
					return 0;
				}
				break;
			case SC_KEYMENU:
				return 0;
			}
			break;
		case WM_ERASEBKGND:
			return 1;
		case WM_SIZE: {
			const bool iconified = (wParam == SIZE_MINIMIZED) ? true : false;
			if (iconified != vp_app_impl::instance._iconified) {
				vp_app_impl::instance._iconified = iconified;
				if (iconified) {
					vp_app_impl::instance.app_event(vapp_event_iconified);
				} else {
					vp_app_impl::instance.app_event(vapp_event_restored);
				}
			}
			break;
		}
		case WM_SETCURSOR:
			if (vp_app_impl::instance._user_cursor) {
				if (LOWORD(lParam) == HTCLIENT) {
					vp_app_impl::instance.app_event(vapp_event_update_cursor);
					return 1;
				}
			}
			break;
		case WM_LBUTTONDOWN:
			vp_app_impl::instance.mouse_event(vapp_event_mouse_down, vapp_mb_left);
			break;
		case WM_RBUTTONDOWN:
			vp_app_impl::instance.mouse_event(vapp_event_mouse_down, vapp_mb_right);
			break;
		case WM_MBUTTONDOWN:
			vp_app_impl::instance.mouse_event(vapp_event_mouse_down, vapp_mb_middle);
			break;
		case WM_LBUTTONUP:
			vp_app_impl::instance.mouse_event(vapp_event_mouse_up, vapp_mb_left);
			break;
		case WM_RBUTTONUP:
			vp_app_impl::instance.mouse_event(vapp_event_mouse_up, vapp_mb_right);
			break;
		case WM_MBUTTONUP:
			vp_app_impl::instance.mouse_event(vapp_event_mouse_up, vapp_mb_middle);
			break;
		case WM_MOUSEMOVE:
			vp_app_impl::instance._mouse_x = static_cast<float>(GET_X_LPARAM(lParam)) * vp_app_impl::instance._mouse_scale;
			vp_app_impl::instance._mouse_y = static_cast<float>(GET_Y_LPARAM(lParam)) * vp_app_impl::instance._mouse_scale;
			if (!vp_app_impl::instance._mouse_tracked) {
				vp_app_impl::instance._mouse_tracked = true;
				TRACKMOUSEEVENT tme = {};
				tme.cbSize = sizeof(TRACKMOUSEEVENT);
				tme.dwFlags = TME_LEAVE;
				tme.hwndTrack = hWnd;
				TrackMouseEvent(&tme);
				vp_app_impl::instance.mouse_event(vapp_event_mouse_enter, vapp_mb_invalid);
			}
			vp_app_impl::instance.mouse_event(vapp_event_mouse_move, vapp_mb_invalid);
			break;
		case WM_MOUSELEAVE:
			vp_app_impl::instance._mouse_tracked = false;
			vp_app_impl::instance.mouse_event(vapp_event_mouse_leave, vapp_mb_invalid);
			break;
		case WM_MOUSEWHEEL:
			vp_app_impl::instance.scroll_event(0.0f, static_cast<float>((SHORT)HIWORD(wParam)));
			break;
		case WM_MOUSEHWHEEL:
			vp_app_impl::instance.scroll_event(static_cast<float>((SHORT)HIWORD(wParam)), 0.0f);
			break;
		case WM_CHAR:
			vp_app_impl::instance.char_event(static_cast<unsigned>(wParam), !!(lParam & 0x40000000));
			break;
		case WM_KEYDOWN:
		case WM_SYSKEYDOWN:
			vp_app_impl::instance.key_event(vapp_event_key_down, static_cast<int>(HIWORD(lParam) & 0x1FF), !!(lParam & 0x40000000));
			break;
		case WM_KEYUP:
		case WM_SYSKEYUP:
			vp_app_impl::instance.key_event(vapp_event_key_up, static_cast<int>(HIWORD(lParam) & 0x1FF), false);
			break;
		default:
			break;
		}
	}

	return DefWindowProc(hWnd, msg, wParam, lParam);
}

void vp_app_impl::create_window()
{
	WNDCLASS cls = {};
	cls.style = CS_HREDRAW | CS_VREDRAW | CS_OWNDC;
	cls.lpfnWndProc = win32_app_proc;
	cls.hInstance = GetModuleHandle(0);
	cls.hCursor = LoadCursor(0, IDC_ARROW);
	cls.hIcon = prop.icon != NULL ? (HICON)prop.icon : LoadIcon(0, IDI_WINLOGO);
	cls.lpszClassName = "ViperApp";
	RegisterClass(&cls);

	DWORD style;
	const DWORD win_ex_style = WS_EX_APPWINDOW | WS_EX_WINDOWEDGE;
	RECT rect = {};
	if (prop.full_screen) {
		style = WS_POPUP | WS_SYSMENU | WS_VISIBLE;
		rect.right = GetSystemMetrics(SM_CXSCREEN);
		rect.bottom = GetSystemMetrics(SM_CYSCREEN);
	} else {
		if (prop.resizable) {
			style = WS_CLIPSIBLINGS | WS_CLIPCHILDREN | WS_CAPTION | WS_SYSMENU | WS_MINIMIZEBOX | WS_MAXIMIZEBOX | WS_SIZEBOX;
		} else {
			style = WS_CLIPSIBLINGS | WS_CLIPCHILDREN | WS_CAPTION | WS_SYSMENU | WS_MINIMIZEBOX;
		}
		rect.right = static_cast<int>((float)_window_width * _window_scale);
		rect.bottom = static_cast<int>((float)_window_height * _window_scale);
	}
	AdjustWindowRectEx(&rect, style, FALSE, win_ex_style);

	const int wWidth = rect.right - rect.left;
	const int wHeight = rect.bottom - rect.top;
	_in_window_creating = true;
	_hwnd = CreateWindowEx(
		win_ex_style,
		"ViperApp",
		_window_title,
		style,
		CW_USEDEFAULT,
		CW_USEDEFAULT,
		wWidth,
		wHeight,
		0, 0,
		GetModuleHandle(0),
		0
	);
	if (prop.auto_center) {
		RECT r;
		GetClientRect(_hwnd, &r);
		int x = (GetSystemMetrics(SM_CXSCREEN) - r.right) / 2;
		int y = (GetSystemMetrics(SM_CYSCREEN) - r.bottom) / 2;
		SetWindowPos(_hwnd, 0, x, y, 0, 0, SWP_NOZORDER | SWP_NOSIZE);
	}
	ShowWindow(_hwnd, SW_SHOW);
	_in_window_creating = false;
	_hdc = GetDC(_hwnd);
	update_dimensions();
}

vp_app_impl::vp_app_impl(const vapp_prop* prop)
{
	this->prop = *prop;
	_first_frame = true;

	_window_width = (this->prop.width != 0) ? this->prop.width : 640;
	_window_height = (this->prop.height != 0) ? this->prop.height : 480;
	_window_title = (strlen(this->prop.title) != 0) ? this->prop.title : "Viper Application";

	_framebuffer_width = _window_width;
	_framebuffer_height = _window_height;

	this->prop.sample_count = (this->prop.sample_count != 0) ? this->prop.sample_count : 1;
	this->prop.swap_interval = (this->prop.swap_interval != 0) ? this->prop.swap_interval : 1;
	_dpi_scale = 1.0f;
}

void vp_app_impl::shutdown()
{
	call_cleanup();

#if VP_APP_D3D11_BACKEND
	if (_d3d11_rt)             _d3d11_rt->Release();
	if (_d3d11_rtv)            _d3d11_rtv->Release();
	if (_d3d11_ds)             _d3d11_ds->Release();
	if (_d3d11_dsv)            _d3d11_dsv->Release();
	if (_dxgi_swap_chain)      _dxgi_swap_chain->Release();
	if (_d3d11_device_context) _d3d11_device_context->Release();
	if (_d3d11_device)         _d3d11_device->Release();
#else // TODO Uninitialize GL
#endif

	if (_hwnd) {
		DestroyWindow(_hwnd);
		UnregisterClass("ViperApp", GetModuleHandle(0));
	}

	if (_clipboard_enabled) {
		delete[] _clipboard;
	}
}

void vp_app_impl::enable_debugging_log()
{
	_debugging_log_enabled = true;
}

void vp_app_impl::log(vapp_log_message_type type, const char* msg)
{
	log_event(type, msg);
}

void vp_app_impl::fail(const char* msg)
{
	fail_event(msg);
	_quit_ordered = true;
}

void vp_app_impl::run()
{
	init_keytable();
	init_dpi();
	create_window();

	try {
#if VP_APP_D3D11_BACKEND
		d3d11_create_device_and_swapchain();
		d3d11_create_default_render_target();
#else // TODO init GL
#endif
	} catch (std::exception& e) {
		std::cerr << "Error: " << e.what() << std::endl;
		return;
	}

	MSG msg = {};
	while (!_quit_ordered) {
		while (PeekMessage(&msg, NULL, 0, 0, PM_REMOVE)) {
			TranslateMessage(&msg);
			DispatchMessage(&msg);
		}
		do_frame();
#if VP_APP_D3D11_BACKEND
		_dxgi_swap_chain->Present(this->prop.swap_interval, 0);
		if (IsIconic(_hwnd)) {
			Sleep(16 * this->prop.swap_interval);
		}
#else	// TODO GL Swap buffers
#endif
		if (update_dimensions()) {
#if VP_APP_D3D11_BACKEND
			d3d11_resize_default_render_target();
#endif
			app_event(vapp_event_resized);
		}
	}
}

void vp_app_impl::enable_clipboard()
{
	_clipboard_enabled = true;
	_clipboard_size = 8192;
	_clipboard = new char[_clipboard_size];
}

void vp_app_impl::set_clipboard_size(int size)
{
	_clipboard_size = size;
}

void vp_app_impl::set_clipboard_string(const char* str)
{
	if (!_clipboard_enabled) {
		return;
	}

	wchar_t* wcbuf = nullptr;
	const int wcbuf_size = _clipboard_size * sizeof(wchar_t);
	HGLOBAL object = nullptr;

	try {
		object = GlobalAlloc(GMEM_MOVEABLE, wcbuf_size);
		if (!object) {
			throw std::runtime_error("can't allocate clipboard object");
		}
		wcbuf = (wchar_t*)GlobalLock(object);
		if (!wcbuf) {
			throw std::runtime_error("can't lock clipboard object");
		}
		if (!utf8_to_utf16(str, wcbuf, wcbuf_size)) {
			throw std::runtime_error("can't convert to unicode");
		}
		GlobalUnlock(wcbuf);
		wcbuf = nullptr;
		if (!OpenClipboard(_hwnd)) {
			throw std::runtime_error("can't set clipboard data");
		}
		EmptyClipboard();
		SetClipboardData(CF_UNICODETEXT, object);
		CloseClipboard();
	} catch (std::exception& e) {
		std::cerr << "WARNING: Set clipboard failed, " << e.what() << std::endl;
		if (wcbuf) {
			GlobalUnlock(object);
		}
		if (object) {
			GlobalFree(object);
		}
		return;
	}

	vp_copy_string(str, _clipboard, _clipboard_size);
}

const char* vp_app_impl::get_clipboard_string()
{
	if (!_clipboard_enabled) {
		return "";
	}
	if (!OpenClipboard(_hwnd)) {
		return _clipboard;
	}
	HANDLE object = GetClipboardData(CF_UNICODETEXT);
	if (!object) {
		CloseClipboard();
		return _clipboard;
	}
	const wchar_t* wcbuf = (const WCHAR*)GlobalLock(object);
	if (!wcbuf) {
		CloseClipboard();
		return _clipboard;
	}
	utf16_to_utf8(wcbuf, _clipboard, _clipboard_size);
	GlobalUnlock(object);
	CloseClipboard();
	return _clipboard;
}

void vp_app_impl::quit()
{
	_quit_ordered = true;
}

void vp_app_impl::consume_event()
{
	_event_consumed = true;
}

bool vp_app_impl::is_keyboard_shown()
{
	return _onscreen_keyboard_shown;
}

void vp_app_impl::show_keyboard(bool shown)
{
	// TODO show keyboard for android and emscripten backend
}

int vp_app_impl::get_width()
{
	return (_framebuffer_width > 0) ? _framebuffer_width : 1;
}

int vp_app_impl::get_height()
{
	return (_framebuffer_height > 0) ? _framebuffer_height : 1;
}

float vp_app_impl::get_dpi_scale()
{
	return _dpi_scale;
}

#pragma region C and C++ Shared Library Entries
VP_API vp_app* vp_app::create(const vapp_prop* prop)
{
	assert(prop);
	vp_app_impl::instance = vp_app_impl{prop};
	return &vp_app_impl::instance;
}

VP_EXTERN_C VP_API vp_app* vp_app_create(const vapp_prop* prop)
{
	return vp_app::create(prop);
}

vp_impl_c_method0(vp_app, shutdown);
vp_impl_c_method0(vp_app, run);

vp_impl_c_method0(vp_app, enable_debugging_log);
vp_impl_c_method2(vp_app, log, vapp_log_message_type, const char*);
vp_impl_c_method1(vp_app, fail, const char*);

vp_impl_c_method0(vp_app, enable_clipboard);
vp_impl_c_method1(vp_app, set_clipboard_size, int);
vp_impl_c_method1(vp_app, set_clipboard_string, const char*);
vp_impl_c_function0(vp_app, get_clipboard_string, const char*);

#if VP_APP_D3D11_BACKEND
vp_impl_c_function0(vp_app, d3d11_get_device, const void*);
vp_impl_c_function0(vp_app, d3d11_get_device_context, const void*);
vp_impl_c_function0(vp_app, d3d11_get_render_target_view, const void*);
vp_impl_c_function0(vp_app, d3d11_get_depth_stencil_view, const void*);
#endif

vp_impl_c_function0(vp_app, get_width, int);
vp_impl_c_function0(vp_app, get_height, int);
vp_impl_c_function0(vp_app, get_dpi_scale, float);

vp_impl_c_method1(vp_app, show_keyboard, bool);
vp_impl_c_function0(vp_app, is_keyboard_shown, bool);

vp_impl_c_method0(vp_app, quit);
vp_impl_c_method0(vp_app, consume_event);
#pragma endregion
