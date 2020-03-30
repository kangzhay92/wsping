#pragma once

#ifndef VP_APP_INCLUDED
#error "Please include <viper/app.h> before <viper/gfx.h>"
#endif
#define VP_GFX_INCLUDED

#define vgfx_declare_resource(name) \
	vp_begin_struct(name) \
		uint32_t id; \
	vp_end(name)

// ViperGFX Resources
vgfx_declare_resource(vgfx_buffer);
vgfx_declare_resource(vgfx_image);
vgfx_declare_resource(vgfx_shader);
vgfx_declare_resource(vgfx_pipeline);
vgfx_declare_resource(vgfx_pass);
vgfx_declare_resource(vgfx_context);

vp_begin_enum(vgfx_backend)
	vgfx_backend_gles2,
	vgfx_backend_gles3,
	vgfx_backend_d3d11
vp_end(vgfx_backend);

vp_begin_struct(vgfx_features)
	bool instancing;
	bool origin_top_left;
	bool multiple_render_targets;
	bool msaa_render_targets;
	bool imagetype_3d;
	bool imagetype_array;
	bool image_clamp_to_border;
vp_end(vgfx_features);

vp_begin_struct(vgfx_limits)
	unsigned max_image_size_2d;
	unsigned max_image_size_cube;
	unsigned max_image_size_3d;
	unsigned max_image_size_array;
	unsigned max_image_array_layers;
	unsigned max_vertex_attrs;
vp_end(vgfx_limits);

vp_begin_struct(vgfx_pixelformat_info)
	bool sample;
	bool filter;
	bool render;
	bool blend;
	bool msaa;
	bool depth;
vp_end(vgfx_pixelformat_info);

vp_begin_enum(vgfx_pixel_format)
	vgfx_pixel_format_default,
	vgfx_pixel_format_none,
	vgfx_pixel_format_r8,	
	vgfx_pixel_format_r8sn, 
	vgfx_pixel_format_r8ui, 
	vgfx_pixel_format_r8si,
	vgfx_pixel_format_r16, 
	vgfx_pixel_format_r16sn, 
	vgfx_pixel_format_r16ui, 
	vgfx_pixel_format_r16si, 
	vgfx_pixel_format_r16f,
	vgfx_pixel_format_rg8, 
	vgfx_pixel_format_rg8sn, 
	vgfx_pixel_format_rg8ui, 
	vgfx_pixel_format_rg8si,
	vgfx_pixel_format_r32ui, 
	vgfx_pixel_format_r32si, 
	vgfx_pixel_format_r32f, 
	vgfx_pixel_format_rg16, 
	vgfx_pixel_format_rg16sn, 
	vgfx_pixel_format_rg16ui, 
	vgfx_pixel_format_rg16si, 
	vgfx_pixel_format_rg16f, 
	vgfx_pixel_format_rgba8, 
	vgfx_pixel_format_rgba8sn, 
	vgfx_pixel_format_rgba8ui, 
	vgfx_pixel_format_rgba8si, 
	vgfx_pixel_format_bgra8,
	vgfx_pixel_format_rgb10a2, 
	vgfx_pixel_format_rg11b10f,
	vgfx_pixel_format_rg32ui, 
	vgfx_pixel_format_rg32si, 
	vgfx_pixel_format_rg32f, 
	vgfx_pixel_format_rgba16, 
	vgfx_pixel_format_rgba16sn, 
	vgfx_pixel_format_rgba16ui, 
	vgfx_pixel_format_rgba16si, 
	vgfx_pixel_format_rgba16f,
	vgfx_pixel_format_rgba32ui, 
	vgfx_pixel_format_rgba32si, 
	vgfx_pixel_format_rgba32f,
	vgfx_pixel_format_depth,
	vgfx_pixel_format_depth_stencil,
	vgfx_pixel_format_bc1_rgba,
	vgfx_pixel_format_bc2_rgba,
	vgfx_pixel_format_bc3_rgba,
	vgfx_pixel_format_bc4_r,
	vgfx_pixel_format_bc4_rsn,
	vgfx_pixel_format_bc5_rg,
	vgfx_pixel_format_bc5_rgsn,
	vgfx_pixel_format_bc6h_rgbf,
	vgfx_pixel_format_bc6h_rgbuf,
	vgfx_pixel_format_bc7_rgba,
	vgfx_pixel_format_pvrtc_rgb_2bpp,
	vgfx_pixel_format_pvrtc_rgb_4bpp,
	vgfx_pixel_format_pvrtc_rgba_2bpp,
	vgfx_pixel_format_pvrtc_rgba_4bpp,
	vgfx_pixel_format_etc2_rgb8,
	vgfx_pixel_format_etc2_rgb8a1,
	vgfx_pixel_format_etc2_rgba8,
	vgfx_pixel_format_etc2_rg11,
	vgfx_pixel_format_etc2_rg11sn,
	VGFX_NUM_PIXEL_FORMAT
