#ifndef VP_UNITY_BUILD
#include "app.h"
#endif
#include "gfx.h"

#if VP_APP_D3D11_BACKEND
#include <d3d11.h>
#include <d3dcompiler.h>
#endif
#include <cassert>

// Platform specific declaration
struct native_buffer;
struct native_image;
struct native_shader;
struct native_pipeline;
struct native_pass;
struct native_context;

// Resource pools declaration
struct vgfx_pool
{
	int size;
	int queue_top;
	uint32_t* gen_ctrs;
	int* free_queue;
};

struct vgfx_pools
{
	vgfx_pool buffer_pool;
	vgfx_pool image_pool;
	vgfx_pool shader_pool;
	vgfx_pool pipeline_pool;
	vgfx_pool pass_pool;
	vgfx_pool context_pool;
	native_buffer* buffers;
	native_image* images;
	native_shader* shaders;
	native_pipeline* pipelines;
	native_pass* passes;
	native_context* contexts;
};

struct vgfx_slot
{
	unsigned id;
	unsigned ctx_id;
	vgfx_resource_state state;
};

static constexpr float DEFAULT_CLEAR_RED = 0.5f;
static constexpr float DEFAULT_CLEAR_GREEN = 0.5f;
static constexpr float DEFAULT_CLEAR_BLUE = 0.5f;
static constexpr float DEFAULT_CLEAR_ALPHA = 1.0f;
static constexpr float DEFAULT_CLEAR_DEPTH = 1.0f;
static constexpr unsigned char DEFAULT_CLEAR_STENCIL = 0;
static constexpr uint32_t MAX_POOL_SIZE = (1 << 16);

// Logging utility
enum vgfx_error_type
{
	vgfx_error_unallocated,
	vgfx_error_uninitialized,
	vgfx_error_create_failed,
	vgfx_error_invalid,
	vgfx_error_any
};

static const char* VGFX_GLOBAL_OBJECT = "ViperGFX";

struct vp_gfx_impl : public vp_gfx
{
	constexpr vp_gfx_impl() {}
	vp_gfx_impl(const vgfx_prop* prop, vp_app* app);
	~vp_gfx_impl() {}

	void shutdown() final;

	vgfx_prop query_props() final;
	vgfx_backend query_backend() final;
	vgfx_features query_features() final;
	vgfx_limits query_limits() final;
	vgfx_pixelformat_info query_pixel_format(vgfx_pixel_format fmt) final;

	vgfx_buffer   make_buffer(const vgfx_buffer_prop* prop) final;
	vgfx_image    make_image(const vgfx_image_prop* prop) final;
	vgfx_shader   make_shader(const vgfx_shader_prop* prop) final;
	vgfx_pipeline make_pipeline(const vgfx_pipeline_prop* prop) final;
	vgfx_pass     make_pass(const vgfx_pass_prop* prop) final;

	void destroy_buffer(vgfx_buffer buf) final;
	void destroy_image(vgfx_image img) final;
	void destroy_shader(vgfx_shader shd) final;
	void destroy_pipeline(vgfx_pipeline pip) final;
	void destroy_pass(vgfx_pass pass) final;

	void update_buffer(vgfx_buffer buf_id, const void* data_ptr, int data_size) final;
	void update_image(vgfx_image img_id, const vgfx_image_content* data) final;
	int append_buffer(vgfx_buffer buf, const void* data_ptr, int data_size) final;
	bool query_buffer_overflow(vgfx_buffer buf) final;

	void begin_default_pass(const vgfx_pass_action* act, int width, int height) final;
	void begin_pass(vgfx_pass pass_id, const vgfx_pass_action* act) final;
	void apply_viewport(int x, int y, int width, int height, bool origin_top_left) final;
	void apply_scissor_rect(int x, int y, int width, int height, bool origin_top_left) final;
	void apply_bindings(const vgfx_bindings* bindings) final;
	void apply_pipeline(vgfx_pipeline pip_id) final;
	void apply_uniforms(vgfx_shader_stages stage, int ub_index, const void* data, int num_bytes) final;
	void draw(int base_element, int num_elements, int num_instances) final;
	void end_pass() final;
	void commit() final;

	bool is_valid_rendertarget_color_format(vgfx_pixel_format fmt);
	bool is_valid_rendertarget_depth_format(vgfx_pixel_format fmt);

	void warning_exhausted(const char* res);
	void warning_mismatch();
	void fail(const char* object, vgfx_error_type ctx);

	static vp_gfx_impl instance;
	vgfx_prop prop = {};

	vp_app* _app = nullptr;

	vgfx_context _active_context = {};
	vgfx_pass _cur_pass = {};
	vgfx_pipeline _cur_pipeline = {};

	bool _pass_valid = false;
	bool _bindings_valid = false;
	bool _next_draw_valid = false;

	vgfx_backend _backend = {};
	vgfx_features _features = {};
	vgfx_limits _limits = {};
	vgfx_pixelformat_info _formats[VGFX_NUM_PIXEL_FORMAT] = {};

	vgfx_pools _pools = {};
	uint32_t _frame_index = 0;
	bool _valid = false;

private:
	void init_pool(vgfx_pool* pool, int num);
	void setup_pools(vgfx_pools* p, const vgfx_prop* prop);
	int  pool_alloc_index(vgfx_pool* pool);
	void pool_free_index(vgfx_pool* pool, int index);
	void discard_pool(vgfx_pool* pool);
	void discard_pools(vgfx_pools* p);

	uint32_t slot_alloc(vgfx_pool* pool, vgfx_slot* slot, int slot_index);
	int      slot_index(uint32_t id);
	void     destroy_all_resources(vgfx_pools* p, uint32_t ctx_id);

	vgfx_context    setup_context();
	native_context* context_at(const vgfx_pools* p, uint32_t context_id);
	native_context* lookup_context(const vgfx_pools* p, uint32_t ctx_id);

	void init_buffer(vgfx_buffer buf_id, const vgfx_buffer_prop* prop);
	void init_image(vgfx_image img_id, const vgfx_image_prop* prop);
	void init_shader(vgfx_shader shd_id, const vgfx_shader_prop* prop);
	void init_pipeline(vgfx_pipeline pip_id, const vgfx_pipeline_prop* prop);
	void init_pass(vgfx_pass pass_id, const vgfx_pass_prop* prop);

	void reset_buffer(native_buffer* buf);
	void reset_image(native_image* img);
	void reset_shader(native_shader* shd);
	void reset_pipeline(native_pipeline* pip);
	void reset_pass(native_pass* pass);

	vgfx_buffer   alloc_buffer();
	vgfx_image    alloc_image();
	vgfx_shader   alloc_shader();
	vgfx_pipeline alloc_pipeline();
	vgfx_pass     alloc_pass();

	native_buffer*   buffer_at(const vgfx_pools* p, uint32_t buf_id);
	native_image*    image_at(const vgfx_pools* p, uint32_t img_id);
	native_shader*   shader_at(const vgfx_pools* p, uint32_t shd_id);
	native_pipeline* pipeline_at(const vgfx_pools* p, uint32_t pip_id);
	native_pass*     pass_at(const vgfx_pools* p, uint32_t pass_id);

	native_buffer*   lookup_buffer(const vgfx_pools* p, uint32_t buf_id);
	native_image*    lookup_image(const vgfx_pools* p, uint32_t img_id);
	native_shader*   lookup_shader(const vgfx_pools* p, uint32_t shd_id);
	native_pipeline* lookup_pipeline(const vgfx_pools* p, uint32_t pip_id);
	native_pass*     lookup_pass(const vgfx_pools* p, uint32_t pass_id);

	void resolve_default_pass_action(const vgfx_pass_action* from, vgfx_pass_action* to);
};

vp_gfx_impl vp_gfx_impl::instance;

struct vgfx_uniform_block
{
	int size;
};

struct vgfx_shader_image
{
	vgfx_image_type type;
};

struct vgfx_buffer_common
{
	int size;
	int append_pos;
	bool append_overflow;
	vgfx_buffer_type type;
	vgfx_usage usage;
	unsigned update_frame_index;
	unsigned append_frame_index;
	int num_slots;
	int active_slot;
};

struct vgfx_image_common
{
	vgfx_image_type type;
	bool render_target;
	int width;
	int height;
	int depth;
	int num_mipmaps;
	vgfx_usage usage;
	vgfx_pixel_format pixel_format;
	int sample_count;
	vgfx_filter min_filter;
	vgfx_filter mag_filter;
	vgfx_wrap wrap_u;
	vgfx_wrap wrap_v;
	vgfx_wrap wrap_w;
	vgfx_border_color border_color;
	unsigned max_anisotropy;
	unsigned upd_frame_index;
	int num_slots;
	int active_slot;
};

struct vgfx_shader_stage
{
	int num_uniform_blocks;
	int num_images;
	vgfx_uniform_block uniform_blocks[VGFX_MAX_SHADERSTAGE_UBS];
	vgfx_shader_image images[VGFX_MAX_SHADERSTAGE_IMAGES];
};

struct vgfx_shader_common
{
	vgfx_shader_stage stage[VGFX_NUM_SHADER_STAGES];
};

struct vgfx_pipeline_common
{
	vgfx_shader shader_id;
	vgfx_index_type index_type;
	bool vertex_layout_valid[VGFX_MAX_SHADERSTAGE_BUFFERS];
	int color_attachment_count;
	vgfx_pixel_format color_format;
	vgfx_pixel_format depth_format;
	int sample_count;
	float depth_bias;
	float depth_bias_slope_scale;
	float depth_bias_clamp;
	float blend_color[4];
};

struct vgfx_attachment_common
{
	vgfx_image image_id;
	int mip_level;
	int slice;
};

struct pass_common
{
	int num_color_atts;
	vgfx_attachment_common color_atts[VGFX_MAX_COLOR_ATTACHMENTS];
	vgfx_attachment_common ds_att;
};

static constexpr int INVALID_SLOT_INDEX = 0;
static constexpr int INVALID_ID = 0;
static constexpr unsigned SLOT_SHIFT = 16;
static constexpr unsigned SLOT_MASK = (1 << SLOT_SHIFT) - 1;

bool vp_gfx_impl::is_valid_rendertarget_color_format(vgfx_pixel_format fmt)
{
	const int fmt_index = (int)fmt;
	if ((fmt_index < 0) || (fmt_index >= VGFX_NUM_PIXEL_FORMAT)) {
		fail("pixel format", vgfx_error_invalid);
	}
	return _formats[fmt_index].render && !_formats[fmt_index].depth;
}

bool vp_gfx_impl::is_valid_rendertarget_depth_format(vgfx_pixel_format fmt)
{
	const int index = (int)fmt;
	if ((index < 0) || (index >= VGFX_NUM_PIXEL_FORMAT)) {
		fail("pixel format", vgfx_error_invalid);
	}
	return _formats[index].render && _formats[index].depth;
}

