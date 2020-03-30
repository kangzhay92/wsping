#ifndef VP_UNITY_BUILD
#include <viper/app.h>
#include <viper/gfx.h>
#endif
#include <cassert>

#ifndef VN_IMGUI_C_INTERFACE
#include <imgui/imgui.h>
#else
#define CIMGUI_NO_EXPORT
#define CIMGUI_DEFINE_ENUMS_AND_STRUCTS
#include <imgui/cimgui.h>
#endif

#include "imgui.h"

vn_imgui vn_imgui::instance;

#if VP_APP_D3D11_BACKEND
static const char* imgui_vs_source = R"(
cbuffer params
{
	float2 disp_size;
};

struct VSOutput
{
	float2 uv : TEXCOORD0;
	float4 color : COLOR0;
	float4 pos : SV_POSITION;
};

VSOutput main(float2 pos: POSITION, float2 uv: TEXCOORD0, float4 color: COLOR0)
{
	VSOutput Out;
	Out.pos = float4(((pos / disp_size) - 0.5) * float2(2.0, -2.0), 0.5, 1.0);
	Out.uv = uv;
	Out.color = color;
	return Out;
})";

static const char* imgui_fs_source = R"(
Texture2D<float4> tex : register(t0);
SamplerState smp: register(s0);

float4 main(float2 uv: TEXCOORD0, float4 color: COLOR0) : SV_TARGET0
{
	return tex.Sample(smp, uv) * color;
})";
#endif

struct imgui_vs_param
{
	ImVec2 disp_size;
};

void vn_imgui::set_clipboard(void* user_data, const char* text)
{
	auto app = (vp_app*)user_data;
	app->set_clipboard_string(text);
}

const char* vn_imgui::get_clipboard(void* user_data)
{
	auto app = (vp_app*)user_data;
	return app->get_clipboard_string();
}

vn_imgui* vn_imgui::create(const vimgui_prop* prop, vp_app* app, vp_gfx* gfx)
{
	assert(prop && app && gfx);
	instance = vn_imgui{prop, app, gfx};
	return &instance;
}