vp_end(vgfx_pixel_format);

vp_begin_enum(vgfx_index_type)
	vgfx_index_type_default,
	vgfx_index_type_none,
	vgfx_index_type_uint16,
	vgfx_index_type_uint32,
vp_end(vgfx_index_type);

vp_begin_enum(vgfx_primitive_type)
	vgfx_primitive_type_default,
	vgfx_primitive_type_points,
	vgfx_primitive_type_lines,
	vgfx_primitive_type_line_strip,
	vgfx_primitive_type_triangles,
	vgfx_primitive_type_triangle_strip,
vp_end(vgfx_primitive_type);

vp_begin_enum(vgfx_filter)
	vgfx_filter_default,
	vgfx_filter_nearest,
	vgfx_filter_linear,
	vgfx_filter_nearest_mipmap_nearest,
	vgfx_filter_nearest_mipmap_linear,
	vgfx_filter_linear_mipmap_nearest,
	vgfx_filter_linear_mipmap_linear
vp_end(vgfx_filter);

vp_begin_enum(vgfx_wrap)
	vgfx_wrap_default,
	vgfx_wrap_repeat,
	vgfx_wrap_clamp_to_edge,
	vgfx_wrap_clamp_to_border,
	vgfx_wrap_mirrored_repeat
vp_end(vgfx_wrap);

vp_begin_enum(vgfx_border_color)
	vgfx_border_color_default,
	vgfx_border_color_transparent_black,
	vgfx_border_color_opaque_black,
	vgfx_border_color_opaque_white
vp_end(vgfx_border_color);

vp_begin_enum(vgfx_resource_state)
	vgfx_resource_state_initial,
	vgfx_resource_state_alloc,
	vgfx_resource_state_valid,
	vgfx_resource_state_failed,
	vgfx_resource_state_invalid
vp_end(vgfx_resource_state);

vp_begin_enum(vgfx_buffer_type)
	vgfx_buffer_type_default,
	vgfx_buffer_type_vertex_buffer,
	vgfx_buffer_type_index_buffer
vp_end(vgfx_buffer_type);

vp_begin_enum(vgfx_image_type)
	vgfx_image_type_default,
	vgfx_image_type_2d,
	vgfx_image_type_cube,
	vgfx_image_type_3d,
	vgfx_image_type_array
vp_end(vgfx_image_type);

vp_begin_enum(vgfx_usage)
	vgfx_usage_default,
	vgfx_usage_immutable,
	vgfx_usage_dynamic,
	vgfx_usage_stream
vp_end(vgfx_usage);

vp_begin_enum(vgfx_stencil_op)
	vgfx_stencil_op_default,
	vgfx_stencil_op_keep,
	vgfx_stencil_op_zero,
	vgfx_stencil_op_replace,
	vgfx_stencil_op_incr_clamp,
	vgfx_stencil_op_decr_clamp,
	vgfx_stencil_op_invert,
	vgfx_stencil_op_incr_wrap,
	vgfx_stencil_op_decr_wrap
vp_end(vgfx_stencil_op);