static void buffer_common_init(vgfx_buffer_common* cmn, const vgfx_buffer_prop* prop)
{
	memset(cmn, 0, sizeof(vgfx_buffer_common));
	cmn->size = prop->size;
	cmn->append_pos = 0;
	cmn->append_overflow = false;
	cmn->type = prop->type;
	cmn->usage = prop->usage;
	cmn->update_frame_index = 0;
	cmn->append_frame_index = 0;
	cmn->num_slots = (cmn->usage == vgfx_usage_immutable) ? 1 : 2;
	cmn->active_slot = 0;
}

void image_common_init(vgfx_image_common* cmn, const vgfx_image_prop* prop)
{
	memset(cmn, 0, sizeof(vgfx_image_common));
	cmn->type = prop->type;
	cmn->render_target = prop->render_target;
	cmn->width = prop->width;
	cmn->height = prop->height;
	cmn->depth = prop->depth;
	cmn->num_mipmaps = prop->num_mipmaps;
	cmn->usage = prop->usage;
	cmn->pixel_format = prop->pixel_format;
	cmn->sample_count = prop->sample_count;
	cmn->min_filter = prop->min_filter;
	cmn->mag_filter = prop->mag_filter;
	cmn->wrap_u = prop->wrap_u;
	cmn->wrap_v = prop->wrap_v;
	cmn->wrap_w = prop->wrap_w;
	cmn->border_color = prop->border_color;
	cmn->max_anisotropy = prop->max_anisotropy;
	cmn->upd_frame_index = 0;
	cmn->num_slots = (cmn->usage == vgfx_usage_immutable) ? 1 : 2;
	cmn->active_slot = 0;
}

static void shader_common_init(vgfx_shader_common* cmn, const vgfx_shader_prop* prop)
{
	memset(cmn, 0, sizeof(vgfx_shader_common));
	for (int i = 0; i < VGFX_NUM_SHADER_STAGES; i++) {
		const vgfx_shader_stage_prop* stage_prop = (i == vgfx_shader_stage_vs) ? &prop->vs : &prop->fs;
		vgfx_shader_stage* stage = &cmn->stage[i];
		for (int ui = 0; ui < VGFX_MAX_SHADERSTAGE_UBS; ui++) {
			const vgfx_shader_uniform_block_prop* ub_prop = &stage_prop->uniform_blocks[ui];
			if (ub_prop->size == 0) {
				break;
			}
			stage->uniform_blocks[ui].size = ub_prop->size;
			stage->num_uniform_blocks++;
		}
		for (int idx = 0; idx < VGFX_MAX_SHADERSTAGE_IMAGES; idx++) {
			const vgfx_shader_image_prop* img_prop = &stage_prop->images[idx];
			if (img_prop->type == vgfx_image_type_default) {
				break;
			}
			stage->images[idx].type = img_prop->type;
			stage->num_images++;
		}
	}
}

static void pipeline_common_init(vgfx_pipeline_common* cmn, const vgfx_pipeline_prop* prop)
{
	memset(cmn, 0, sizeof(vgfx_pipeline_common));
	cmn->shader_id = prop->shader;
	cmn->index_type = prop->index_type;
	for (int i = 0; i < VGFX_MAX_SHADERSTAGE_BUFFERS; i++) {
		cmn->vertex_layout_valid[i] = false;
	}
	cmn->color_attachment_count = prop->blend.color_attachment_count;
	cmn->color_format = prop->blend.color_format;
	cmn->depth_format = prop->blend.depth_format;
	cmn->sample_count = prop->rasterizer.sample_count;
	cmn->depth_bias = prop->rasterizer.depth_bias;
	cmn->depth_bias_slope_scale = prop->rasterizer.depth_bias_slope_scale;
	cmn->depth_bias_clamp = prop->rasterizer.depth_bias_clamp;
	for (int i = 0; i < 4; i++) {
		cmn->blend_color[i] = prop->blend.blend_color[i];
	}
}

static void pass_common_init(pass_common* cmn, const vgfx_pass_prop* prop)
{
	memset(cmn, 0, sizeof(pass_common));

	const vgfx_attachment_prop* att_prop = nullptr;
	vgfx_attachment_common* att = nullptr;

	for (int i = 0; i < VGFX_MAX_COLOR_ATTACHMENTS; i++) {
		att_prop = &prop->color_attachments[i];
		if (att_prop->image.id != INVALID_ID) {
			cmn->num_color_atts++;
			att = &cmn->color_atts[i];
			att->image_id = att_prop->image;
			att->mip_level = att_prop->mip_level;
			att->slice = att_prop->slice;
		}
	}

	att_prop = &prop->depth_stencil_attachment;
	if (att_prop->image.id != INVALID_ID) {
		att = &cmn->ds_att;
		att->image_id = att_prop->image;
		att->mip_level = att_prop->mip_level;
		att->slice = att_prop->slice;
	}
}

static int pixelformat_bytesize(vgfx_pixel_format fmt)
{
	switch (fmt) {
		case vgfx_pixel_format_r8:
		case vgfx_pixel_format_r8sn:
		case vgfx_pixel_format_r8ui:
		case vgfx_pixel_format_r8si:
			return 1;
		case vgfx_pixel_format_r16:
		case vgfx_pixel_format_r16sn:
		case vgfx_pixel_format_r16ui:
		case vgfx_pixel_format_r16si:
		case vgfx_pixel_format_r16f:
		case vgfx_pixel_format_rg8:
		case vgfx_pixel_format_rg8sn:
		case vgfx_pixel_format_rg8ui:
		case vgfx_pixel_format_rg8si:
			return 2;
		case vgfx_pixel_format_r32ui:
		case vgfx_pixel_format_r32si:
		case vgfx_pixel_format_r32f:
		case vgfx_pixel_format_rg16:
		case vgfx_pixel_format_rg16sn:
		case vgfx_pixel_format_rg16ui:
		case vgfx_pixel_format_rg16si:
		case vgfx_pixel_format_rg16f:
		case vgfx_pixel_format_rgba8:
		case vgfx_pixel_format_rgba8sn:
		case vgfx_pixel_format_rgba8ui:
		case vgfx_pixel_format_rgba8si:
		case vgfx_pixel_format_bgra8:
		case vgfx_pixel_format_rgb10a2:
		case vgfx_pixel_format_rg11b10f:
			return 4;
		case vgfx_pixel_format_rg32ui:
		case vgfx_pixel_format_rg32si:
		case vgfx_pixel_format_rg32f:
		case vgfx_pixel_format_rgba16:
		case vgfx_pixel_format_rgba16sn:
		case vgfx_pixel_format_rgba16ui:
		case vgfx_pixel_format_rgba16si:
		case vgfx_pixel_format_rgba16f:
			return 8;
		case vgfx_pixel_format_rgba32ui:
		case vgfx_pixel_format_rgba32si:
		case vgfx_pixel_format_rgba32f:
			return 16;
	}
	return 0;
}

static int vertexformat_bytesize(vgfx_vertex_format fmt)
{
	switch (fmt) {
		case vgfx_vertex_format_float:    return 4;
		case vgfx_vertex_format_float2:   return 8;
		case vgfx_vertex_format_float3:   return 12;
		case vgfx_vertex_format_float4:   return 16;
		case vgfx_vertex_format_byte4:    return 4;
		case vgfx_vertex_format_byte4n:   return 4;
		case vgfx_vertex_format_ubyte4:   return 4;
		case vgfx_vertex_format_ubyte4n:  return 4;
		case vgfx_vertex_format_short2:   return 4;
		case vgfx_vertex_format_short2n:  return 4;
		case vgfx_vertex_format_ushort2n: return 4;
		case vgfx_vertex_format_short4:   return 8;
		case vgfx_vertex_format_short4n:  return 8;
		case vgfx_vertex_format_ushort4n: return 8;
		case vgfx_vertex_format_uint10n2: return 4;
		case vgfx_vertex_format_invalid:  return 0;
		default:
			return -1;
	}
}

static int row_pitch(vgfx_pixel_format fmt, int width)
{
	int pitch;
	switch (fmt) {
		case vgfx_pixel_format_bc1_rgba:
		case vgfx_pixel_format_bc4_r:
		case vgfx_pixel_format_bc4_rsn:
		case vgfx_pixel_format_etc2_rgb8:
		case vgfx_pixel_format_etc2_rgb8a1:
			pitch = ((width + 3) / 4) * 8;
			pitch = pitch < 8 ? 8 : pitch;
			break;
		case vgfx_pixel_format_bc2_rgba:
		case vgfx_pixel_format_bc3_rgba:
		case vgfx_pixel_format_bc5_rg:
		case vgfx_pixel_format_bc5_rgsn:
		case vgfx_pixel_format_bc6h_rgbf:
		case vgfx_pixel_format_bc6h_rgbuf:
		case vgfx_pixel_format_bc7_rgba:
		case vgfx_pixel_format_etc2_rgba8:
		case vgfx_pixel_format_etc2_rg11:
		case vgfx_pixel_format_etc2_rg11sn:
			pitch = ((width + 3) / 4) * 16;
			pitch = pitch < 16 ? 16 : pitch;
			break;
		case vgfx_pixel_format_pvrtc_rgb_4bpp:
		case vgfx_pixel_format_pvrtc_rgba_4bpp: {
			const int block_size = 16;
			const int bpp = 4;
			int width_blocks = width / 4;
			width_blocks = width_blocks < 2 ? 2 : width_blocks;
			pitch = width_blocks * ((block_size * bpp) / 8);
			break;
		}
		case vgfx_pixel_format_pvrtc_rgb_2bpp:
		case vgfx_pixel_format_pvrtc_rgba_2bpp: {
			const int block_size = 32;
			const int bpp = 2;
			int width_blocks = width / 4;
			width_blocks = width_blocks < 2 ? 2 : width_blocks;
			pitch = width_blocks * ((block_size * bpp) / 8);
			break;
		}
		default:
			pitch = width * pixelformat_bytesize(fmt);
			break;
	}
	return pitch;
}

static int surface_pitch(vgfx_pixel_format fmt, int width, int height)
{
	int num_rows = 0;
	switch (fmt) {
		case vgfx_pixel_format_bc1_rgba:
		case vgfx_pixel_format_bc4_r:
		case vgfx_pixel_format_bc4_rsn:
		case vgfx_pixel_format_etc2_rgb8:
		case vgfx_pixel_format_etc2_rgb8a1:
		case vgfx_pixel_format_etc2_rgba8:
		case vgfx_pixel_format_etc2_rg11:
		case vgfx_pixel_format_etc2_rg11sn:
		case vgfx_pixel_format_bc2_rgba:
		case vgfx_pixel_format_bc3_rgba:
		case vgfx_pixel_format_bc5_rg:
		case vgfx_pixel_format_bc5_rgsn:
		case vgfx_pixel_format_bc6h_rgbf:
		case vgfx_pixel_format_bc6h_rgbuf:
		case vgfx_pixel_format_bc7_rgba:
		case vgfx_pixel_format_pvrtc_rgb_4bpp:
		case vgfx_pixel_format_pvrtc_rgba_4bpp:
		case vgfx_pixel_format_pvrtc_rgb_2bpp:
		case vgfx_pixel_format_pvrtc_rgba_2bpp:
			num_rows = ((height + 3) / 4);
			break;
		default:
			num_rows = height;
			break;
	}
	if (num_rows < 1) {
		num_rows = 1;
	}
	return num_rows * row_pitch(fmt, width);
}

