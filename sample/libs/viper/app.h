#pragma once

#ifdef _WIN32
#define WIN32_LEAN_AND_MEAN
#include <Windows.h>
#include <windowsx.h>
#include <shellapi.h>
#endif

// Some Useful Macros
#ifdef __cplusplus
#define VP_EXTERN_C extern "C"

#define vp_begin_struct(name)     struct name {
#define vp_begin_enum(name)       enum name {
#define vp_end(name)              }

#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <cstdint>
#include <iostream>
#else
#define VP_EXTERN_C
#define VP_C_INTERFACE

#define vp_begin_struct(name)     typedef struct __##name {
#define vp_begin_enum(name)       typedef enum __##name {
#define vp_end(name)              } name

#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <stdbool.h>
#include <string.h>
#endif

#define VP_APP_INCLUDED

// Platform specific macro for building shared library
#ifndef VP_API
#   if (defined _WIN32 || defined __CYGWIN__)
#       if (defined(VP_DLL) && !defined(VP_DECL))
#           define VP_API __declspec(dllexport)
#       elif (defined(VP_DLL))
#           define VP_API __declspec(dllimport)
#		else
#			define VP_API
#       endif
#   elif (defined (__GNUC__) && defined(VP_DLL))
#       if (__GNUC__ >= 4)
#			define VP_API __attribute__((visibility("default")))
#		endif
#	else
#       define VP_API
#   endif
#endif

#ifdef _WIN32
#ifdef VP_USE_ANGLE
#define VP_APP_ANGLE_BACKEND 1
#define VP_APP_D3D11_BACKEND 0
#else
#define VP_APP_ANGLE_BACKEND 0
#define VP_APP_D3D11_BACKEND 1
#endif
#endif

// C function callback macro
#define vp_function(name, ret, ...)        ret (*name)(__VA_ARGS__)

#if (defined _MSC_VER)
#define VP_ALIGN(a) __declspec(align(a))
#elif (defined __GNUC__)
#define VP_ALIGN(a) __attribute__((aligned(a)))
#endif

// C binding utilities
#define vp_impl_c_function0(prefix, name, ret)                      VP_EXTERN_C VP_API ret prefix##_##name(prefix* ctx) { return ctx->name(); }
#define vp_impl_c_function1(prefix, name, ret, t1)                  VP_EXTERN_C VP_API ret prefix##_##name(prefix* ctx, t1 a1) { return ctx->name(a1); }
#define vp_impl_c_function2(prefix, name, ret, t1, t2)              VP_EXTERN_C VP_API ret prefix##_##name(prefix* ctx, t1 a1, t2 a2) { return ctx->name(a1, a2); }
#define vp_impl_c_function3(prefix, name, ret, t1, t2, t3)          VP_EXTERN_C VP_API ret prefix##_##name(prefix* ctx, t1 a1, t2 a2, t3 a3) { return ctx->name(a1, a2, a3); }
#define vp_impl_c_function4(prefix, name, ret, t1, t2, t3, t4)      VP_EXTERN_C VP_API ret prefix##_##name(prefix* ctx, t1 a1, t2 a2, t3 a3, t4 a4) { return ctx->name(a1, a2, a3, a4); }
#define vp_impl_c_function5(prefix, name, ret, t1, t2, t3, t4, t5)  VP_EXTERN_C VP_API ret prefix##_##name(prefix* ctx, t1 a1, t2 a2, t3 a3, t4 a4, t5 a5) { return ctx->name(a1, a2, a3, a4, a5); }

#define vp_impl_c_method0(prefix, name)                             VP_EXTERN_C VP_API void prefix##_##name(prefix* ctx) { ctx->name(); }
#define vp_impl_c_method1(prefix, name, t1)                         VP_EXTERN_C VP_API void prefix##_##name(prefix* ctx, t1 a1) { ctx->name(a1); }
#define vp_impl_c_method2(prefix, name, t1, t2)                     VP_EXTERN_C VP_API void prefix##_##name(prefix* ctx, t1 a1, t2 a2) { ctx->name(a1, a2); }
#define vp_impl_c_method3(prefix, name, t1, t2, t3)                 VP_EXTERN_C VP_API void prefix##_##name(prefix* ctx, t1 a1, t2 a2, t3 a3) { ctx->name(a1, a2, a3); }
#define vp_impl_c_method4(prefix, name, t1, t2, t3, t4)             VP_EXTERN_C VP_API void prefix##_##name(prefix* ctx, t1 a1, t2 a2, t3 a3, t4 a4) { ctx->name(a1, a2, a3, a4); }
#define vp_impl_c_method5(prefix, name, t1, t2, t3, t4, t5)         VP_EXTERN_C VP_API void prefix##_##name(prefix* ctx, t1 a1, t2 a2, t3 a3, t4 a4, t5 a5) { ctx->name(a1, a2, a3, a4, a5); }