vp_begin_enum(vgfx_compare_func)
	vgfx_compare_func_default,
	vgfx_compare_func_never,
	vgfx_compare_func_less,
	vgfx_compare_func_equal,
	vgfx_compare_func_less_equal,
	vgfx_compare_func_greater,
	vgfx_compare_func_not_equal,
	vgfx_compare_func_greater_equal,
	vgfx_compare_func_always
vp_end(vgfx_compare_func);

vp_begin_enum(vgfx_blend_factor)
	vgfx_blend_factor_default,
	vgfx_blend_factor_zero,
	vgfx_blend_factor_one,
	vgfx_blend_factor_src_color,
	vgfx_blend_factor_one_minus_src_color,
	vgfx_blend_factor_src_alpha,
	vgfx_blend_factor_one_minus_src_alpha,
	vgfx_blend_factor_dst_color,
	vgfx_blend_factor_one_minus_dst_color,
	vgfx_blend_factor_dst_alpha,
	vgfx_blend_factor_one_minus_dst_alpha,
	vgfx_blend_factor_src_alpha_saturated,
	vgfx_blend_factor_blend_color,
	vgfx_blend_factor_one_minus_blend_color,
	vgfx_blend_factor_blend_alpha,
	vgfx_blend_factor_one_minus_blend_alpha
vp_end(vgfx_blend_factor);

vp_begin_enum(vgfx_blend_op)
	vgfx_blend_op_default,
	vgfx_blend_op_add,
	vgfx_blend_op_subtract,
	vgfx_blend_op_reverse_subtract
vp_end(vgfx_blend_op);

vp_begin_enum(vgfx_cull_mode)
	vgfx_cull_mode_default,
	vgfx_cull_mode_none,
	vgfx_cull_mode_front,
	vgfx_cull_mode_back
vp_end(vgfx_cull_mode);

vp_begin_enum(vgfx_face_winding)
	vgfx_face_winding_default,
	vgfx_face_winding_ccw,
	vgfx_face_winding_cw
vp_end(vgfx_face_winding);

vp_begin_enum(vgfx_cube_face)
	vgfx_cube_face_posx,
	vgfx_cube_face_negx,
	vgfx_cube_face_posy,
	vgfx_cube_face_negy,
	vgfx_cube_face_posz,
	vgfx_cube_face_negz,
	VGFX_NUM_CUBEFACE
vp_end(vgfx_cube_face);

vp_begin_enum(vgfx_action)
	vgfx_action_default,
	vgfx_action_clear,
	vgfx_action_load,
	vgfx_action_dont_care,
vp_end(vgfx_action);

vp_begin_enum(vgfx_uniform_type)
	vgfx_uniform_type_invalid,
	vgfx_uniform_type_float,
	vgfx_uniform_type_float2,
	vgfx_uniform_type_float3,
	vgfx_uniform_type_float4,
	vgfx_uniform_type_mat4
vp_end(vgfx_uniform_type);

vp_begin_enum(vgfx_shader_stages)
	vgfx_shader_stage_vs,
	vgfx_shader_stage_fs
vp_end(vgfx_shader_stages);

vp_begin_enum(vgfx_vertex_step)
	vgfx_vertex_step_invalid,
	vgfx_vertex_step_per_vertex,
	vgfx_vertex_step_per_instance
vp_end(vgfx_vertex_step);

vp_begin_enum(vgfx_vertex_format)
	vgfx_vertex_format_invalid,
	vgfx_vertex_format_float,
	vgfx_vertex_format_float2,
	vgfx_vertex_format_float3,
	vgfx_vertex_format_float4,
	vgfx_vertex_format_byte4,
	vgfx_vertex_format_byte4n,
	vgfx_vertex_format_ubyte4,
	vgfx_vertex_format_ubyte4n,
	vgfx_vertex_format_short2,
	vgfx_vertex_format_short2n,
	vgfx_vertex_format_ushort2n,
	vgfx_vertex_format_short4,
	vgfx_vertex_format_short4n,
	vgfx_vertex_format_ushort4n,
	vgfx_vertex_format_uint10n2