void vp_gfx_impl::warning_exhausted(const char* res)
{
	char outbuf[256];
#ifdef _MSC_VER
	sprintf_s(outbuf, "%s pool exhausted", res);
#else
	sprintf(outbuf, "%s pool exhausted", res);
#endif
	_app->log(vapp_log_message_type_warning, outbuf);
}

void vp_gfx_impl::warning_mismatch()
{
	_app->log(vapp_log_message_type_warning, "Active context mismatch (must be same as for creation)");
}

void vp_gfx_impl::fail(const char* object, vgfx_error_type ctx)
{
	const char* ex_title = "ViperGFX exception";
	const char* err_ctx = "";
	char outbuf[256];

	switch (ctx) {
		case vgfx_error_unallocated:
			err_ctx = "unallocated";
			break;
		case vgfx_error_uninitialized:
			err_ctx = "uninitialized";
			break;
		case vgfx_error_create_failed:
			err_ctx = "creation failed";
			break;
	}

#ifdef _MSC_VER
	if (ctx == vgfx_error_invalid) {
		sprintf_s(outbuf, "%s, invalid %s", ex_title, object);
	} else if (ctx == vgfx_error_create_failed) {
		sprintf_s(outbuf, "%s, %s creation failed", ex_title, object);
	} else if (ctx == vgfx_error_any) {
		sprintf_s(outbuf, "%s, %s", ex_title, object);
	} else {
		sprintf_s(outbuf, "%s, %s was %s", ex_title, object, err_ctx);
	}
#else
	if (ctx == vgfx_error_invalid) {
		sprintf(outbuf, "%s, invalid %s", ex_title, object);
	} else if (ctx == vgfx_error_create_failed) {
		sprintf(outbuf, "%s, %s creation failed", ex_title, object);
	} else if (ctx == vgfx_error_any) {
		sprintf(outbuf, "%s, %s", object);
	} else {
		sprintf(outbuf, "%s, %s was %s", ex_title, object, err_ctx);
	}
#endif
	_app->fail(outbuf);
}

#pragma region Resource Initialization and Native Handlings
#ifdef VP_APP_D3D11_BACKEND
#include "_priv/vp_gfx_d3d11.h"
#endif

vp_gfx_impl::vp_gfx_impl(const vgfx_prop* prop, vp_app* app)
{
	this->_app = app;

	this->prop = *prop;
	this->prop.buffer_pool_size = (prop->buffer_pool_size != 0) ? prop->buffer_pool_size : 128;
	this->prop.image_pool_size = (prop->image_pool_size != 0) ? prop->image_pool_size : 128;
	this->prop.shader_pool_size = (prop->shader_pool_size != 0) ? prop->shader_pool_size : 32;
	this->prop.pipeline_pool_size = (prop->pipeline_pool_size != 0) ? prop->pipeline_pool_size : 64;
	this->prop.pass_pool_size = (prop->pass_pool_size != 0) ? prop->pass_pool_size : 16;
	this->prop.context_pool_size = (prop->context_pool_size != 0) ? prop->context_pool_size : 16;

	setup_pools(&_pools, &this->prop);
	_frame_index = 1;

#if VP_APP_D3D11_BACKEND
	d3d11_setup_backend(&this->prop);
#endif

	_valid = true;
	setup_context();
}

void vp_gfx_impl::shutdown()
{
	if (_active_context.id != INVALID_ID) {
		native_context* ctx = lookup_context(&_pools, _active_context.id);
		if (ctx) {
			destroy_all_resources(&_pools, _active_context.id);
#if VP_APP_D3D11_BACKEND
			d3d11_destroy_context(ctx);
#endif
		}
	}
#if VP_APP_D3D11_BACKEND
	d3d11_discard_backend();
#endif
	discard_pools(&_pools);
	_valid = false;
}

void vp_gfx_impl::init_pool(vgfx_pool* pool, int num)
{
	if (!pool) {
		fail("pool", vgfx_error_uninitialized);
	}
	if (num < 1) {
		fail("pool size", vgfx_error_invalid);
	}
	pool->size = num + 1;
	pool->queue_top = 0;
	size_t gen_ctrs_size = sizeof(uint32_t) * pool->size;
	pool->gen_ctrs = (uint32_t*)malloc(gen_ctrs_size);
	if (!pool->gen_ctrs) {
		fail("pool gen ctrs", vgfx_error_unallocated);
	}
	memset(pool->gen_ctrs, 0, gen_ctrs_size);
	pool->free_queue = (int*)malloc(sizeof(int) * num);
	if (!pool->free_queue) {
		fail("pool free queue", vgfx_error_unallocated);
	}
	for (int i = pool->size - 1; i >= 1; i--) {
		pool->free_queue[pool->queue_top++] = i;
	}
}

void vp_gfx_impl::setup_pools(vgfx_pools* p, const vgfx_prop* prop)
{
	if (!p) {
		fail("pools", vgfx_error_uninitialized);
	}
	if (!prop) {
		fail("ViperGFX prop", vgfx_error_uninitialized);
	}

	if ((prop->buffer_pool_size <= 0) || (prop->buffer_pool_size >= MAX_POOL_SIZE)) {
		fail("buffer pool size", vgfx_error_invalid);
	} 
	init_pool(&p->buffer_pool, prop->buffer_pool_size);
	size_t buffer_pool_size = sizeof(native_buffer) * p->buffer_pool.size;
	p->buffers = (native_buffer*)malloc(buffer_pool_size);
	if (!p->buffers) {
		fail("pool buffers", vgfx_error_unallocated);
	}
	memset(p->buffers, 0, buffer_pool_size);

	if ((prop->image_pool_size <= 0) || (prop->image_pool_size >= MAX_POOL_SIZE)) {
		fail("image pool size", vgfx_error_invalid);
	}
	init_pool(&p->image_pool, prop->image_pool_size);
	size_t image_pool_size = sizeof(native_image) * p->image_pool.size;
	p->images = (native_image*)malloc(image_pool_size);
	if (!p->images) {
		fail("pool images", vgfx_error_unallocated);
	}
	memset(p->images, 0, image_pool_size);

	if ((prop->shader_pool_size <= 0) || (prop->shader_pool_size >= MAX_POOL_SIZE)) {
		fail("shader pool size", vgfx_error_invalid);
	}
	init_pool(&p->shader_pool, prop->shader_pool_size);
	size_t shader_pool_size = sizeof(native_shader) * p->shader_pool.size;
	p->shaders = (native_shader*)malloc(shader_pool_size);
	if (!p->shaders) {
		fail("pool shaders", vgfx_error_unallocated);
	}
	memset(p->shaders, 0, shader_pool_size);

	if ((prop->pipeline_pool_size <= 0) || (prop->pipeline_pool_size >= MAX_POOL_SIZE)) {
		fail("pipeline pool size", vgfx_error_invalid);
	}
	init_pool(&p->pipeline_pool, prop->pipeline_pool_size);
	size_t pipeline_pool_size = sizeof(native_pipeline) * p->pipeline_pool.size;
	p->pipelines = (native_pipeline*)malloc(pipeline_pool_size);
	if (!p->pipelines) {
		fail("pool pipelines", vgfx_error_unallocated);
	}
	memset(p->pipelines, 0, pipeline_pool_size);

	if ((prop->pass_pool_size <= 0) || (prop->pass_pool_size >= MAX_POOL_SIZE)) {
		fail("pass pool size", vgfx_error_invalid);
	}
	init_pool(&p->pass_pool, prop->pass_pool_size);
	size_t pass_pool_size = sizeof(native_pass) * p->pass_pool.size;
	p->passes = (native_pass*)malloc(pass_pool_size);
	if (!p->passes) {
		fail("pool passes", vgfx_error_unallocated);
	}
	memset(p->passes, 0, pass_pool_size);

	if ((prop->context_pool_size <= 0) || (prop->context_pool_size >= MAX_POOL_SIZE)) {
		fail("context pool size", vgfx_error_invalid);
	}
	init_pool(&p->context_pool, prop->context_pool_size);
	size_t context_pool_size = sizeof(native_context) * p->context_pool.size;
	p->contexts = (native_context*)malloc(context_pool_size);
	if (!p->contexts) {
		fail("pool contexts", vgfx_error_unallocated);
	}
	memset(p->contexts, 0, context_pool_size);
}

int vp_gfx_impl::pool_alloc_index(vgfx_pool* pool)
{
	if (!pool) {
		fail("pool", vgfx_error_uninitialized);
	}
	if (!pool->free_queue) {
		fail("pool free queue", vgfx_error_uninitialized);
	}
	if (pool->queue_top > 0) {
		int slot_index = pool->free_queue[--pool->queue_top];
		if ((slot_index <= 0) || (slot_index >= pool->size)) {
			fail("slot index", vgfx_error_invalid);
		}
		return slot_index;
	}
	return INVALID_SLOT_INDEX;
}

void vp_gfx_impl::pool_free_index(vgfx_pool* pool, int index)
{
	if ((index <= INVALID_SLOT_INDEX) || (index >= pool->size)) {
		fail("index", vgfx_error_invalid);
	}
	if (!pool) {
		fail("pool", vgfx_error_uninitialized);
	}
	if (!pool->free_queue) {
		fail("pool free queue", vgfx_error_uninitialized);
	}
	if (pool->queue_top >= pool->size) {
		fail("pool queue top", vgfx_error_invalid);
	}
#ifdef _DEBUG
	for (int i = 0; i < pool->queue_top; i++) {
		if (pool->free_queue[i] == index) {
			fail("double free occurred", vgfx_error_any);
		}
	}
#endif
	pool->free_queue[pool->queue_top++] = index;
	if (pool->queue_top > (pool->size - 1)) {
		fail("pool queue top", vgfx_error_invalid);
	}
}

uint32_t vp_gfx_impl::slot_alloc(vgfx_pool* pool, vgfx_slot* slot, int slot_index)
{
	if (!pool) {
		fail("pool", vgfx_error_uninitialized);
	}
	if (!pool->gen_ctrs) {
		fail("pool gen ctrs", vgfx_error_uninitialized);
	}
	if ((slot_index <= INVALID_SLOT_INDEX) || (slot_index >= pool->size)) {
		fail("slot index", vgfx_error_invalid);
	}
	if (slot->state != vgfx_resource_state_initial) {
		fail("slot resource state", vgfx_error_invalid);
	}
	if (slot->id != INVALID_ID) {
		fail("slot resource id", vgfx_error_invalid);
	}
	uint32_t ctr = ++pool->gen_ctrs[slot_index];
	slot->id = (ctr << SLOT_SHIFT) | (slot_index & SLOT_MASK);
	slot->state = vgfx_resource_state_alloc;
	return slot->id;		
}