vn_imgui::vn_imgui(const vimgui_prop* prop, vp_app* app, vp_gfx* gfx)
{
	_app = app;
	_gfx = gfx;

	_prop = *prop;
	_prop.max_vertices = (_prop.max_vertices != 0) ? _prop.max_vertices : 65536;
	_prop.dpi_scale = (_prop.dpi_scale != 0.0f) ? _prop.dpi_scale : 1.0f;
	_prop.theme = (_prop.theme != vimgui_style_default) ? _prop.theme : vimgui_style_dark;

#ifndef VN_IMGUI_C_INTERFACE
	ImGui::CreateContext();
	switch (_prop.theme) {
		case vimgui_style_dark:
			ImGui::StyleColorsDark();
			break;
		case vimgui_style_classic:
			ImGui::StyleColorsClassic();
			break;
		case vimgui_style_light:
			ImGui::StyleColorsLight();
			break;
	}
	ImGuiIO* io = &ImGui::GetIO();
	if (!_prop.no_default_font) {
		io->Fonts->AddFontDefault();
	}
#else
	igCreateContext(NULL);
	switch (_prop.theme) {
		case vimgui_style_dark:
			igStyleColorsDark(igGetStyle());
			break;
		case vimgui_style_light:
			igStyleColorsLight(igGetStyle());
			break;
		case vimgui_style_classic:
			igStyleColorsClassic(igGetStyle());
			break;
	}
	ImGuiIO* io = igGetIO();
	if (!_prop.no_default_font) {
		ImFontAtlas_AddFontDefault(io->Fonts, NULL);
	}
#endif
	io->IniFilename = _prop.ini_filename;
	io->ConfigMacOSXBehaviors = false;
	io->BackendFlags |= ImGuiBackendFlags_RendererHasVtxOffset;
	io->KeyMap[ImGuiKey_Tab] = vapp_key_tab;
	io->KeyMap[ImGuiKey_LeftArrow] = vapp_key_left;
	io->KeyMap[ImGuiKey_RightArrow] = vapp_key_right;
	io->KeyMap[ImGuiKey_UpArrow] = vapp_key_up;
	io->KeyMap[ImGuiKey_DownArrow] = vapp_key_down;
	io->KeyMap[ImGuiKey_PageUp] = vapp_key_pageup;
	io->KeyMap[ImGuiKey_PageDown] = vapp_key_pagedown;
	io->KeyMap[ImGuiKey_Home] = vapp_key_home;
	io->KeyMap[ImGuiKey_End] = vapp_key_end;
	io->KeyMap[ImGuiKey_Delete] = vapp_key_delete;
	io->KeyMap[ImGuiKey_Backspace] = vapp_key_backspace;
	io->KeyMap[ImGuiKey_Space] = vapp_key_space;
	io->KeyMap[ImGuiKey_Enter] = vapp_key_enter;
	io->KeyMap[ImGuiKey_Escape] = vapp_key_escape;
	if (!_prop.disable_hotkeys) {
		io->KeyMap[ImGuiKey_A] = vapp_key_a;
		io->KeyMap[ImGuiKey_C] = vapp_key_c;
		io->KeyMap[ImGuiKey_V] = vapp_key_v;
		io->KeyMap[ImGuiKey_X] = vapp_key_x;
		io->KeyMap[ImGuiKey_Y] = vapp_key_y;
		io->KeyMap[ImGuiKey_Z] = vapp_key_z;
	}
	io->ClipboardUserData  = _app;
	io->SetClipboardTextFn = set_clipboard;
	io->GetClipboardTextFn = get_clipboard;

	vgfx_buffer_prop vb = {};
	vb.usage = vgfx_usage_stream;
	vb.size = _prop.max_vertices * sizeof(ImDrawVert);
	_vbuf = _gfx->make_buffer(&vb);

	vgfx_buffer_prop ib = {};
	ib.type = vgfx_buffer_type_index_buffer;
	ib.usage = vgfx_usage_stream;
	ib.size = _prop.max_vertices * 3 * sizeof(unsigned short);
	_ibuf = _gfx->make_buffer(&ib);

	if (!_prop.no_default_font) {
		uint8_t* font_pixels;
		int font_width, font_height;
#ifndef VN_IMGUI_C_INTERFACE
		io->Fonts->GetTexDataAsRGBA32(&font_pixels, &font_width, &font_height);
#else
		int bpp;
		ImFontAtlas_GetTexDataAsRGBA32(io->Fonts, &font_pixels, &font_width, &font_height, &bpp);
#endif
		vgfx_image_prop img = {};
		img.width = font_width;
		img.height = font_height;
		img.pixel_format = vgfx_pixel_format_rgba8;
		img.wrap_u = vgfx_wrap_clamp_to_edge;
		img.wrap_v = vgfx_wrap_clamp_to_edge;
		img.min_filter = vgfx_filter_linear;
		img.mag_filter = vgfx_filter_linear;
		img.content.subimage[0][0].ptr = font_pixels;
		img.content.subimage[0][0].size = font_width * font_height * sizeof(unsigned long);
		_img = _gfx->make_image(&img);
		io->Fonts->TexID = (ImTextureID)_img.id;
	}

	vgfx_shader_prop shd = {};
	auto ub = &shd.vs.uniform_blocks[0];
	ub->size = sizeof(imgui_vs_param);
	ub->uniforms[0].name = "disp_size";
	ub->uniforms[0].type = vgfx_uniform_type_float2;
	shd.attrs[0].name = "position";
	shd.attrs[0].sem_name = "POSITION";
	shd.attrs[1].name = "texcoord";
	shd.attrs[1].sem_name = "TEXCOORD";
	shd.attrs[2].name = "color";
	shd.attrs[2].sem_name = "COLOR";
	shd.fs.images[0].name = "tex";
	shd.fs.images[0].type = vgfx_image_type_2d; 
	shd.vs.source = imgui_vs_source;
	shd.fs.source = imgui_fs_source;
	_shd = _gfx->make_shader(&shd);

	vgfx_pipeline_prop pip = {};
	pip.layout.buffers[0].stride = sizeof(ImDrawVert);
	{
		vgfx_vertex_attr_prop* attr = &pip.layout.attrs[0];
		attr->offset = offsetof(ImDrawVert, pos);
		attr->format = vgfx_vertex_format_float2;
	}
	{
		vgfx_vertex_attr_prop* attr = &pip.layout.attrs[1];
		attr->offset = offsetof(ImDrawVert, uv);
		attr->format = vgfx_vertex_format_float2;
	}
	{
		vgfx_vertex_attr_prop* attr = &pip.layout.attrs[2];
		attr->offset = offsetof(ImDrawVert, col);
		attr->format = vgfx_vertex_format_ubyte4n;
	}
	pip.shader = _shd;
	pip.index_type = vgfx_index_type_uint16;
	pip.blend.enabled = true;
	pip.blend.src_factor_rgb = vgfx_blend_factor_src_alpha;
	pip.blend.dst_factor_rgb = vgfx_blend_factor_one_minus_src_alpha;
	pip.blend.color_write_mask = VGFX_COLORMASK_RGB;
	pip.blend.color_format = _prop.color_format;
	pip.blend.depth_format = _prop.depth_format;
	pip.rasterizer.sample_count = _prop.sample_count;
	_pip = _gfx->make_pipeline(&pip);
}