vp_end(vgfx_vertex_format);

enum
{
	VGFX_MAX_COLOR_ATTACHMENTS = 4,
	VGFX_MAX_SHADERSTAGE_UBS = 4,
	VGFX_MAX_SHADERSTAGE_IMAGES = 12,
	VGFX_NUM_SHADER_STAGES = 2,
	VGFX_MAX_UB_MEMBERS = 16,
	VGFX_MAX_VERTEX_ATTRIBUTES = 16,
	VGFX_MAX_SHADERSTAGE_BUFFERS = 8,
	VGFX_MAX_MIPMAPS = 16,
	VGFX_MAX_TEXTUREARRAY_LAYERS = 128,

	VGFX_COLORMASK_NONE = 0x10,
	VGFX_COLORMASK_R = 1 << 0,
	VGFX_COLORMASK_G = 1 << 1,
	VGFX_COLORMASK_B = 1 << 2,
	VGFX_COLORMASK_A = 1 << 3,
	VGFX_COLORMASK_RGB = 0x07,
	VGFX_COLORMASK_RGBA = 0x0F
};

vp_begin_struct(vgfx_color_attachment_action)
	vgfx_action action;
	float val[4];
vp_end(vgfx_color_attachment_action);

vp_begin_struct(vgfx_depth_attachment_action)
	vgfx_action action;
	float val;
vp_end(vgfx_depth_attachment_action);

vp_begin_struct(vgfx_stencil_attachment_action)
	vgfx_action action;
	uint8_t val;
vp_end(vgfx_stencil_attachment_action);

vp_begin_struct(vgfx_pass_action)
	vgfx_color_attachment_action colors[VGFX_MAX_COLOR_ATTACHMENTS];
	vgfx_depth_attachment_action depth;
	vgfx_stencil_attachment_action stencil;
vp_end(vgfx_pass_action);

vp_begin_struct(vgfx_buffer_prop)
	int size;
	vgfx_buffer_type type;
	vgfx_usage usage;
	const void* content;
	const void* d3d11_buffer;
vp_end(vgfx_buffer_prop);

vp_begin_struct(vgfx_bindings)
	vgfx_buffer vertex_buffers[VGFX_MAX_SHADERSTAGE_BUFFERS];
	int vertex_buffer_offsets[VGFX_MAX_SHADERSTAGE_BUFFERS];
	vgfx_buffer index_buffer;
	int index_buffer_offset;
	vgfx_image vs_images[VGFX_MAX_SHADERSTAGE_IMAGES];
	vgfx_image fs_images[VGFX_MAX_SHADERSTAGE_IMAGES];
vp_end(vgfx_bindings);

vp_begin_struct(vgfx_shader_attr_prop)
	const char* name;
	const char* sem_name;
	int sem_index;
vp_end(vgfx_shader_attr_prop);

vp_begin_struct(vgfx_shader_uniform_prop)
	const char* name;
	vgfx_uniform_type type;
	int array_count;
vp_end(vgfx_shader_uniform_prop);

vp_begin_struct(vgfx_shader_uniform_block_prop)
	int size;
	vgfx_shader_uniform_prop uniforms[VGFX_MAX_UB_MEMBERS];
vp_end(vgfx_shader_uniform_block_prop);

vp_begin_struct(vgfx_shader_image_prop)
	const char* name;
	vgfx_image_type type;
vp_end(vgfx_shader_image_prop);

vp_begin_struct(vgfx_shader_stage_prop)
	const char* source;
	const uint8_t* byte_code;
	int byte_code_size;
	const char* entry;
	vgfx_shader_uniform_block_prop uniform_blocks[VGFX_MAX_SHADERSTAGE_UBS];
	vgfx_shader_image_prop images[VGFX_MAX_SHADERSTAGE_IMAGES];
vp_end(vgfx_shader_stage_prop);

vp_begin_struct(vgfx_shader_prop)
	vgfx_shader_attr_prop attrs[VGFX_MAX_VERTEX_ATTRIBUTES];
	vgfx_shader_stage_prop vs;
	vgfx_shader_stage_prop fs;