int vp_gfx_impl::slot_index(uint32_t id)
{
	int index = (int)(id & SLOT_MASK);
	if (index == INVALID_SLOT_INDEX) {
		fail("index", vgfx_error_invalid);
	}
	return index;
}

void vp_gfx_impl::destroy_all_resources(vgfx_pools* p, uint32_t ctx_id)
{
	for (int i = 1; i < p->buffer_pool.size; i++) {
		if (p->buffers[i].slot.ctx_id == ctx_id) {
			vgfx_resource_state state = p->buffers[i].slot.state;
			if ((state == vgfx_resource_state_valid) || (state == vgfx_resource_state_failed)) {
#if VP_APP_D3D11_BACKEND
				d3d11_destroy_buffer(&p->buffers[i]);
#endif
			}
		}
	}
	for (int i = 1; i < p->image_pool.size; i++) {
		if (p->images[i].slot.ctx_id == ctx_id) {
			vgfx_resource_state state = p->images[i].slot.state;
			if ((state == vgfx_resource_state_valid) || (state == vgfx_resource_state_failed)) {
#if VP_APP_D3D11_BACKEND
				d3d11_destroy_image(&p->images[i]);
#endif
			}
		}
	}
	for (int i = 1; i < p->shader_pool.size; i++) {
		if (p->shaders[i].slot.ctx_id == ctx_id) {
			vgfx_resource_state state = p->shaders[i].slot.state;
			if ((state == vgfx_resource_state_valid) || (state == vgfx_resource_state_failed)) {
#if VP_APP_D3D11_BACKEND
				d3d11_destroy_shader(&p->shaders[i]);
#endif
			}
		}
	}
	for (int i = 1; i < p->pipeline_pool.size; i++) {
		if (p->pipelines[i].slot.ctx_id == ctx_id) {
			vgfx_resource_state state = p->pipelines[i].slot.state;
			if ((state == vgfx_resource_state_valid) || (state == vgfx_resource_state_failed)) {
#if VP_APP_D3D11_BACKEND
				d3d11_destroy_pipeline(&p->pipelines[i]);
#endif
			}
		}
	}
	for (int i = 1; i < p->pass_pool.size; i++) {
		if (p->passes[i].slot.ctx_id == ctx_id) {
			vgfx_resource_state state = p->passes[i].slot.state;
			if ((state == vgfx_resource_state_valid) || (state == vgfx_resource_state_failed)) {
#if VP_APP_D3D11_BACKEND
				d3d11_destroy_pass(&p->passes[i]);
#endif
			}
		}
	}
}

void vp_gfx_impl::discard_pool(vgfx_pool* pool)
{
	if (!pool) {
		fail("pool", vgfx_error_uninitialized);
	}
	if (!pool->free_queue) {
		fail("pool free queue", vgfx_error_uninitialized);
	}
	free(pool->free_queue); 
	pool->free_queue = nullptr;
	if (!pool->gen_ctrs) {
		fail("pool gen ctrs", vgfx_error_uninitialized);
	} 
	free(pool->gen_ctrs);   
	pool->gen_ctrs = nullptr;
	pool->size = 0;
	pool->queue_top = 0;
}

void vp_gfx_impl::discard_pools(vgfx_pools* p)
{
	if (!p) {
		fail("pools", vgfx_error_uninitialized);
	}
	free(p->contexts);  p->contexts = nullptr;
	free(p->passes);    p->passes = nullptr;
	free(p->pipelines); p->pipelines = nullptr;
	free(p->shaders);   p->shaders = nullptr;
	free(p->images);    p->images = nullptr;
	free(p->buffers);   p->buffers = nullptr;

	discard_pool(&p->context_pool);
	discard_pool(&p->pass_pool);
	discard_pool(&p->pipeline_pool);
	discard_pool(&p->shader_pool);
	discard_pool(&p->image_pool);
	discard_pool(&p->buffer_pool);
}

vgfx_prop vp_gfx_impl::query_props()
{
	if (!_valid) {
		fail(VGFX_GLOBAL_OBJECT, vgfx_error_uninitialized);
	}
	return this->prop;
}

vgfx_backend vp_gfx_impl::query_backend()
{
	if (!_valid) {
		fail(VGFX_GLOBAL_OBJECT, vgfx_error_uninitialized);
	}
	return _backend;
}

vgfx_features vp_gfx_impl::query_features()
{
	if (!_valid) {
		fail(VGFX_GLOBAL_OBJECT, vgfx_error_uninitialized);
	}
	return _features;
}

vgfx_limits vp_gfx_impl::query_limits()
{
	if (!_valid) {
		fail(VGFX_GLOBAL_OBJECT, vgfx_error_uninitialized);
	}
	return _limits;
}

vgfx_pixelformat_info vp_gfx_impl::query_pixel_format(vgfx_pixel_format fmt)
{
	if (!_valid) {
		fail(VGFX_GLOBAL_OBJECT, vgfx_error_uninitialized);
	}
	int fmt_index = (int)fmt;
	if ((fmt_index <= vgfx_pixel_format_none) || (fmt_index >= VGFX_NUM_PIXEL_FORMAT)) {
		fail("Invalid pixel format", vgfx_error_any);
	}
	return _formats[fmt_index];
}

native_context* vp_gfx_impl::context_at(const vgfx_pools* p, uint32_t context_id)
{
	if (!p) {
		fail("pools", vgfx_error_uninitialized);
	}
	if (context_id == INVALID_ID) {
		fail("context id", vgfx_error_invalid);
	}
	int index = slot_index(context_id);
	if ((index <= INVALID_SLOT_INDEX) || (index >= p->context_pool.size)) {
		fail("index", vgfx_error_invalid);
	}
	return &p->contexts[index];
}

native_context* vp_gfx_impl::lookup_context(const vgfx_pools* p, uint32_t ctx_id)
{
	if (!p) {
		fail("pools", vgfx_error_uninitialized);
	}
	if (ctx_id != INVALID_ID) {
		native_context* ctx = context_at(p, ctx_id);
		if (ctx->slot.id == ctx_id) {
			return ctx;
		}
	}
	return nullptr;
}

vgfx_context vp_gfx_impl::setup_context()
{
	if (!_valid) {
		fail(VGFX_GLOBAL_OBJECT, vgfx_error_uninitialized);
	}
	vgfx_context res = {};
	int slot_index = pool_alloc_index(&_pools.context_pool);
	if (slot_index != INVALID_SLOT_INDEX) {
		res.id = slot_alloc(&_pools.context_pool, &_pools.contexts[slot_index].slot, slot_index);
		native_context* ctx = context_at(&_pools, res.id);
#if VP_APP_D3D11_BACKEND
		ctx->slot.state = d3d11_create_context(ctx);
#endif
		if (ctx->slot.state != vgfx_resource_state_valid) {
			fail("slot state", vgfx_error_invalid);
		}
#if VP_APP_D3D11_BACKEND
		d3d11_activate_context(ctx);
#endif
	} else {
		res.id = INVALID_ID;
	}
	_active_context = res;
	return res;
}

static vgfx_buffer_prop buffer_prop_defaults(const vgfx_buffer_prop* prop)
{
	vgfx_buffer_prop def = *prop;
	def.type = (def.type != vgfx_buffer_type_default) ? def.type : vgfx_buffer_type_vertex_buffer;
	def.usage = (def.usage != vgfx_usage_default) ? def.usage : vgfx_usage_immutable;
	return def;
}

vgfx_buffer vp_gfx_impl::alloc_buffer()
{
	vgfx_buffer res = {};
	int slot_index = pool_alloc_index(&_pools.buffer_pool);
	if (slot_index != INVALID_SLOT_INDEX) {
		res.id = slot_alloc(&_pools.buffer_pool, &_pools.buffers[slot_index].slot, slot_index);
	} else {
		res.id = INVALID_ID;
	}
	return res;
}

native_buffer* vp_gfx_impl::buffer_at(const vgfx_pools* p, uint32_t buf_id)
{
	if (!p) {
		fail("pools", vgfx_error_uninitialized);
	}
	if (buf_id == INVALID_ID) {
		fail("buffer id", vgfx_error_invalid);
	}
	int index = slot_index(buf_id);
	if ((index <= INVALID_SLOT_INDEX) || (index >= p->buffer_pool.size)) {
		fail("buffer index", vgfx_error_invalid);
	}
	return &p->buffers[index];
}

native_buffer* vp_gfx_impl::lookup_buffer(const vgfx_pools* p, uint32_t buf_id)
{
	if (!p) {
		fail("pools", vgfx_error_unallocated);
	}
	if (buf_id != INVALID_ID) {
		native_buffer* buf = buffer_at(p, buf_id);
		if (buf->slot.id == buf_id) {
			return buf;
		}
	}
	return nullptr;
}

void vp_gfx_impl::init_buffer(vgfx_buffer buf_id, const vgfx_buffer_prop* prop)
{
	if (!prop) {
		fail("buffer prop", vgfx_error_uninitialized);
	}
	if (buf_id.id == INVALID_ID) {
		fail("buffer id", vgfx_error_invalid);
	}
	native_buffer* buf = lookup_buffer(&_pools, buf_id.id);
	if (!buf) {
		fail("buffer", vgfx_error_unallocated);
	}
	if (buf->slot.state != vgfx_resource_state_alloc) {
		fail("buffer resource state", vgfx_error_invalid);
	}
	buf->slot.ctx_id = _active_context.id;
#if VP_APP_D3D11_BACKEND
	buf->slot.state = d3d11_create_buffer(buf, prop);
#endif
	if ((buf->slot.state != vgfx_resource_state_valid) && (buf->slot.state != vgfx_resource_state_failed)) {
		fail("buffer", vgfx_error_create_failed);
	}
}

vgfx_buffer vp_gfx_impl::make_buffer(const vgfx_buffer_prop* prop)
{
	if (!_valid) {
		fail(VGFX_GLOBAL_OBJECT, vgfx_error_uninitialized);
	}
	if (!prop) {
		fail("buffer prop", vgfx_error_uninitialized);
	}
	vgfx_buffer_prop prop_def = buffer_prop_defaults(prop);
	vgfx_buffer buf_id = alloc_buffer();
	if (buf_id.id != INVALID_ID) {
		init_buffer(buf_id, &prop_def);
	} else {
		warning_exhausted("Buffer");
	}
	return buf_id;
}

void vp_gfx_impl::reset_buffer(native_buffer* buf)
{
	if (!buf) {
		fail("buffer", vgfx_error_uninitialized);
	}
	memset(buf, 0, sizeof(native_buffer));
}