void vn_imgui::shutdown()
{
#ifndef VN_IMGUI_C_INTERFACE
	ImGui::DestroyContext();
#else
	igDestroyContext(NULL);
#endif
	_gfx->destroy_pipeline(_pip);
	_gfx->destroy_shader(_shd);
	if (!_prop.no_default_font) {
		// FIXME: Bug occurred, sometimes application crash if we using custom font
		_gfx->destroy_image(_img);
	}
	_gfx->destroy_buffer(_ibuf);
	_gfx->destroy_buffer(_vbuf);
}

void vn_imgui::set_modifiers(ImGuiIO* io, uint32_t mods)
{
	io->KeyAlt   = (mods & vapp_kmod_alt) != 0;
	io->KeyCtrl  = (mods & vapp_kmod_ctrl) != 0;
	io->KeyShift = (mods & vapp_kmod_shift) != 0;
	io->KeySuper = (mods & vapp_kmod_super) != 0;
}

bool vn_imgui::is_ctrl(unsigned modifiers)
{
	return (modifiers & vapp_kmod_ctrl) != 0;
}

void vn_imgui::new_frame(int width, int height, double dt)
{
#ifndef VN_IMGUI_C_INTERFACE
	ImGuiIO* io = &ImGui::GetIO();
#else
	ImGuiIO* io = igGetIO();
#endif
	io->DisplaySize.x = ((float)width) / _prop.dpi_scale;
	io->DisplaySize.y = ((float)height) / _prop.dpi_scale;
	for (int i = 0; i < VAPP_MAX_MOUSEBUTTONS; i++) {
		if (btn_down[i]) {
			btn_down[i] = false;
			io->MouseDown[i] = true;
		} else if (btn_up[i]) {
			btn_up[i] = false;
			io->MouseDown[i] = false;
		}
	}
	for (int i = 0; i < VAPP_MAX_KEYCODES; i++) {
		if (keys_down[i]) {
			io->KeysDown[i] = true;
			set_modifiers(io, keys_down[i]);
			keys_down[i] = 0;
		} else if (keys_up[i]) {
			io->KeysDown[i] = false;
			set_modifiers(io, keys_up[i]);
			keys_up[i] = 0;
		}
	}
	if (io->WantTextInput && !_app->is_keyboard_shown()) {
		_app->show_keyboard(true);
	}
	if (!io->WantTextInput && _app->is_keyboard_shown()) {
		_app->show_keyboard(false);
	}
#ifndef VN_IMGUI_C_INTERFACE
	ImGui::NewFrame();
#else
	igNewFrame();
#endif
}