vp_end(vgfx_shader_prop);

vp_begin_struct(vgfx_buffer_layout_prop)
	int stride;
	vgfx_vertex_step step_func;
	int step_rate;
vp_end(vgfx_buffer_layout_prop);

vp_begin_struct(vgfx_vertex_attr_prop)
	int buffer_index;
	int offset;
	vgfx_vertex_format format;
vp_end(vgfx_vertex_attr_prop);

vp_begin_struct(vgfx_layout_prop)
	vgfx_buffer_layout_prop buffers[VGFX_MAX_SHADERSTAGE_BUFFERS];
	vgfx_vertex_attr_prop attrs[VGFX_MAX_VERTEX_ATTRIBUTES];
vp_end(vgfx_layout_prop);

vp_begin_struct(vgfx_stencil_state)
	vgfx_stencil_op fail_op;
	vgfx_stencil_op depth_fail_op;
	vgfx_stencil_op pass_op;
	vgfx_compare_func compare_func;
vp_end(vgfx_stencil_state);

vp_begin_struct(vgfx_depth_stencil_state)
	vgfx_stencil_state stencil_front;
	vgfx_stencil_state stencil_back;
	vgfx_compare_func depth_compare_func;
	bool depth_write_enabled;
	bool stencil_enabled;
	uint8_t stencil_read_mask;
	uint8_t stencil_write_mask;
	uint8_t stencil_ref;
vp_end(vgfx_depth_stencil_state);

vp_begin_struct(vgfx_blend_state)
	bool enabled;
	vgfx_blend_factor src_factor_rgb;
	vgfx_blend_factor dst_factor_rgb;
	vgfx_blend_op op_rgb;
	vgfx_blend_factor src_factor_alpha;
	vgfx_blend_factor dst_factor_alpha;
	vgfx_blend_op op_alpha;
	uint8_t color_write_mask;
	int color_attachment_count;
	vgfx_pixel_format color_format;
	vgfx_pixel_format depth_format;
	float blend_color[4];
vp_end(vgfx_blend_state);

vp_begin_struct(vgfx_rasterizer_state)
	bool alpha_to_coverage_enabled;
	vgfx_cull_mode cull_mode;
	vgfx_face_winding face_winding;
	int sample_count;
	float depth_bias;
	float depth_bias_slope_scale;
	float depth_bias_clamp;
vp_end(vgfx_rasterizer_state);

vp_begin_struct(vgfx_pipeline_prop)
	vgfx_layout_prop layout;
	vgfx_shader shader;
	vgfx_primitive_type primitive_type;
	vgfx_index_type index_type;
	vgfx_depth_stencil_state depth_stencil;
	vgfx_blend_state blend;
	vgfx_rasterizer_state rasterizer;
vp_end(vgfx_pipeline_prop);

vp_begin_struct(vgfx_subimage_content)
	const void* ptr;
	int size;
vp_end(vgfx_subimage_content);

vp_begin_struct(vgfx_image_content)
	vgfx_subimage_content subimage[VGFX_NUM_CUBEFACE][VGFX_MAX_MIPMAPS];
vp_end(vgfx_image_content);

vp_begin_struct(vgfx_image_prop)
	vgfx_image_type type;
	bool render_target;
	int width;
	int height;
	union { 
		int depth;
		int layers;
	};
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
	uint32_t max_anisotropy;
	float min_lod;
	float max_lod;
	vgfx_image_content content;

#if VP_APP_D3D11_BACKEND
	const void* d3d11_texture;
#endif
vp_end(vgfx_image_prop);

vp_begin_struct(vgfx_attachment_prop)
	vgfx_image image;
	int mip_level;
	union {
		int face;
		int layer;
		int slice;
	};
vp_end(vgfx_attachment_prop);

vp_begin_struct(vgfx_pass_prop)
	vgfx_attachment_prop color_attachments[VGFX_MAX_COLOR_ATTACHMENTS];
	vgfx_attachment_prop depth_stencil_attachment;