// Small size string utilities, used by ViperGFX
// for vertex and fragment shader's semantic name
vp_begin_struct(vp_str)
	char buf[16];
vp_end(vp_str);

inline void vp_strcpy(vp_str* dst, const char* src)
{
	if (src) {
#ifdef _MSC_VER
		strncpy_s(dst->buf, 16, src, 15);
#else
		strncpy(dst->buf, src, 16);
#endif
		dst->buf[15] = 0;
	} else {
		memset(dst->buf, 0, 16);
	}
}

inline const char* vp_strptr(const vp_str* str)
{
	return &str->buf[0];
}

// C strcpy replacement
inline void vp_copy_string(const char* src, char* dst, int length)
{
	char* const end = &(dst[length - 1]);
	char c = 0;
	for (int i = 0; i < length; i++) {
		c = *src;
		if (c != 0) {
			src++;
		}
		*dst++ = c;
	}
	if (c != 0) {
		*end = 0;
	}
}

vp_begin_enum(vapp_event_type)
	vapp_event_invalid,
	vapp_event_key_down,
	vapp_event_key_up,
	vapp_event_char,
	vapp_event_mouse_down,
	vapp_event_mouse_up,
	vapp_event_mouse_scroll,
	vapp_event_mouse_move,
	vapp_event_mouse_enter,
	vapp_event_mouse_leave,
	vapp_event_touches_begin,
	vapp_event_touches_moved,
	vapp_event_touches_ended,
	vapp_event_touches_cancelled,
	vapp_event_resized,
	vapp_event_iconified,
	vapp_event_restored,
	vapp_event_suspended,
	vapp_event_resumed,
	vapp_event_update_cursor,
	vapp_event_clipboard_pasted
vp_end(vapp_event_type);

vp_begin_enum(vapp_mouse_button)
	vapp_mb_invalid = -1,
	vapp_mb_left,
	vapp_mb_right,
	vapp_mb_middle
vp_end(vapp_mouse_button);

vp_begin_enum(vapp_key_modifier)
	vapp_kmod_shift = (1 << 0),
	vapp_kmod_ctrl  = (1 << 1),
	vapp_kmod_alt   = (1 << 2),
	vapp_kmod_super = (1 << 3)
vp_end(vapp_key_modifier);