void vn_imgui::render()
{
#ifndef VN_IMGUI_C_INTERFACE
	ImGui::Render();
	ImDrawData* draw_data = ImGui::GetDrawData();
	ImGuiIO* io = &ImGui::GetIO();
#else
	igRender();
	ImDrawData* draw_data = igGetDrawData();
	ImGuiIO* io = igGetIO();
#endif
	if (draw_data == nullptr) {
		return;
	}
	if (draw_data->CmdListsCount == 0) {
		return;
	}

	const float dpi_scale = _prop.dpi_scale;
	const int fb_width = (int)(io->DisplaySize.x * dpi_scale);
	const int fb_height = (int)(io->DisplaySize.y * dpi_scale);

	_gfx->apply_viewport(0, 0, fb_width, fb_height, true);
	_gfx->apply_scissor_rect(0, 0, fb_width, fb_height, true);
	_gfx->apply_pipeline(_pip);

	imgui_vs_param vsp;
	vsp.disp_size.x = io->DisplaySize.x;
	vsp.disp_size.y = io->DisplaySize.y;
	_gfx->apply_uniforms(vgfx_shader_stage_vs, 0, &vsp, sizeof(imgui_vs_param));

	vgfx_bindings bind = {};
	bind.vertex_buffers[0] = _vbuf;
	bind.index_buffer = _ibuf;
	ImTextureID tex_id = io->Fonts->TexID;
	bind.fs_images[0].id = (unsigned)tex_id;

	uint32_t vb_offset = 0;
	uint32_t ib_offset = 0;

	for (int cl_index = 0; cl_index < draw_data->CmdListsCount; cl_index++) {
		ImDrawList* cl = draw_data->CmdLists[cl_index];
#ifndef VN_IMGUI_C_INTERFACE
		const int vtx_size = cl->VtxBuffer.size() * sizeof(ImDrawVert);
		const int idx_size = cl->IdxBuffer.size() * sizeof(ImDrawIdx);
		const ImDrawVert* vtx_ptr = &cl->VtxBuffer.front();
		const ImDrawIdx* idx_ptr = &cl->IdxBuffer.front();
#else
		const int vtx_size = cl->VtxBuffer.Size * sizeof(ImDrawVert);
		const int idx_size = cl->IdxBuffer.Size * sizeof(ImDrawIdx);
		const ImDrawVert* vtx_ptr = cl->VtxBuffer.Data;
		const ImDrawIdx* idx_ptr = cl->IdxBuffer.Data;
#endif
		if (vtx_ptr) {
			vb_offset = _gfx->append_buffer(bind.vertex_buffers[0], vtx_ptr, vtx_size);
		}
		if (idx_ptr) {
			ib_offset = _gfx->append_buffer(bind.index_buffer, idx_ptr, idx_size);
		}
		if (_gfx->query_buffer_overflow(bind.vertex_buffers[0]) || 
			_gfx->query_buffer_overflow(bind.index_buffer)) {
			break;
		}
		bind.vertex_buffer_offsets[0] = vb_offset;
		bind.index_buffer_offset = ib_offset;
		_gfx->apply_bindings(&bind);

		int base_element = 0;
#ifndef VN_IMGUI_C_INTERFACE
		const int num_cmds = cl->CmdBuffer.size();
#else
		const int num_cmds = cl->CmdBuffer.Size;
#endif
		uint32_t vtx_offset = 0;
		for (int cmd_index = 0; cmd_index < num_cmds; cmd_index++) {
			ImDrawCmd* pcmd = &cl->CmdBuffer.Data[cmd_index];
			if (pcmd->UserCallback) {
				pcmd->UserCallback(cl, pcmd);
				_gfx->apply_viewport(0, 0, fb_width, fb_height, true);
				_gfx->apply_pipeline(_pip);
				_gfx->apply_uniforms(vgfx_shader_stage_vs, 0, &vsp, sizeof(imgui_vs_param));
				_gfx->apply_bindings(&bind);
			} else {
				if ((tex_id != pcmd->TextureId) || (vtx_offset != pcmd->VtxOffset)) {
					tex_id = pcmd->TextureId;
					vtx_offset = pcmd->VtxOffset * sizeof(ImDrawVert);
					bind.fs_images[0].id = (unsigned)tex_id;
					bind.vertex_buffer_offsets[0] = vb_offset + vtx_offset;
					_gfx->apply_bindings(&bind);
				}
				const int scissor_x = (int)(pcmd->ClipRect.x * dpi_scale);
				const int scissor_y = (int)(pcmd->ClipRect.y * dpi_scale);
				const int scissor_w = (int)((pcmd->ClipRect.z - pcmd->ClipRect.x) * dpi_scale);
				const int scissor_h = (int)((pcmd->ClipRect.w - pcmd->ClipRect.y) * dpi_scale);
				_gfx->apply_scissor_rect(scissor_x, scissor_y, scissor_w, scissor_h, true);
				_gfx->draw(base_element, pcmd->ElemCount, 1);
			}
			base_element += pcmd->ElemCount;
		}
	}

	_gfx->apply_viewport(0, 0, fb_width, fb_height, true);
	_gfx->apply_scissor_rect(0, 0, fb_width, fb_height, true);
}