vp_end(vgfx_pass_prop);

vp_begin_struct(vgfx_prop)
	int buffer_pool_size;
	int image_pool_size;
	int shader_pool_size;
	int pipeline_pool_size;
	int pass_pool_size;
	int context_pool_size;

#if VP_APP_D3D11_BACKEND
	const void* d3d11_device;
	const void* d3d11_device_context;
	vp_function(d3d11_render_target_view_cb, const void*);
	vp_function(d3d11_depth_stencil_view_cb, const void*);
#endif
vp_end(vgfx_prop);

// ViperGFX core struct
#if (defined(__cplusplus) && !defined(VP_C_INTERFACE))
vp_begin_struct(vp_gfx)
	// Initializer function
	VP_API static vp_gfx* create(const vgfx_prop* prop, vp_app* app);

	// Deinitializer function
	virtual void shutdown() = 0;

	// Info getter
	virtual vgfx_prop query_props() = 0;
	virtual vgfx_backend query_backend() = 0;
	virtual vgfx_features query_features() = 0;
	virtual vgfx_limits query_limits() = 0;
	virtual vgfx_pixelformat_info query_pixel_format(vgfx_pixel_format fmt) = 0;

	// Resource allocation functions
	virtual vgfx_buffer   make_buffer(const vgfx_buffer_prop* prop) = 0;
	virtual vgfx_image    make_image(const vgfx_image_prop* prop) = 0;
	virtual vgfx_shader   make_shader(const vgfx_shader_prop* prop) = 0;
	virtual vgfx_pipeline make_pipeline(const vgfx_pipeline_prop* prop) = 0;
	virtual vgfx_pass     make_pass(const vgfx_pass_prop* prop) = 0;

	// Resource destruction functions
	virtual void destroy_buffer(vgfx_buffer buf) = 0;
	virtual void destroy_image(vgfx_image img) = 0;
	virtual void destroy_shader(vgfx_shader shd) = 0;
	virtual void destroy_pipeline(vgfx_pipeline pip) = 0;
	virtual void destroy_pass(vgfx_pass pass) = 0;

	// Updating functions
	virtual void update_buffer(vgfx_buffer buf_id, const void* data_ptr, int data_size) = 0;
	virtual void update_image(vgfx_image img_id, const vgfx_image_content* data) = 0;
	virtual int append_buffer(vgfx_buffer buf, const void* data_ptr, int data_size) = 0;
	virtual bool query_buffer_overflow(vgfx_buffer buf) = 0;

	// Rendering functions
	virtual void begin_default_pass(const vgfx_pass_action* act, int width, int height) = 0;
	virtual void begin_pass(vgfx_pass pass_id, const vgfx_pass_action* act) = 0;
	virtual void apply_viewport(int x, int y, int width, int height, bool origin_top_left) = 0;
	virtual void apply_scissor_rect(int x, int y, int width, int height, bool origin_top_left) = 0;
	virtual void apply_bindings(const vgfx_bindings* bindings) = 0;
	virtual void apply_pipeline(vgfx_pipeline pip_id) = 0;
	virtual void apply_uniforms(vgfx_shader_stages stage, int ub_index, const void* data, int num_bytes) = 0;
	virtual void draw(int base_element, int num_elements, int num_instances) = 0;
	virtual void end_pass() = 0;
	virtual void commit() = 0;
vp_end(vp_gfx);
#endif

#ifdef VP_C_INTERFACE
typedef struct vp_gfx vp_gfx;

// Initializer
VP_EXTERN_C VP_API vp_gfx* vp_gfx_create(const vgfx_prop* prop, vp_app* app);

// Deinitializer
VP_EXTERN_C VP_API void vp_gfx_shutdown(vp_gfx* gfx);