void vp_gfx_impl::destroy_buffer(vgfx_buffer buf_id)
{
	if (!_valid) {
		fail(VGFX_GLOBAL_OBJECT, vgfx_error_uninitialized);
	}
	native_buffer* buf = lookup_buffer(&_pools, buf_id.id);
	if (buf) {
		if (buf->slot.ctx_id == _active_context.id) {
#if VP_APP_D3D11_BACKEND
			d3d11_destroy_buffer(buf);
#endif
			reset_buffer(buf);
			pool_free_index(&_pools.buffer_pool, slot_index(buf_id.id));
		} else {
			warning_mismatch();
		}
	}
}

static vgfx_image_prop image_prop_defaults(const vgfx_image_prop* prop)
{
	vgfx_image_prop def = *prop;

	def.type = (def.type != vgfx_image_type_default) ? def.type : vgfx_image_type_2d;
	def.depth = (def.depth != 0) ? def.depth : 1;
	def.num_mipmaps = (def.num_mipmaps != 0) ? def.num_mipmaps : 1;
	def.usage = (def.usage != vgfx_usage_default) ? def.usage : vgfx_usage_immutable;
	if (prop->render_target) {
#if VP_APP_D3D11_BACKEND
		def.pixel_format = (def.pixel_format != vgfx_pixel_format_default) ? def.pixel_format : vgfx_pixel_format_bgra8;
#else 
		def.pixel_format = (def.pixel_format != vgfx_pixel_format_default) ? def.pixel_format : vgfx_pixel_format_rgba8;
#endif
	} else {
		def.pixel_format = (def.pixel_format != vgfx_pixel_format_default) ? def.pixel_format : vgfx_pixel_format_rgba8;
	}

	def.sample_count = (def.sample_count != 0) ? def.sample_count : 1;
	def.min_filter = (def.min_filter != vgfx_filter_default) ? def.min_filter : vgfx_filter_nearest;
	def.mag_filter = (def.mag_filter != vgfx_filter_default) ? def.mag_filter : vgfx_filter_nearest;
	def.wrap_u = (def.wrap_u != vgfx_wrap_default) ? def.wrap_u : vgfx_wrap_repeat;
	def.wrap_v = (def.wrap_v != vgfx_wrap_default) ? def.wrap_v : vgfx_wrap_repeat;
	def.wrap_w = (def.wrap_w != vgfx_wrap_default) ? def.wrap_w : vgfx_wrap_repeat;
	def.border_color = (def.border_color != vgfx_border_color_default) ? def.border_color : vgfx_border_color_opaque_black;
	def.max_anisotropy = (def.max_anisotropy) ? def.max_anisotropy : true;
	def.max_lod = (def.max_lod != 0.0f) ? def.max_lod : FLT_MAX;

	return def;
}

vgfx_image vp_gfx_impl::alloc_image()
{
	vgfx_image res = {};
	int index = pool_alloc_index(&_pools.image_pool);
	if (index != INVALID_SLOT_INDEX) {
		res.id = slot_alloc(&_pools.image_pool, &_pools.images[index].slot, index);
	} else {
		res.id = INVALID_ID;
	}
	return res;
}

native_image* vp_gfx_impl::image_at(const vgfx_pools* p, uint32_t img_id)
{
	if (!p) {
		fail("pools", vgfx_error_uninitialized);
	}
	if (img_id == INVALID_ID) {
		fail("image index", vgfx_error_invalid);
	}
	int index = slot_index(img_id);
	if ((index <= INVALID_SLOT_INDEX) || (index >= p->image_pool.size)) {
		fail("index", vgfx_error_invalid);
	}
	return &p->images[index];
}

native_image* vp_gfx_impl::lookup_image(const vgfx_pools* p, uint32_t img_id)
{
	if (!p) {
		fail("pools", vgfx_error_uninitialized);
	}
	if (img_id != INVALID_ID) {
		native_image* img = image_at(p, img_id);
		if (img->slot.id == img_id) {
			return img;
		}
	}
	return nullptr;
}

void vp_gfx_impl::init_image(vgfx_image img_id, const vgfx_image_prop* prop)
{
	if (!prop) {
		fail("image prop", vgfx_error_uninitialized);
	}
	if (img_id.id == INVALID_ID) {
		fail("image index", vgfx_error_invalid);
	}
	native_image* img = lookup_image(&_pools, img_id.id);
	if (!img || img->slot.state != vgfx_resource_state_alloc) {
		fail("image", vgfx_error_unallocated);
	}
	img->slot.ctx_id = _active_context.id;
#if VP_APP_D3D11_BACKEND
	img->slot.state = d3d11_create_image(img, prop);
#endif
	if ((img->slot.state != vgfx_resource_state_valid) && (img->slot.state != vgfx_resource_state_failed)) {
		fail("image", vgfx_error_create_failed);
	}
}

vgfx_image vp_gfx_impl::make_image(const vgfx_image_prop* prop)
{
	if (!_valid || !prop) {
		fail("image prop", vgfx_error_uninitialized);
	}
	vgfx_image_prop prop_def = image_prop_defaults(prop);
	vgfx_image img_id = alloc_image();
	if (img_id.id != INVALID_ID) {
		init_image(img_id, &prop_def);
	} else {
		warning_exhausted("Image");
	}
	return img_id;
}

void vp_gfx_impl::reset_image(native_image* img)
{
	if (!img) {
		fail("image", vgfx_error_uninitialized);
	}
	memset(img, 0, sizeof(native_image));
}

void vp_gfx_impl::destroy_image(vgfx_image img_id)
{
	if (!_valid) {
		fail(VGFX_GLOBAL_OBJECT, vgfx_error_uninitialized);
	}
	native_image* img = lookup_image(&_pools, img_id.id);
	if (img) {
		if (img->slot.ctx_id == _active_context.id) {
#if VP_APP_D3D11_BACKEND
			d3d11_destroy_image(img);
#endif
			reset_image(img);
			pool_free_index(&_pools.image_pool, slot_index(img_id.id));
		} else {
			warning_mismatch();
		}
	}
}

static vgfx_shader_prop shader_prop_defaults(const vgfx_shader_prop* prop)
{
	vgfx_shader_prop def = *prop;

	def.vs.entry = (def.vs.entry) ? def.vs.entry : "main";
	def.fs.entry = (def.fs.entry) ? def.fs.entry : "main";

	for (int stage_index = 0; stage_index < VGFX_NUM_SHADER_STAGES; stage_index++) {
		vgfx_shader_stage_prop* stage_prop = (stage_index == vgfx_shader_stage_vs) ? &def.vs : &def.fs;
		for (int ub_index = 0; ub_index < VGFX_MAX_SHADERSTAGE_UBS; ub_index++) {
			vgfx_shader_uniform_block_prop* ub_prop = &stage_prop->uniform_blocks[ub_index];
			if (ub_prop->size == 0) {
				break;
			}
			for (int u_index = 0; u_index < VGFX_MAX_UB_MEMBERS; u_index++) {
				vgfx_shader_uniform_prop* u_prop = &ub_prop->uniforms[u_index];
				if (u_prop->type == vgfx_uniform_type_invalid) {
					break;
				}
				u_prop->array_count = (u_prop->array_count) ? u_prop->array_count : 1;
			}
		}
	}
	return def;
}

vgfx_shader vp_gfx_impl::alloc_shader()
{
	vgfx_shader res = {};
	int index = pool_alloc_index(&_pools.shader_pool);
	if (index != INVALID_SLOT_INDEX) {
		res.id = slot_alloc(&_pools.shader_pool, &_pools.shaders[index].slot, index);
	} else {
		res.id = INVALID_ID;
	}
	return res;
}

native_shader* vp_gfx_impl::shader_at(const vgfx_pools* p, uint32_t shd_id)
{
	if (!p || (shd_id == INVALID_ID)) {
		fail("shader", vgfx_error_unallocated);
	}
	int index = slot_index(shd_id);
	if ((index <= INVALID_SLOT_INDEX) || (index >= p->shader_pool.size)) {
		fail("shader index", vgfx_error_invalid);
	}
	return &p->shaders[index];
}

native_shader* vp_gfx_impl::lookup_shader(const vgfx_pools* p, uint32_t shd_id)
{
	if (!p) {
		fail("pools", vgfx_error_uninitialized);
	}
	if (shd_id != INVALID_ID) {
		native_shader* shd = shader_at(p, shd_id);
		if (shd->slot.id == shd_id) {
			return shd;
		}
	}
	return nullptr;
}

void vp_gfx_impl::init_shader(vgfx_shader shd_id, const vgfx_shader_prop* prop)
{
	if (shd_id.id == INVALID_ID || !prop) {
		fail("shader prop", vgfx_error_uninitialized);
	}
	native_shader* shd = lookup_shader(&_pools, shd_id.id);
	if (!shd || shd->slot.state != vgfx_resource_state_alloc) {
		fail("shader", vgfx_error_unallocated);
	} 
	shd->slot.ctx_id = _active_context.id;
#if VP_APP_D3D11_BACKEND
	shd->slot.state = d3d11_create_shader(shd, prop);
#endif
	if ((shd->slot.state != vgfx_resource_state_valid) && (shd->slot.state != vgfx_resource_state_failed)) {
		fail("shader", vgfx_error_create_failed);
	}
}

vgfx_shader vp_gfx_impl::make_shader(const vgfx_shader_prop* prop)
{
	if (!_valid || !prop) {
		fail("shader prop", vgfx_error_uninitialized);
	}
	vgfx_shader_prop prop_def = shader_prop_defaults(prop);
	vgfx_shader shd_id = alloc_shader();
	if (shd_id.id != INVALID_ID) {
		init_shader(shd_id, &prop_def);
	} else {
		warning_exhausted("shader");
	}
	return shd_id;
}

void vp_gfx_impl::reset_shader(native_shader* shd)
{
	if (!shd) {
		fail("shader", vgfx_error_uninitialized);
	}
	memset(shd, 0, sizeof(native_shader));
}

void vp_gfx_impl::destroy_shader(vgfx_shader shd_id)
{
	if (!_valid) {
		fail(VGFX_GLOBAL_OBJECT, vgfx_error_uninitialized);
	}
	native_shader* shd = lookup_shader(&_pools, shd_id.id);
	if (shd) {
		if (shd->slot.ctx_id == _active_context.id) {
#if VP_APP_D3D11_BACKEND
			d3d11_destroy_shader(shd);
#endif
			reset_shader(shd);
			pool_free_index(&_pools.shader_pool, slot_index(shd_id.id));
		} else {
			warning_mismatch();
		}
	}
}