bool vn_imgui::handle_event(const vapp_event* ev)
{
	const float dpi_scale = _prop.dpi_scale;
#ifndef VN_IMGUI_C_INTERFACE
	ImGuiIO* io = &ImGui::GetIO();
#else
	ImGuiIO* io = igGetIO();
#endif
	set_modifiers(io, ev->modifiers);
	switch (ev->type) {
		case vapp_event_mouse_down:
			io->MousePos.x = ev->mouse_x / dpi_scale;
			io->MousePos.y = ev->mouse_y / dpi_scale;
			if (ev->mouse_button < 3) {
				btn_down[ev->mouse_button] = true;
			}
			break;
		case vapp_event_mouse_up:
			io->MousePos.x = ev->mouse_x / dpi_scale;
			io->MousePos.y = ev->mouse_y / dpi_scale;
			if (ev->mouse_button < 3) {
				btn_up[ev->mouse_button] = true;
			}
			break;
		case vapp_event_mouse_move:
			io->MousePos.x = ev->mouse_x / dpi_scale;
			io->MousePos.y = ev->mouse_y / dpi_scale;
			break;
		case vapp_event_mouse_enter:
		case vapp_event_mouse_leave:
			for (int i = 0; i < 3; i++) {
				btn_down[i] = false;
				btn_up[i] = false;
				io->MouseDown[i] = false;
			}
			break;
		case vapp_event_mouse_scroll:
			io->MouseWheelH = ev->scroll_x;
			io->MouseWheel = ev->scroll_y;
			break;
		case vapp_event_touches_begin:
			btn_down[0] = true;
			io->MousePos.x = ev->touches[0].pos_x / dpi_scale;
			io->MousePos.y = ev->touches[0].pos_y / dpi_scale;
			break;
		case vapp_event_touches_moved:
			io->MousePos.x = ev->touches[0].pos_x / dpi_scale;
			io->MousePos.y = ev->touches[0].pos_y / dpi_scale;
			break;
		case vapp_event_touches_ended:
			btn_up[0] = true;
			io->MousePos.x = ev->touches[0].pos_x / dpi_scale;
			io->MousePos.y = ev->touches[0].pos_y / dpi_scale;
			break;
		case vapp_event_touches_cancelled:
			btn_up[0] = btn_down[0] = false;
			break;
		case vapp_event_key_down:
			if (is_ctrl(ev->modifiers) && (ev->key_code == vapp_key_v)) {
				break;
			}
			if (is_ctrl(ev->modifiers) && (ev->key_code == vapp_key_x)) {
				_app->consume_event();
			}
			if (is_ctrl(ev->modifiers) && (ev->key_code == vapp_key_c)) {
				_app->consume_event();
			}
			keys_down[ev->key_code] = 0x80 | (BYTE)ev->modifiers;
			break;
		case vapp_event_key_up:
			if (is_ctrl(ev->modifiers) && (ev->key_code == vapp_key_v)) {
				break;
			}
			if (is_ctrl(ev->modifiers) && (ev->key_code == vapp_key_x)) {
				_app->consume_event();
			}
			if (is_ctrl(ev->modifiers) && (ev->key_code == vapp_key_c)) {
				_app->consume_event();
			}
			keys_up[ev->key_code] = 0x80 | (BYTE)ev->modifiers;
			break;
		case vapp_event_char:
			if ((ev->char_code >= 32) && (ev->char_code != 127) && 
			   ((ev->modifiers & (vapp_kmod_alt | vapp_kmod_ctrl | vapp_kmod_super)) == 0)) {
#ifndef VN_IMGUI_C_INTERFACE
				io->AddInputCharacter((ImWchar)ev->char_code);
#else
				ImGuiIO_AddInputCharacter(io, (ImWchar)ev->char_code);
#endif
			}
			break;
		case vapp_event_clipboard_pasted:
			keys_down[vapp_key_v] = keys_up[vapp_key_v] = 0x80 | vapp_kmod_ctrl;
			break;
		default:
			break;
	}
	return io->WantCaptureKeyboard || io->WantCaptureMouse;
}

#pragma region C Bindings
VP_EXTERN_C vn_imgui* vn_imgui_create(const vimgui_prop* prop, vp_app* app, vp_gfx* gfx)
{
	return vn_imgui::create(prop, app, gfx);
}

VP_EXTERN_C void vn_imgui_shutdown(vn_imgui* ctx)
{
	ctx->shutdown();
}

VP_EXTERN_C void vn_imgui_new_frame(vn_imgui* ctx, int width, int height, double dt)
{
	ctx->new_frame(width, height, dt);
}

VP_EXTERN_C void vn_imgui_render(vn_imgui* ctx)
{
	ctx->render();
}

VP_EXTERN_C bool vn_imgui_handle_event(vn_imgui* ctx, const vapp_event* ev)
{
	return ctx->handle_event(ev);
}
#pragma endregion