vp_begin_enum(vapp_key_code)
	vapp_key_space = 32,
	vapp_key_apostrophe = 39,
	vapp_key_comma = 44,
	vapp_key_minus,
	vapp_key_period,
	vapp_key_slash,
	vapp_key_0,
	vapp_key_1,
	vapp_key_2,
	vapp_key_3,
	vapp_key_4,
	vapp_key_5,
	vapp_key_6,
	vapp_key_7,
	vapp_key_8,
	vapp_key_9,

	vapp_key_semicolon = 59,
	vapp_key_equal = 61,

	vapp_key_a = 65, 
	vapp_key_b, 
	vapp_key_c,
	vapp_key_d,
	vapp_key_e,
	vapp_key_f,
	vapp_key_g,
	vapp_key_h,
	vapp_key_i,
	vapp_key_j,
	vapp_key_k,
	vapp_key_l,
	vapp_key_m,
	vapp_key_n,
	vapp_key_o,
	vapp_key_p,
	vapp_key_q,
	vapp_key_r,
	vapp_key_s,
	vapp_key_t,
	vapp_key_u,
	vapp_key_v,
	vapp_key_w,
	vapp_key_x,
	vapp_key_y,
	vapp_key_z,

	vapp_key_left_bracket = 91,
	vapp_key_backslash = 92,
	vapp_key_right_bracket = 93,
	vapp_key_grave_accent = 96,
	vapp_key_world1 = 161,
	vapp_key_world2 = 162,
	vapp_key_escape = 256,
	vapp_key_enter = 257,
	vapp_key_tab = 258,
	vapp_key_backspace = 259,
	vapp_key_insert = 260,
	vapp_key_delete = 261,
	vapp_key_right = 262,
	vapp_key_left = 263,
	vapp_key_down = 264,
	vapp_key_up = 265,
	vapp_key_pageup = 266,
	vapp_key_pagedown = 267,
	vapp_key_home = 268,
	vapp_key_end = 269,
	vapp_key_caps_lock = 280,
	vapp_key_scroll_lock = 281,
	vapp_key_num_lock = 282,
	vapp_key_print_screen = 283,
	vapp_key_pause = 284,
	
	vapp_key_f1 = 290, 
	vapp_key_f2, 
	vapp_key_f3, 
	vapp_key_f4, 
	vapp_key_f5, 
	vapp_key_f6, 
	vapp_key_f7, 
	vapp_key_f8, 
	vapp_key_f9, 
	vapp_key_f10, 
	vapp_key_f11, 
	vapp_key_f12,
	vapp_key_f13, 
	vapp_key_f14, 
	vapp_key_f15, 
	vapp_key_f16, 
	vapp_key_f17, 
	vapp_key_f18, 
	vapp_key_f19, 
	vapp_key_f20, 
	vapp_key_f21, 
	vapp_key_f22, 
	vapp_key_f23, 
	vapp_key_f24, 
	vapp_key_f25,
	
	vapp_key_kp_0 = 320,
	vapp_key_kp_1, 
	vapp_key_kp_2, 
	vapp_key_kp_3, 
	vapp_key_kp_4, 
	vapp_key_kp_5, 
	vapp_key_kp_6,
	vapp_key_kp_7,
	vapp_key_kp_8,
	vapp_key_kp_9,
	vapp_key_kp_decimal,
	vapp_key_kp_divide,
	vapp_key_kp_multiply,
	vapp_key_kp_subtract,
	vapp_key_kp_add,
	vapp_key_kp_enter,
	vapp_key_kp_equal,

	vapp_key_left_shift = 340,
	vapp_key_left_control,
	vapp_key_left_alt,
	vapp_key_left_super,
	vapp_key_right_shift,
	vapp_key_right_control,
	vapp_key_right_alt,
	vapp_key_right_super,
	vapp_key_menu
vp_end(vapp_key_code);

vp_begin_enum(vapp_log_message_type)
	vapp_log_message_type_debug,
	vapp_log_message_type_warning,
	vapp_log_message_type_error
vp_end(vapp_log_message_type);

enum 
{
	VAPP_MAX_KEYCODES = 512,
	VAPP_MAX_MOUSEBUTTONS = 3,
	VAPP_MAX_TOUCHPOINTS = 8
};

vp_begin_struct(vapp_touchpoint)
	uint32_t identifier;
	float pos_x;
	float pos_y;
	bool changed;
vp_end(vapp_touchpoint);

vp_begin_struct(vapp_event)
	vapp_event_type type;
	uint64_t frame_count;
	int window_width, window_height;
	int framebuffer_width, framebuffer_height;
	float mouse_x, mouse_y, scroll_x, scroll_y;
	vapp_mouse_button mouse_button;
	vapp_touchpoint touches[VAPP_MAX_TOUCHPOINTS];
	vapp_key_code key_code;
	uint32_t modifiers;
	bool key_repeat;
	uint32_t char_code;
vp_end(vapp_event);

vp_begin_struct(vapp_prop)
	vp_function(init_cb, void);
	vp_function(frame_cb, void);
	vp_function(cleanup_cb, void);
	vp_function(event_cb, void, const vapp_event*);
	vp_function(log_cb, void, const char*);
	vp_function(fail_cb, void, const char*);

	void* user_data;
	vp_function(init_user_data_cb, void, void*);
	vp_function(frame_user_data_cb, void, void*);
	vp_function(cleanup_user_data_cb, void, void*);
	vp_function(event_user_data_cb, void, const vapp_event*, void*);
	vp_function(log_user_data_cb, void, const char*, void*);
	vp_function(fail_user_data_cb, void, const char*, void*);

	int width;
	int height;
	const char* title;
	void* icon;
	int sample_count;
	int swap_interval;
	bool high_dpi;
	bool full_screen;
	bool resizable;
	bool auto_center;