static vgfx_pipeline_prop pipeline_prop_defaults(const vgfx_pipeline_prop* prop)
{
	const char* MSG_INVALID_INDEX_BUFFER_SIZE = "Invalid index buffer size";
	vgfx_pipeline_prop def = *prop;

	def.primitive_type = (def.primitive_type != vgfx_primitive_type_default) ? def.primitive_type : vgfx_primitive_type_triangles;
	def.index_type = (def.index_type != vgfx_index_type_default) ? def.index_type : vgfx_index_type_none;

	def.depth_stencil.stencil_front.fail_op = (def.depth_stencil.stencil_front.fail_op != vgfx_stencil_op_default) ? def.depth_stencil.stencil_front.fail_op : vgfx_stencil_op_keep;
	def.depth_stencil.stencil_front.depth_fail_op = (def.depth_stencil.stencil_front.depth_fail_op != vgfx_stencil_op_default) ? def.depth_stencil.stencil_front.depth_fail_op : vgfx_stencil_op_keep;
	def.depth_stencil.stencil_front.pass_op = (def.depth_stencil.stencil_front.pass_op != vgfx_stencil_op_default) ? def.depth_stencil.stencil_front.pass_op : vgfx_stencil_op_keep;
	def.depth_stencil.stencil_front.compare_func = (def.depth_stencil.stencil_front.compare_func != vgfx_compare_func_default) ? def.depth_stencil.stencil_front.compare_func : vgfx_compare_func_always;
	def.depth_stencil.stencil_back.fail_op = (def.depth_stencil.stencil_back.fail_op != vgfx_stencil_op_default) ? def.depth_stencil.stencil_back.fail_op : vgfx_stencil_op_keep;
	def.depth_stencil.stencil_back.depth_fail_op = (def.depth_stencil.stencil_back.depth_fail_op != vgfx_stencil_op_default) ? def.depth_stencil.stencil_back.depth_fail_op : vgfx_stencil_op_keep;
	def.depth_stencil.stencil_back.pass_op = (def.depth_stencil.stencil_back.pass_op != vgfx_stencil_op_default) ? def.depth_stencil.stencil_back.pass_op : vgfx_stencil_op_keep;
	def.depth_stencil.stencil_back.compare_func = (def.depth_stencil.stencil_back.compare_func != vgfx_compare_func_default) ? def.depth_stencil.stencil_back.compare_func : vgfx_compare_func_always;
	def.depth_stencil.depth_compare_func = (def.depth_stencil.depth_compare_func != vgfx_compare_func_default) ? def.depth_stencil.depth_compare_func : vgfx_compare_func_always;

	def.blend.src_factor_rgb = (def.blend.src_factor_rgb != vgfx_blend_factor_default) ? def.blend.src_factor_rgb : vgfx_blend_factor_one;
	def.blend.dst_factor_rgb = (def.blend.dst_factor_rgb != vgfx_blend_factor_default) ? def.blend.dst_factor_rgb : vgfx_blend_factor_zero;
	def.blend.op_rgb = (def.blend.op_rgb != vgfx_blend_op_default) ? def.blend.op_rgb : vgfx_blend_op_add;
	def.blend.src_factor_alpha = (def.blend.src_factor_alpha != vgfx_blend_factor_default) ? def.blend.src_factor_alpha : vgfx_blend_factor_one;
	def.blend.dst_factor_alpha = (def.blend.dst_factor_alpha != vgfx_blend_factor_default) ? def.blend.dst_factor_alpha : vgfx_blend_factor_zero;
	def.blend.op_alpha = (def.blend.op_alpha != vgfx_blend_op_default) ? def.blend.op_alpha : vgfx_blend_op_add;
	if (def.blend.color_write_mask == VGFX_COLORMASK_NONE) {
		def.blend.color_write_mask = 0;
	} else {
		def.blend.color_write_mask = (def.blend.color_write_mask != 0) ? def.blend.color_write_mask : VGFX_COLORMASK_RGBA;
	}
	def.blend.color_attachment_count = (def.blend.color_attachment_count != 0) ? def.blend.color_attachment_count : 1;
#if VP_APP_D3D11_BACKEND
	def.blend.color_format = (def.blend.color_format != vgfx_pixel_format_default) ? def.blend.color_format : vgfx_pixel_format_bgra8;
#else
	def.blend.color_format = (def.blend.color_format != vgfx_pixel_format_default) ? def.blend.color_format : vgfx_pixel_format_rgba8;
#endif
	def.blend.depth_format = (def.blend.depth_format != vgfx_pixel_format_default) ? def.blend.depth_format : vgfx_pixel_format_depth_stencil;

	def.rasterizer.cull_mode = (def.rasterizer.cull_mode != vgfx_cull_mode_default) ? def.rasterizer.cull_mode : vgfx_cull_mode_none;
	def.rasterizer.face_winding = (def.rasterizer.face_winding != vgfx_face_winding_default) ? def.rasterizer.face_winding : vgfx_face_winding_cw;
	def.rasterizer.sample_count = (def.rasterizer.sample_count != 0) ? def.rasterizer.sample_count : 1;

	for (int attr_index = 0; attr_index < VGFX_MAX_VERTEX_ATTRIBUTES; attr_index++) {
		vgfx_vertex_attr_prop* a_prop = &def.layout.attrs[attr_index];
		if (a_prop->format == vgfx_vertex_format_invalid) {
			break;
		}
		if ((a_prop->buffer_index < 0) || (a_prop->buffer_index >= VGFX_MAX_SHADERSTAGE_BUFFERS)) {
			vp_gfx_impl::instance.fail("attribute buffer index", vgfx_error_invalid);

		} 
		vgfx_buffer_layout_prop* b_prop = &def.layout.buffers[a_prop->buffer_index];
		b_prop->step_func = (b_prop->step_func != vgfx_vertex_step_invalid) ? b_prop->step_func : vgfx_vertex_step_per_vertex;
		b_prop->step_rate = (b_prop->step_rate != 0) ? b_prop->step_rate : 1;
	}

	int auto_offset[VGFX_MAX_SHADERSTAGE_BUFFERS] = { 0 };
	bool use_auto_offset = true;
	for (int i = 0; i < VGFX_MAX_VERTEX_ATTRIBUTES; i++) {
		if (def.layout.attrs[i].offset != 0) {
			use_auto_offset = false;
		}
	}
	for (int i = 0; i < VGFX_MAX_VERTEX_ATTRIBUTES; i++) {
		vgfx_vertex_attr_prop* a_prop = &def.layout.attrs[i];
		if (a_prop->format == vgfx_vertex_format_invalid) {
			break;
		}
		if ((a_prop->buffer_index < 0) || (a_prop->buffer_index >= VGFX_MAX_SHADERSTAGE_BUFFERS)) {
			vp_gfx_impl::instance.fail("attribute buffer index", vgfx_error_invalid);
		}
		if (use_auto_offset) {
			a_prop->offset = auto_offset[a_prop->buffer_index];
		}
		auto_offset[a_prop->buffer_index] += vertexformat_bytesize(a_prop->format);
	}
	for (int i = 0; i < VGFX_MAX_SHADERSTAGE_BUFFERS; i++) {
		vgfx_buffer_layout_prop* l_prop = &def.layout.buffers[i];
		if (l_prop->stride == 0) {
			l_prop->stride = auto_offset[i];
		}
	}

	return def;
}

vgfx_pipeline vp_gfx_impl::alloc_pipeline()
{
	vgfx_pipeline res = {};
	int index = pool_alloc_index(&_pools.pipeline_pool);
	if (index != INVALID_SLOT_INDEX) {
		res.id = slot_alloc(&_pools.pipeline_pool, &_pools.pipelines[index].slot, index);
	} else {
		res.id = INVALID_ID;
	}
	return res;
}

native_pipeline* vp_gfx_impl::pipeline_at(const vgfx_pools* p, uint32_t pip_id)
{
	if (!p || (pip_id == INVALID_ID)) {
		fail("pipeline", vgfx_error_unallocated);
		
	}
	int index = slot_index(pip_id);
	if ((index <= INVALID_SLOT_INDEX) || (index >= p->pipeline_pool.size)) {
		fail("pipeline id", vgfx_error_invalid);
		
	}
	return &p->pipelines[index];
}

native_pipeline* vp_gfx_impl::lookup_pipeline(const vgfx_pools* p, uint32_t pip_id)
{
	if (!p) {
		fail("pools", vgfx_error_uninitialized);
	}
	if (pip_id != INVALID_ID) {
		native_pipeline* pip = pipeline_at(p, pip_id);
		if (pip->slot.id == pip_id) {
			return pip;
		}
	}
	return nullptr;
}

void vp_gfx_impl::init_pipeline(vgfx_pipeline pip_id, const vgfx_pipeline_prop* prop)
{
	if (pip_id.id == INVALID_ID || !prop) {
		fail("pipeline prop", vgfx_error_uninitialized);
	} 
	native_pipeline* pip = lookup_pipeline(&_pools, pip_id.id);
	if (!pip || pip->slot.state != vgfx_resource_state_alloc) {
		fail("pipeline", vgfx_error_unallocated);
	}
	pip->slot.ctx_id = _active_context.id;
	native_shader* shd = lookup_shader(&_pools, prop->shader.id);
	if (!shd || shd->slot.state != vgfx_resource_state_valid) {
		fail("pipeline shader", vgfx_error_uninitialized);
	} 
#if VP_APP_D3D11_BACKEND
	pip->slot.state = d3d11_create_pipeline(pip, shd, prop);
#endif	
	if ((pip->slot.state != vgfx_resource_state_valid) && (pip->slot.state != vgfx_resource_state_failed)) {
		fail("pipeline", vgfx_error_create_failed);
	}
}

vgfx_pipeline vp_gfx_impl::make_pipeline(const vgfx_pipeline_prop* prop)
{
	if (!_valid || !prop) {
		fail("pipeline prop", vgfx_error_uninitialized);
	}
	vgfx_pipeline_prop prop_def = pipeline_prop_defaults(prop);
	vgfx_pipeline pip_id = alloc_pipeline();
	if (pip_id.id != INVALID_ID) {
		init_pipeline(pip_id, &prop_def);
	} else {
		warning_exhausted("pipeline");
	}
	return pip_id;
}

void vp_gfx_impl::reset_pipeline(native_pipeline* pip)
{
	if (!pip) {
		fail("pipeline", vgfx_error_uninitialized);
	}
	memset(pip, 0, sizeof(native_pipeline));
}

void vp_gfx_impl::destroy_pipeline(vgfx_pipeline pip_id)
{
	if (!_valid) {
		fail(VGFX_GLOBAL_OBJECT, vgfx_error_uninitialized);
	}
	native_pipeline* pip = lookup_pipeline(&_pools, pip_id.id);
	if (pip) {
		if (pip->slot.ctx_id == _active_context.id) {
#if VP_APP_D3D11_BACKEND
			d3d11_destroy_pipeline(pip);
#endif
			reset_pipeline(pip);
			pool_free_index(&_pools.pipeline_pool, slot_index(pip_id.id));
		} else {
			warning_mismatch();
		}
	}
}

static vgfx_pass_prop pass_prop_defaults(const vgfx_pass_prop* prop)
{
	vgfx_pass_prop def = *prop;
	return def;
}