// Info getter
VP_EXTERN_C VP_API vgfx_prop vp_gfx_query_props(vp_gfx* gfx);
VP_EXTERN_C VP_API vgfx_backend vp_gfx_query_backend(vp_gfx* gfx);
VP_EXTERN_C VP_API vgfx_features vp_gfx_query_features(vp_gfx* gfx);
VP_EXTERN_C VP_API vgfx_limits vp_gfx_query_limits(vp_gfx* gfx);
VP_EXTERN_C VP_API vgfx_pixelformat_info vp_gfx_query_pixel_format(vp_gfx* gfx, vgfx_pixel_format fmt);

// Resource creation
VP_EXTERN_C VP_API vgfx_buffer   vp_gfx_make_buffer(vp_gfx* gfx, const vgfx_buffer_prop* prop);
VP_EXTERN_C VP_API vgfx_image    vp_gfx_make_image(vp_gfx* gfx, const vgfx_image_prop* prop);
VP_EXTERN_C VP_API vgfx_shader   vp_gfx_make_shader(vp_gfx* gfx, const vgfx_shader_prop* prop);
VP_EXTERN_C VP_API vgfx_pipeline vp_gfx_make_pipeline(vp_gfx* gfx, const vgfx_pipeline_prop* prop);
VP_EXTERN_C VP_API vgfx_pass     vp_gfx_make_pass(vp_gfx* gfx, const vgfx_pass_prop* prop);

// Resource destruction
VP_EXTERN_C VP_API void vp_gfx_destroy_buffer(vp_gfx* gfx, vgfx_buffer buf);
VP_EXTERN_C VP_API void vp_gfx_destroy_image(vp_gfx* gfx, vgfx_image img);
VP_EXTERN_C VP_API void vp_gfx_destroy_shader(vp_gfx* gfx, vgfx_shader shd);
VP_EXTERN_C VP_API void vp_gfx_destroy_pipeline(vp_gfx* gfx, vgfx_pipeline pip);
VP_EXTERN_C VP_API void vp_gfx_destroy_pass(vp_gfx* gfx, vgfx_pass pass);

// Updating functions
VP_EXTERN_C VP_API void vp_gfx_update_buffer(vp_gfx* gfx, vgfx_buffer buf_id, const void* data_ptr, int data_size);
VP_EXTERN_C VP_API void vp_gfx_update_image(vp_gfx* gfx, vgfx_image img_id, const vgfx_image_content* data);
VP_EXTERN_C VP_API int vp_gfx_append_buffer(vp_gfx* gfx, vgfx_buffer buf, const void* data_ptr, int data_size);
VP_EXTERN_C VP_API bool vp_gfx_query_buffer_overflow(vp_gfx* gfx, vgfx_buffer buf);

// Rendering functions
VP_EXTERN_C VP_API void vp_gfx_begin_default_pass(vp_gfx* gfx, const vgfx_pass_action* act, int width, int height);
VP_EXTERN_C VP_API void vp_gfx_begin_pass(vp_gfx* gfx, vgfx_pass pass_id, const vgfx_pass_action* act);
VP_EXTERN_C VP_API void vp_gfx_apply_viewport(vp_gfx* gfx, int x, int y, int width, int height, bool origin_top_left);
VP_EXTERN_C VP_API void vp_gfx_apply_scissor_rect(vp_gfx* gfx, int x, int y, int width, int height, bool origin_top_left);
VP_EXTERN_C VP_API void vp_gfx_apply_bindings(vp_gfx* gfx, const vgfx_bindings* bindings);
VP_EXTERN_C VP_API void vp_gfx_apply_pipeline(vp_gfx* gfx, vgfx_pipeline pip_id);
VP_EXTERN_C VP_API void vp_gfx_apply_uniforms(vp_gfx* gfx, vgfx_shader_stages stage, int ub_index, const void* data, int num_bytes);
VP_EXTERN_C VP_API void vp_gfx_draw(vp_gfx* gfx, int base_element, int num_elements, int num_instances);
VP_EXTERN_C VP_API void vp_gfx_end_pass(vp_gfx* gfx);
VP_EXTERN_C VP_API void vp_gfx_commit(vp_gfx* gfx);
#endif
