#pragma once

#ifndef VP_GFX_INCLUDED
#error "Please include <viper/gfx.h> before <venom/imgui.h>"
#endif

#ifndef VN_IMGUI_C_INTERFACE
#	ifndef IMGUI_VERSION
#	error "Please include imgui.h before <venom/imgui.h>"
#	endif
#else
#	ifndef CIMGUI_INCLUDED
#	error "Please include cimgui.h before <venom/imgui.h>"
#	endif
#endif

vp_begin_enum(vimgui_style)
	vimgui_style_default,
	vimgui_style_dark,
	vimgui_style_light,
	vimgui_style_classic
vp_end(vimgui_style);

vp_begin_struct(vimgui_prop)
	vgfx_pixel_format color_format;
	vgfx_pixel_format depth_format;
	int max_vertices;
	int sample_count;
	float dpi_scale;
	const char* ini_filename;
	bool no_default_font;
	bool disable_hotkeys;
	vimgui_style theme;
vp_end(vimgui_prop);

#ifndef VP_C_INTERFACE
class vn_imgui
{
public:
	vn_imgui() = default;
	~vn_imgui() {}

	static vn_imgui* create(const vimgui_prop* prop, vp_app* app, vp_gfx* gfx);

	void shutdown();
	void new_frame(int width, int height, double dt);
	void render();

	bool handle_event(const vapp_event* ev);

private:
	vn_imgui(const vimgui_prop* prop, vp_app* app, vp_gfx* gfx);

	static bool is_ctrl(unsigned modifiers);
	static void set_modifiers(ImGuiIO* io, uint32_t mods);

	static void set_clipboard(void* user_data, const char* text);
	static const char* get_clipboard(void* user_data);

	vp_app* _app;
	vp_gfx* _gfx;
	vimgui_prop _prop;

	vgfx_buffer _vbuf;
	vgfx_buffer _ibuf;
	vgfx_image _img;
	vgfx_shader _shd;
	vgfx_pipeline _pip;

	bool btn_down[VAPP_MAX_MOUSEBUTTONS];
	bool btn_up[VAPP_MAX_MOUSEBUTTONS];
	uint8_t keys_down[VAPP_MAX_KEYCODES];
	uint8_t keys_up[VAPP_MAX_KEYCODES];

	static vn_imgui instance;
};
#else
typedef struct vn_imgui vn_imgui;

VP_EXTERN_C vn_imgui* vn_imgui_create(const vimgui_prop* prop, vp_app* app, vp_gfx* gfx);
VP_EXTERN_C void vn_imgui_shutdown(vn_imgui* ctx);
VP_EXTERN_C void vn_imgui_new_frame(vn_imgui* ctx, int width, int height, double dt);
VP_EXTERN_C void vn_imgui_render(vn_imgui* ctx);
VP_EXTERN_C bool vn_imgui_handle_event(vn_imgui* ctx, const vapp_event* ev);
#endif