vgfx_pass vp_gfx_impl::alloc_pass()
{
	vgfx_pass res = {};
	int index = pool_alloc_index(&_pools.pass_pool);
	if (index != INVALID_SLOT_INDEX) {
		res.id = slot_alloc(&_pools.pass_pool, &_pools.passes[index].slot, index);
	} else {
		res.id = INVALID_ID;
	}
	return res;
}

native_pass* vp_gfx_impl::pass_at(const vgfx_pools* p, uint32_t pass_id)
{
	if (!p || (pass_id == INVALID_ID)) {
		fail("pass", vgfx_error_unallocated);
	}
	int index = slot_index(pass_id);
	if ((index <= INVALID_SLOT_INDEX) || (index >= p->pass_pool.size)) {
		fail("pass index", vgfx_error_invalid);
	}
	return &p->passes[index];
}

native_pass* vp_gfx_impl::lookup_pass(const vgfx_pools* p, uint32_t pass_id)
{
	if (!p) {
		fail("pools", vgfx_error_unallocated);
	}
	if (pass_id != INVALID_ID) {
		native_pass* pass = pass_at(p, pass_id);
		if (pass->slot.id == pass_id) {
			return pass;
		}
	}
	return nullptr;
}

void vp_gfx_impl::init_pass(vgfx_pass pass_id, const vgfx_pass_prop* prop)
{
	if (pass_id.id == INVALID_ID || !prop) {
		fail("pass prop", vgfx_error_uninitialized);
	} 
	native_pass* pass = lookup_pass(&_pools, pass_id.id);
	if (!pass || pass->slot.state != vgfx_resource_state_alloc) {
		fail("pass", vgfx_error_unallocated);
	} 
	pass->slot.ctx_id = _active_context.id;
	native_image* att_imgs[VGFX_MAX_COLOR_ATTACHMENTS + 1] = { 0 };
	for (int i = 0; i < VGFX_MAX_COLOR_ATTACHMENTS; i++) {
		if (prop->color_attachments[i].image.id) {
			att_imgs[i] = lookup_image(&_pools, prop->color_attachments[i].image.id);
			if (!att_imgs[i] || att_imgs[i]->slot.state != vgfx_resource_state_valid) {
				fail("pass color attachment image", vgfx_error_uninitialized);
			}
		} else {
			att_imgs[i] = nullptr;
		}
	}
	const int ds_att_index = VGFX_MAX_COLOR_ATTACHMENTS;
	if (prop->depth_stencil_attachment.image.id) {
		att_imgs[ds_att_index] = lookup_image(&_pools, prop->depth_stencil_attachment.image.id);
		if (!att_imgs[ds_att_index] || att_imgs[ds_att_index]->slot.state != vgfx_resource_state_valid) {
			fail("pass depth stencil attachment image", vgfx_error_uninitialized);
		}
	} else {
		att_imgs[ds_att_index] = nullptr;
	}
#if VP_APP_D3D11_BACKEND
	pass->slot.state = d3d11_create_pass(pass, att_imgs, prop);
#endif
	if ((pass->slot.state != vgfx_resource_state_valid) && (pass->slot.state != vgfx_resource_state_failed)) {
		fail("pass", vgfx_error_create_failed);
	}
}

vgfx_pass vp_gfx_impl::make_pass(const vgfx_pass_prop* prop)
{
	if (!_valid) {
		fail(VGFX_GLOBAL_OBJECT, vgfx_error_uninitialized);
	}
	if (!prop) {
		fail("pass prop", vgfx_error_uninitialized);
	}
	vgfx_pass_prop prop_def = pass_prop_defaults(prop);
	vgfx_pass pass_id = alloc_pass();
	if (pass_id.id != INVALID_ID) {
		init_pass(pass_id, &prop_def);
	} else {
		warning_exhausted("Pass");
	}
	return pass_id;
}

void vp_gfx_impl::reset_pass(native_pass* pass)
{
	if (!pass) {
		fail("pass", vgfx_error_uninitialized);
	}
	memset(pass, 0, sizeof(native_pass));
}

void vp_gfx_impl::destroy_pass(vgfx_pass pass_id)
{
	if (!_valid) {
		fail(VGFX_GLOBAL_OBJECT, vgfx_error_uninitialized);
	}
	native_pass* pass = lookup_pass(&_pools, pass_id.id);
	if (pass) {
		if (pass->slot.ctx_id == _active_context.id) {
#if VP_APP_D3D11_BACKEND
			d3d11_destroy_pass(pass);
#endif
			reset_pass(pass);
			pool_free_index(&_pools.pass_pool, slot_index(pass_id.id));
		} else {
			warning_mismatch();
		}
	}
}
#pragma endregion

static const char* VGFX_MSG_ONLY_ONE_UPDATE_ALLOWED = "only one update allowed per buffer and frame";
static const char* VGFX_MSG_NO_UPDATE_AND_APPEND_ON_SAME_TIME = "update and append on same buffer in same frame is not allowed";

void vp_gfx_impl::update_buffer(vgfx_buffer buf_id, const void* data, int num_bytes)
{
	if (!_valid) {
		fail(VGFX_GLOBAL_OBJECT, vgfx_error_uninitialized);
	}
	native_buffer* buf = lookup_buffer(&_pools, buf_id.id);
	if ((num_bytes > 0) && buf && (buf->slot.state == vgfx_resource_state_valid)) {
		if (num_bytes > buf->cmn.size) {
			fail("number of bytes", vgfx_error_invalid);
		} 
		if (buf->cmn.update_frame_index == _frame_index) {
			fail(VGFX_MSG_ONLY_ONE_UPDATE_ALLOWED, vgfx_error_any);
		} 
		if (buf->cmn.append_frame_index == _frame_index) {
			fail(VGFX_MSG_NO_UPDATE_AND_APPEND_ON_SAME_TIME, vgfx_error_any);
		}
#if VP_APP_D3D11_BACKEND
		d3d11_update_buffer(buf, data, num_bytes);
#endif
		buf->cmn.update_frame_index = _frame_index;
	}
}

void vp_gfx_impl::update_image(vgfx_image img_id, const vgfx_image_content* data)
{
	if (!_valid) {
		fail(VGFX_GLOBAL_OBJECT, vgfx_error_uninitialized);
	}
	native_image* img = lookup_image(&_pools, img_id.id);
	if (img && img->slot.state == vgfx_resource_state_valid) {
		if (img->cmn.upd_frame_index == _frame_index) {
			fail(VGFX_MSG_ONLY_ONE_UPDATE_ALLOWED, vgfx_error_any);
		}
#if VP_APP_D3D11_BACKEND
		d3d11_update_image(img, data);
#endif
		img->cmn.upd_frame_index = _frame_index;
	}
}

int vp_gfx_impl::append_buffer(vgfx_buffer buf_id, const void* data, int num_bytes)
{
	if (!_valid) {
		fail(VGFX_GLOBAL_OBJECT, vgfx_error_uninitialized);
	}
	native_buffer* buf = lookup_buffer(&_pools, buf_id.id);
	int result;
	if (buf) {
		if (buf->cmn.append_frame_index != _frame_index) {
			buf->cmn.append_pos = 0;
			buf->cmn.append_overflow = false;
		}
		if ((buf->cmn.append_pos + num_bytes) > buf->cmn.size) {
			buf->cmn.append_overflow = true;
		}
		const int start_pos = buf->cmn.append_pos;
		if (buf->slot.state == vgfx_resource_state_valid) {
			if (!buf->cmn.append_overflow && (num_bytes > 0)) {
				if (buf->cmn.update_frame_index == _frame_index) {
					fail(VGFX_MSG_NO_UPDATE_AND_APPEND_ON_SAME_TIME, vgfx_error_any);
				} 
#if VP_APP_D3D11_BACKEND
				d3d11_append_buffer(buf, data, num_bytes, buf->cmn.append_frame_index != _frame_index);
#endif
				buf->cmn.append_pos += num_bytes;
				buf->cmn.append_frame_index = _frame_index;
			}
		}
		result = start_pos;
	} else {
		result = 0;
	}
	return result;
}

bool vp_gfx_impl::query_buffer_overflow(vgfx_buffer buf_id)
{
	if (!_valid) {
		fail(VGFX_GLOBAL_OBJECT, vgfx_error_uninitialized);
	}
	native_buffer* buf = lookup_buffer(&_pools, buf_id.id);
	return buf ? buf->cmn.append_overflow : false;
}

void vp_gfx_impl::resolve_default_pass_action(const vgfx_pass_action* from, vgfx_pass_action* to)
{
	if (!from || !to) {
		fail("pass action", vgfx_error_uninitialized);
	}
	*to = *from;
	for (int i = 0; i < VGFX_MAX_COLOR_ATTACHMENTS; i++) {
		if (to->colors[i].action == vgfx_action_default) {
			to->colors[i].action = vgfx_action_clear;
			to->colors[i].val[0] = DEFAULT_CLEAR_RED;
			to->colors[i].val[1] = DEFAULT_CLEAR_GREEN;
			to->colors[i].val[2] = DEFAULT_CLEAR_BLUE;
			to->colors[i].val[3] = DEFAULT_CLEAR_ALPHA;
		}
	}
	if (to->depth.action == vgfx_action_default) {
		to->depth.action = vgfx_action_clear;
		to->depth.val = DEFAULT_CLEAR_DEPTH;
	}
	if (to->stencil.action == vgfx_action_default) {
		to->stencil.action = vgfx_action_clear;
		to->stencil.val = DEFAULT_CLEAR_STENCIL;
	}
}

void vp_gfx_impl::begin_default_pass(const vgfx_pass_action* pass, int width, int height)
{
	if (!_valid) {
		fail(VGFX_GLOBAL_OBJECT, vgfx_error_uninitialized);
	}
	if (!pass) {
		fail("pass", vgfx_error_uninitialized);
	}
	vgfx_pass_action pa;
	resolve_default_pass_action(pass, &pa);
	_cur_pass.id = INVALID_ID;
	_pass_valid = true;
#if VP_APP_D3D11_BACKEND	
	d3d11_begin_pass(nullptr, &pa, width, height);
#endif
}

void vp_gfx_impl::begin_pass(vgfx_pass pass_id, const vgfx_pass_action* act)
{
	if (!_valid) {
		fail(VGFX_GLOBAL_OBJECT, vgfx_error_uninitialized);
	}
	if (!act) {
		fail("pass action", vgfx_error_uninitialized);
	}
	_cur_pass = pass_id;
	native_pass* pass = lookup_pass(&_pools, pass_id.id);
	if (pass) {
		_pass_valid = true;
		vgfx_pass_action pa;
		resolve_default_pass_action(act, &pa);
		const native_image* img;
#if VP_APP_D3D11_BACKEND
		img = d3d11_pass_color_image(pass, 0);
#endif
		const int w = img->cmn.width;
		const int h = img->cmn.height;
#if VP_APP_D3D11_BACKEND
		d3d11_begin_pass(pass, &pa, w, h);
#endif
	} else {
		_pass_valid = false;
	}
}