vp_end(vapp_prop);

// ViperApp core struct
#if (defined(__cplusplus) && !defined(VP_C_INTERFACE))
vp_begin_struct(vp_app)
	// Initializer
	VP_API static vp_app* create(const vapp_prop* prop);

	// Run function and deinitializer
	virtual void shutdown() = 0;
	virtual void run() = 0;

	// Logging functions
	virtual void enable_debugging_log() = 0;
	virtual void log(vapp_log_message_type logtype, const char* msg) = 0;
	virtual void fail(const char* msg) = 0;

	// Clipboard functions
	virtual void enable_clipboard() = 0;
	virtual void set_clipboard_size(int size) = 0;
	virtual void set_clipboard_string(const char* str) = 0;
	virtual const char* get_clipboard_string() = 0;

	// Platform specific functions
#if VP_APP_D3D11_BACKEND
	virtual const void* d3d11_get_device() = 0;
	virtual const void* d3d11_get_device_context() = 0;
	virtual const void* d3d11_get_render_target_view() = 0;
	virtual const void* d3d11_get_depth_stencil_view() = 0;
#endif

	// Property getter functions
	virtual int get_width() = 0;
	virtual int get_height() = 0;
	virtual float get_dpi_scale() = 0;

	// Mobile application support (WIP)
	virtual void show_keyboard(bool shown) = 0;
	virtual bool is_keyboard_shown() = 0;

	// Application events stuff
	virtual void quit() = 0;
	virtual void consume_event() = 0;
vp_end(vp_app);
#endif

#ifdef VP_C_INTERFACE
typedef struct vp_app* vp_app;

// Initializer
VP_EXTERN_C VP_API vp_app* vp_app_create(const vapp_prop* prop);

// Run function and deinitializer
VP_EXTERN_C VP_API void vp_app_run(vp_app* app);
VP_EXTERN_C VP_API void vp_app_shutdown(vp_app* app);

// Logging functions
VP_EXTERN_C VP_API void vp_app_enable_debugging_log(vp_app* app);
VP_EXTERN_C VP_API void vp_app_log(vp_app* app, vapp_log_message_type type, const char* msg);
VP_EXTERN_C VP_API void vp_app_fail(vp_app* app, const char* msg);

// Clipboard functions
VP_EXTERN_C VP_API void vp_app_enable_clipboard(vp_app* app);
VP_EXTERN_C VP_API void vp_app_set_clipboard_size(vp_app* app, int size);
VP_EXTERN_C VP_API void vp_app_set_clipboard_string(vp_app* app, const char* str);
VP_EXTERN_C VP_API const char* vp_app_get_clipboard_string(vp_app* app);

// Platform specific functions
#if VP_APP_D3D11_BACKEND
VP_EXTERN_C VP_API const void* vp_app_d3d11_get_device(vp_app* app);
VP_EXTERN_C VP_API const void* vp_app_d3d11_get_device_context(vp_app* app);
VP_EXTERN_C VP_API const void* vp_app_d3d11_get_render_target_view(vp_app* app);
VP_EXTERN_C VP_API const void* vp_app_d3d11_get_depth_stencil_view(vp_app* app);
#endif

// Property getter functions
VP_EXTERN_C VP_API int vp_app_get_width(vp_app* app);
VP_EXTERN_C VP_API int vp_app_get_height(vp_app* app);
VP_EXTERN_C VP_API float vp_app_get_dpi_scale(vp_app* app);

// Mobile application support
VP_EXTERN_C VP_API void vp_app_show_keyboard(vp_app* app, bool shown);
VP_EXTERN_C VP_API bool vp_app_is_keyboard_shown(vp_app* app);

// Application events stuff
VP_EXTERN_C VP_API void vp_app_consume_event(vp_app* app);
VP_EXTERN_C VP_API void vp_app_quit(vp_app* app);
#endif