void vp_gfx_impl::end_pass()
{
	if (!_valid) {
		fail(VGFX_GLOBAL_OBJECT, vgfx_error_uninitialized);
	}
	if (!_pass_valid) {
		return;
	}
#if VP_APP_D3D11_BACKEND
	d3d11_end_pass();
#endif
	_cur_pass.id = INVALID_ID;
	_cur_pipeline.id = INVALID_ID;
	_pass_valid = false;
}

void vp_gfx_impl::commit()
{
	if (!_valid) {
		fail(VGFX_GLOBAL_OBJECT, vgfx_error_uninitialized);
	}
#if VP_APP_D3D11_BACKEND
	d3d11_commit();
#endif
	_frame_index++;
}

void vp_gfx_impl::apply_viewport(int x, int y, int w, int h, bool origin_top_left)
{
	if (!_valid) {
		fail(VGFX_GLOBAL_OBJECT, vgfx_error_uninitialized);
	}
	if (!_pass_valid) {
		return;
	}
#if VP_APP_D3D11_BACKEND
	d3d11_apply_viewport(x, y, w, h, origin_top_left);
#endif
}

void vp_gfx_impl::apply_scissor_rect(int x, int y, int width, int height, bool origin_top_left)
{
	if (!_valid) {
		fail(VGFX_GLOBAL_OBJECT, vgfx_error_uninitialized);
	}
	if (!_pass_valid) {
		return;
	}
#if VP_APP_D3D11_BACKEND
	d3d11_apply_scissor_rect(x, y, width, height, origin_top_left);
#endif
}

void vp_gfx_impl::apply_bindings(const vgfx_bindings* bindings)
{
	if (!_valid) {
		fail(VGFX_GLOBAL_OBJECT, vgfx_error_uninitialized);
	}
	if (!bindings) {
		fail("bindings", vgfx_error_uninitialized);
	}
	_bindings_valid = true;
	native_pipeline* pip = lookup_pipeline(&_pools, _cur_pipeline.id);
	if (!pip) {
		fail("pipeline", vgfx_error_uninitialized);
	}
	native_buffer* vbs[VGFX_MAX_SHADERSTAGE_BUFFERS] = { 0 };
	int nvbs = 0;
	for (int i = 0; i < VGFX_MAX_SHADERSTAGE_BUFFERS; i++, nvbs++) {
		if (bindings->vertex_buffers[i].id) {
			vbs[i] = lookup_buffer(&_pools, bindings->vertex_buffers[i].id);
			if (!vbs[i]) {
				fail("vertex buffer object", vgfx_error_uninitialized);
			}
			_next_draw_valid &= (vbs[i]->slot.state == vgfx_resource_state_valid);
			_next_draw_valid &= !vbs[i]->cmn.append_overflow;
		} else {
			break;
		}
	}
	native_buffer* ib = nullptr;
	if (bindings->index_buffer.id) {
		ib = lookup_buffer(&_pools, bindings->index_buffer.id);
		if (!ib) {
			fail("index buffer object", vgfx_error_uninitialized);
		}
		_next_draw_valid &= (ib->slot.state == vgfx_resource_state_valid);
		_next_draw_valid &= !ib->cmn.append_overflow;
	}
	native_image* vs_imgs[VGFX_MAX_SHADERSTAGE_IMAGES] = { 0 };
	int nvsimg = 0;
	for (int i = 0; i < VGFX_MAX_SHADERSTAGE_IMAGES; i++, nvsimg++) {
		if (bindings->vs_images[i].id) {
			vs_imgs[i] = lookup_image(&_pools, bindings->vs_images[i].id);
			if (!vs_imgs[i]) {
				fail("vertex shader texture", vgfx_error_uninitialized);
			}
			_next_draw_valid &= (vs_imgs[i]->slot.state == vgfx_resource_state_valid);
		} else {
			break;
		}
	}
	native_image* fs_imgs[VGFX_MAX_SHADERSTAGE_IMAGES] = { 0 };
	int nfsimg = 0;
	for (int i = 0; i < VGFX_MAX_SHADERSTAGE_IMAGES; i++, nfsimg++) {
		if (bindings->fs_images[i].id) {
			fs_imgs[i] = lookup_image(&_pools, bindings->fs_images[i].id);
			if (!fs_imgs[i]) {
				fail("fragment shader texture", vgfx_error_uninitialized);
			}
			_next_draw_valid &= (fs_imgs[i]->slot.state == vgfx_resource_state_valid);
		} else {
			break;
		}
	}
	if (_next_draw_valid) {
		const int* vb_offsets = bindings->vertex_buffer_offsets;
		int ib_offset = bindings->index_buffer_offset;
#if VP_APP_D3D11_BACKEND
		d3d11_apply_bindings(pip, vbs, vb_offsets, nvbs, ib, ib_offset, vs_imgs, nvsimg, fs_imgs, nfsimg);
#endif
	}
}

void vp_gfx_impl::apply_pipeline(vgfx_pipeline pip_id)
{
	if (!_valid) {
		fail(VGFX_GLOBAL_OBJECT, vgfx_error_uninitialized);
	}
	_bindings_valid = false;
	if (!_pass_valid) {
		return;
	}
	_cur_pipeline = pip_id;
	native_pipeline* pip = lookup_pipeline(&_pools, pip_id.id);
	if (!pip) {
		fail("pipeline", vgfx_error_uninitialized);
	}
	_next_draw_valid = (pip->slot.state == vgfx_resource_state_valid);
	if (!pip->shader) {
		fail("pipeline shader", vgfx_error_uninitialized);
	}
	if (pip->shader->slot.id != pip->cmn.shader_id.id) {
		fail("pipeline shader slot id", vgfx_error_invalid);
	}
#if VP_APP_D3D11_BACKEND
	d3d11_apply_pipeline(pip);
#endif
}

void vp_gfx_impl::apply_uniforms(vgfx_shader_stages stage, int ub_index, const void* data, int num_bytes)
{
	if (!_valid) {
		fail(VGFX_GLOBAL_OBJECT, vgfx_error_uninitialized);
	}
	if ((stage != vgfx_shader_stage_vs) && (stage != vgfx_shader_stage_fs)) {
		fail("shader stage", vgfx_error_invalid);
	}
	if ((ub_index < 0) || (ub_index >= VGFX_MAX_SHADERSTAGE_UBS)) {
		fail("uniform buffer index", vgfx_error_invalid);
	} 
	if (!data) {
		fail("uniform buffer data", vgfx_error_uninitialized);
	}
	if (num_bytes <= 0) {
		fail("number of bytes", vgfx_error_invalid);
	}
	if (!_pass_valid) {
		return;
	}
	if (!_next_draw_valid) {
		return;
	}
#if VP_APP_D3D11_BACKEND
	d3d11_apply_uniforms(stage, ub_index, data, num_bytes);
#endif
}

void vp_gfx_impl::draw(int base_element, int num_elements, int num_instances)
{
	if (!_valid) {
		fail(VGFX_GLOBAL_OBJECT, vgfx_error_uninitialized);
	}
#ifdef _DEBUG
	if (!_bindings_valid) {
		_app->log(vapp_log_message_type_warning, "Attempting draw without resource bindings");
	}
#endif
	if (!_pass_valid) {
		return;
	}
	if (!_next_draw_valid) {
		return;
	}
	if (!_bindings_valid) {
		return;
	}
#if VP_APP_D3D11_BACKEND
	d3d11_draw(base_element, num_elements, num_instances);
#endif
}

#pragma region C And C++ Shared Library Entries
VP_API vp_gfx* vp_gfx::create(const vgfx_prop* prop, vp_app* app)
{
	assert(app && prop);
	vp_gfx_impl::instance = vp_gfx_impl{prop, app};
	return &vp_gfx_impl::instance;
}

VP_EXTERN_C VP_API vp_gfx* vp_gfx_create(const vgfx_prop* prop, vp_app* app)
{
	return vp_gfx::create(prop, app);
}

vp_impl_c_method0(vp_gfx, shutdown);

vp_impl_c_function0(vp_gfx, query_props, vgfx_prop);
vp_impl_c_function0(vp_gfx, query_backend, vgfx_backend);
vp_impl_c_function0(vp_gfx, query_features, vgfx_features);
vp_impl_c_function0(vp_gfx, query_limits, vgfx_limits);
vp_impl_c_function1(vp_gfx, query_pixel_format, vgfx_pixelformat_info, vgfx_pixel_format);

vp_impl_c_function1(vp_gfx, make_buffer, vgfx_buffer, const vgfx_buffer_prop*);
vp_impl_c_function1(vp_gfx, make_image, vgfx_image, const vgfx_image_prop*);
vp_impl_c_function1(vp_gfx, make_shader, vgfx_shader, const vgfx_shader_prop*);
vp_impl_c_function1(vp_gfx, make_pipeline, vgfx_pipeline, const vgfx_pipeline_prop*);
vp_impl_c_function1(vp_gfx, make_pass, vgfx_pass, const vgfx_pass_prop*);

vp_impl_c_method1(vp_gfx, destroy_buffer, vgfx_buffer);
vp_impl_c_method1(vp_gfx, destroy_image, vgfx_image);
vp_impl_c_method1(vp_gfx, destroy_shader, vgfx_shader);
vp_impl_c_method1(vp_gfx, destroy_pipeline, vgfx_pipeline);
vp_impl_c_method1(vp_gfx, destroy_pass, vgfx_pass);

vp_impl_c_method3(vp_gfx, update_buffer, vgfx_buffer, const void*, int);
vp_impl_c_method2(vp_gfx, update_image, vgfx_image, const vgfx_image_content*);
vp_impl_c_function3(vp_gfx, append_buffer, int, vgfx_buffer, const void*, int);
vp_impl_c_function1(vp_gfx, query_buffer_overflow, bool, vgfx_buffer);

vp_impl_c_method3(vp_gfx, begin_default_pass, const vgfx_pass_action*, int, int);
vp_impl_c_method2(vp_gfx, begin_pass, vgfx_pass, const vgfx_pass_action*);
vp_impl_c_method5(vp_gfx, apply_viewport, int, int, int, int, bool);
vp_impl_c_method5(vp_gfx, apply_scissor_rect, int, int, int, int, bool);
vp_impl_c_method1(vp_gfx, apply_bindings, const vgfx_bindings*);
vp_impl_c_method1(vp_gfx, apply_pipeline, vgfx_pipeline);
vp_impl_c_method4(vp_gfx, apply_uniforms, vgfx_shader_stages, int, const void*, int);
vp_impl_c_method3(vp_gfx, draw, int, int, int);
vp_impl_c_method0(vp_gfx, end_pass);
vp_impl_c_method0(vp_gfx, commit);
#pragma endregion
