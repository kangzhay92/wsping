struct d3d11_backend
{
	bool valid;
	ID3D11Device* dev;
	ID3D11DeviceContext* ctx;

	vp_function(rtv_cb, const void*);
	vp_function(dsv_cb, const void*);

	bool in_pass;
	bool use_indexed_draw;
	int cur_width;
	int cur_height;
	int num_rtvs;

	native_pass* cur_pass;
	vgfx_pass cur_pass_id;
	native_pipeline* cur_pipeline;
	vgfx_pipeline cur_pipeline_id;

	ID3D11RenderTargetView* cur_rtvs[VGFX_MAX_COLOR_ATTACHMENTS];
	ID3D11DepthStencilView* cur_dsv;

	HINSTANCE d3dcompiler_dll;
	bool d3dcompiler_dll_load_failed;
	pD3DCompile D3DCompile_func;

	ID3D11RenderTargetView* zero_rtvs[VGFX_MAX_COLOR_ATTACHMENTS];
	ID3D11Buffer* zero_vbs[VGFX_MAX_SHADERSTAGE_BUFFERS];
	UINT zero_vb_offsets[VGFX_MAX_SHADERSTAGE_BUFFERS];
	UINT zero_vb_strides[VGFX_MAX_SHADERSTAGE_BUFFERS];
	ID3D11Buffer* zero_cbs[VGFX_MAX_SHADERSTAGE_UBS];
	ID3D11ShaderResourceView* zero_srvs[VGFX_MAX_SHADERSTAGE_IMAGES];
	ID3D11SamplerState* zero_smps[VGFX_MAX_SHADERSTAGE_IMAGES];

	D3D11_SUBRESOURCE_DATA subres_data[VGFX_MAX_MIPMAPS * VGFX_MAX_TEXTUREARRAY_LAYERS];
};

d3d11_backend _d3d11;

struct native_buffer
{
	vgfx_slot slot;
	vgfx_buffer_common cmn;
	struct {
		ID3D11Buffer* buf;
	} d3d11;
};

struct native_image
{
	vgfx_slot slot;
	vgfx_image_common cmn;
	struct {
		DXGI_FORMAT format;
		ID3D11Texture2D* tex2d;
		ID3D11Texture3D* tex3d;
		ID3D11Texture2D* texds;
		ID3D11Texture2D* texmsaa;
		ID3D11ShaderResourceView* srv;
		ID3D11SamplerState* smp;
	} d3d11;
};

struct d3d11_shader_attr
{
	vp_str sem_name;
	int sem_index;
};

struct d3d11_shader_stage
{
	ID3D11Buffer* cbufs[VGFX_MAX_SHADERSTAGE_UBS];
};

struct native_shader
{
	vgfx_slot slot;
	vgfx_shader_common cmn;
	struct {
		d3d11_shader_attr attrs[VGFX_MAX_VERTEX_ATTRIBUTES];
		d3d11_shader_stage stage[VGFX_NUM_SHADER_STAGES];
		ID3D11VertexShader* vs;
		ID3D11PixelShader* fs;
		void* vs_blob;
		int vs_blob_length;
	} d3d11;
};

struct native_pipeline
{
	vgfx_slot slot;
	vgfx_pipeline_common cmn;
	native_shader* shader;
	struct {
		UINT stencil_ref;
		UINT vb_strides[VGFX_MAX_SHADERSTAGE_BUFFERS];
		D3D_PRIMITIVE_TOPOLOGY topology;
		DXGI_FORMAT index_format;
		ID3D11InputLayout* il;
		ID3D11RasterizerState* rs;
		ID3D11DepthStencilState* dss;
		ID3D11BlendState* bs;
	} d3d11;
};

struct d3d11_color_attachment
{
	native_image* image;
	ID3D11RenderTargetView* rtv;
};

struct d3d11_ds_attachment
{
	native_image* image;
	ID3D11DepthStencilView* dsv;
};

struct native_pass
{
	vgfx_slot slot;
	pass_common cmn;
	struct {
		d3d11_color_attachment color_atts[VGFX_MAX_COLOR_ATTACHMENTS];
		d3d11_ds_attachment ds_att;
	} d3d11;
};
;

struct native_context
{
	vgfx_slot slot;
};

static const char* VGFX_D3D11_BUFFER_LOG_OBJECT = "d3d11 buffer";
static const char* VGFX_D3D11_IMAGE_LOG_OBJECT = "d3d11 image";
static const char* VGFX_D3D11_SHADER_LOG_OBJECT = "d3d11 shader";
static const char* VGFX_D3D11_PIPELINE_LOG_OBJECT = "d3d11 pipeline";
static const char* VGFX_D3D11_PASS_LOG_OBJECT = "d3d11 pass";
static const char* VGFX_D3D11_CONTEXT_LOG_OBJECT = "d3d11 context";
static const char* VGFX_D3D11_BACKEND_LOG_OBJECT = "d3d11 backend";

DXGI_FORMAT d3d11_pixel_format(vgfx_pixel_format fmt)
{
	switch (fmt) {
	case vgfx_pixel_format_r8:             return DXGI_FORMAT_R8_UNORM;
	case vgfx_pixel_format_r8sn:           return DXGI_FORMAT_R8_SNORM;
	case vgfx_pixel_format_r8ui:           return DXGI_FORMAT_R8_UINT;
	case vgfx_pixel_format_r8si:           return DXGI_FORMAT_R8_SINT;
	case vgfx_pixel_format_r16:            return DXGI_FORMAT_R16_UNORM;
	case vgfx_pixel_format_r16sn:          return DXGI_FORMAT_R16_SNORM;
	case vgfx_pixel_format_r16ui:          return DXGI_FORMAT_R16_UINT;
	case vgfx_pixel_format_r16si:          return DXGI_FORMAT_R16_SINT;
	case vgfx_pixel_format_r16f:           return DXGI_FORMAT_R16_FLOAT;
	case vgfx_pixel_format_rgba8:          return DXGI_FORMAT_R8G8B8A8_UNORM;
	case vgfx_pixel_format_rgba8sn:        return DXGI_FORMAT_R8G8B8A8_SNORM;
	case vgfx_pixel_format_rgba8ui:        return DXGI_FORMAT_R8G8B8A8_UINT;
	case vgfx_pixel_format_rgba8si:        return DXGI_FORMAT_R8G8B8A8_SINT;
	case vgfx_pixel_format_bgra8:          return DXGI_FORMAT_B8G8R8A8_UNORM;
	case vgfx_pixel_format_rgb10a2:        return DXGI_FORMAT_R10G10B10A2_UNORM;
	case vgfx_pixel_format_rg11b10f:       return DXGI_FORMAT_R11G11B10_FLOAT;
	case vgfx_pixel_format_rg32ui:         return DXGI_FORMAT_R32G32_UINT;
	case vgfx_pixel_format_rg32si:         return DXGI_FORMAT_R32G32_SINT;
	case vgfx_pixel_format_rg32f:          return DXGI_FORMAT_R32G32_FLOAT;
	case vgfx_pixel_format_rgba16:         return DXGI_FORMAT_R16G16B16A16_UNORM;
	case vgfx_pixel_format_rgba16sn:       return DXGI_FORMAT_R16G16B16A16_SNORM;
	case vgfx_pixel_format_rgba16ui:       return DXGI_FORMAT_R16G16B16A16_UINT;
	case vgfx_pixel_format_rgba16si:       return DXGI_FORMAT_R16G16B16A16_SINT;
	case vgfx_pixel_format_rgba16f:        return DXGI_FORMAT_R16G16B16A16_FLOAT;
	case vgfx_pixel_format_rgba32ui:       return DXGI_FORMAT_R32G32B32A32_UINT;
	case vgfx_pixel_format_rgba32si:       return DXGI_FORMAT_R32G32B32A32_SINT;
	case vgfx_pixel_format_rgba32f:        return DXGI_FORMAT_R32G32B32A32_FLOAT;
	case vgfx_pixel_format_depth:          return DXGI_FORMAT_D32_FLOAT;
	case vgfx_pixel_format_depth_stencil:  return DXGI_FORMAT_D24_UNORM_S8_UINT;
	case vgfx_pixel_format_bc1_rgba:       return DXGI_FORMAT_BC1_UNORM;
	case vgfx_pixel_format_bc2_rgba:       return DXGI_FORMAT_BC2_UNORM;
	case vgfx_pixel_format_bc3_rgba:       return DXGI_FORMAT_BC3_UNORM;
	case vgfx_pixel_format_bc4_r:          return DXGI_FORMAT_BC4_UNORM;
	case vgfx_pixel_format_bc4_rsn:        return DXGI_FORMAT_BC4_SNORM;
	case vgfx_pixel_format_bc5_rg:         return DXGI_FORMAT_BC5_UNORM;
	case vgfx_pixel_format_bc5_rgsn:       return DXGI_FORMAT_BC5_SNORM;
	case vgfx_pixel_format_bc6h_rgbf:      return DXGI_FORMAT_BC6H_SF16;
	case vgfx_pixel_format_bc6h_rgbuf:     return DXGI_FORMAT_BC6H_UF16;
	case vgfx_pixel_format_bc7_rgba:       return DXGI_FORMAT_BC7_UNORM;
	}
	return DXGI_FORMAT_UNKNOWN;
}

static D3D11_USAGE d3d11_usage(vgfx_usage usg)
{
	switch (usg) {
	case vgfx_usage_immutable:
		return D3D11_USAGE_IMMUTABLE;
	case vgfx_usage_dynamic:
	case vgfx_usage_stream:
		return D3D11_USAGE_DYNAMIC;
	}
	return D3D11_USAGE_DEFAULT;
}

static UINT d3d11_cpu_access_flags(vgfx_usage usg)
{
	switch (usg) {
	case vgfx_usage_immutable:
		return 0;
	case vgfx_usage_dynamic:
	case vgfx_usage_stream:
		return D3D11_CPU_ACCESS_WRITE;
	}
	return 0;
}

static DXGI_FORMAT d3d11_index_format(vgfx_index_type index_type)
{
	switch (index_type) {
	case vgfx_index_type_none:   return DXGI_FORMAT_UNKNOWN;
	case vgfx_index_type_uint16: return DXGI_FORMAT_R16_UINT;
	case vgfx_index_type_uint32: return DXGI_FORMAT_R32_UINT;
	}
	return DXGI_FORMAT_UNKNOWN;
}

static DXGI_FORMAT d3d11_vertex_format(vgfx_vertex_format fmt)
{
	switch (fmt) {
	case vgfx_vertex_format_float:     return DXGI_FORMAT_R32_FLOAT;
	case vgfx_vertex_format_float2:    return DXGI_FORMAT_R32G32_FLOAT;
	case vgfx_vertex_format_float3:    return DXGI_FORMAT_R32G32B32_FLOAT;
	case vgfx_vertex_format_float4:    return DXGI_FORMAT_R32G32B32A32_FLOAT;
	case vgfx_vertex_format_byte4:     return DXGI_FORMAT_R8G8B8A8_SINT;
	case vgfx_vertex_format_byte4n:    return DXGI_FORMAT_R8G8B8A8_SNORM;
	case vgfx_vertex_format_ubyte4:    return DXGI_FORMAT_R8G8B8A8_UINT;
	case vgfx_vertex_format_ubyte4n:   return DXGI_FORMAT_R8G8B8A8_UNORM;
	case vgfx_vertex_format_short2:    return DXGI_FORMAT_R16G16_SINT;
	case vgfx_vertex_format_short2n:   return DXGI_FORMAT_R16G16_SNORM;
	case vgfx_vertex_format_ushort2n:  return DXGI_FORMAT_R16G16_UNORM;
	case vgfx_vertex_format_short4:    return DXGI_FORMAT_R16G16B16A16_SINT;
	case vgfx_vertex_format_short4n:   return DXGI_FORMAT_R16G16B16A16_SNORM;
	case vgfx_vertex_format_ushort4n:  return DXGI_FORMAT_R16G16B16A16_UNORM;
	case vgfx_vertex_format_uint10n2:  return DXGI_FORMAT_R10G10B10A2_UNORM;
	}
	return DXGI_FORMAT_UNKNOWN;
}

static D3D11_CULL_MODE d3d11_vgfx_cull_mode(vgfx_cull_mode m)
{
	switch (m) {
	case vgfx_cull_mode_none:  return D3D11_CULL_NONE;
	case vgfx_cull_mode_front: return D3D11_CULL_FRONT;
	case vgfx_cull_mode_back:  return D3D11_CULL_BACK;
	}
	return (D3D11_CULL_MODE)0;
}

static D3D11_COMPARISON_FUNC d3d11_compare_func(vgfx_compare_func f)
{
	switch (f) {
	case vgfx_compare_func_never:         return D3D11_COMPARISON_NEVER;
	case vgfx_compare_func_less:          return D3D11_COMPARISON_LESS;
	case vgfx_compare_func_equal:         return D3D11_COMPARISON_EQUAL;
	case vgfx_compare_func_less_equal:    return D3D11_COMPARISON_LESS_EQUAL;
	case vgfx_compare_func_greater:       return D3D11_COMPARISON_GREATER;
	case vgfx_compare_func_not_equal:     return D3D11_COMPARISON_NOT_EQUAL;
	case vgfx_compare_func_greater_equal: return D3D11_COMPARISON_GREATER_EQUAL;
	case vgfx_compare_func_always:        return D3D11_COMPARISON_ALWAYS;
	}
	return (D3D11_COMPARISON_FUNC)0;
}

static D3D11_STENCIL_OP d3d11_stencil_op(vgfx_stencil_op op)
{
	switch (op) {
	case vgfx_stencil_op_keep:       return D3D11_STENCIL_OP_KEEP;
	case vgfx_stencil_op_zero:       return D3D11_STENCIL_OP_ZERO;
	case vgfx_stencil_op_replace:    return D3D11_STENCIL_OP_REPLACE;
	case vgfx_stencil_op_incr_clamp: return D3D11_STENCIL_OP_INCR_SAT;
	case vgfx_stencil_op_decr_clamp: return D3D11_STENCIL_OP_DECR_SAT;
	case vgfx_stencil_op_invert:     return D3D11_STENCIL_OP_INVERT;
	case vgfx_stencil_op_incr_wrap:  return D3D11_STENCIL_OP_INCR;
	case vgfx_stencil_op_decr_wrap:  return D3D11_STENCIL_OP_DECR;
	}
	return (D3D11_STENCIL_OP)0;
}

static D3D11_INPUT_CLASSIFICATION d3d11_input_classification(vgfx_vertex_step step)
{
	switch (step) {
	case vgfx_vertex_step_per_vertex:   return D3D11_INPUT_PER_VERTEX_DATA;
	case vgfx_vertex_step_per_instance: return D3D11_INPUT_PER_INSTANCE_DATA;
	}
	return (D3D11_INPUT_CLASSIFICATION)0;
}

static D3D11_PRIMITIVE_TOPOLOGY d3d11_primitive_topology(vgfx_primitive_type prim_type)
{
	switch (prim_type) {
	case vgfx_primitive_type_points:         return D3D11_PRIMITIVE_TOPOLOGY_POINTLIST;
	case vgfx_primitive_type_lines:          return D3D11_PRIMITIVE_TOPOLOGY_LINELIST;
	case vgfx_primitive_type_line_strip:     return D3D11_PRIMITIVE_TOPOLOGY_LINESTRIP;
	case vgfx_primitive_type_triangles:      return D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST;
	case vgfx_primitive_type_triangle_strip: return D3D11_PRIMITIVE_TOPOLOGY_TRIANGLESTRIP;
	}
	return (D3D11_PRIMITIVE_TOPOLOGY)0;
}

static D3D11_BLEND d3d11_blend_factor(vgfx_blend_factor f)
{
	switch (f) {
	case vgfx_blend_factor_zero:                  return D3D11_BLEND_ZERO;
	case vgfx_blend_factor_one:                   return D3D11_BLEND_ONE;
	case vgfx_blend_factor_src_color:             return D3D11_BLEND_SRC_COLOR;
	case vgfx_blend_factor_one_minus_src_color:   return D3D11_BLEND_INV_SRC_COLOR;
	case vgfx_blend_factor_src_alpha:             return D3D11_BLEND_SRC_ALPHA;
	case vgfx_blend_factor_one_minus_src_alpha:   return D3D11_BLEND_INV_SRC_ALPHA;
	case vgfx_blend_factor_dst_color:             return D3D11_BLEND_DEST_COLOR;
	case vgfx_blend_factor_one_minus_dst_color:   return D3D11_BLEND_INV_DEST_COLOR;
	case vgfx_blend_factor_dst_alpha:             return D3D11_BLEND_DEST_ALPHA;
	case vgfx_blend_factor_one_minus_dst_alpha:   return D3D11_BLEND_INV_DEST_ALPHA;
	case vgfx_blend_factor_src_alpha_saturated:   return D3D11_BLEND_SRC_ALPHA_SAT;
	case vgfx_blend_factor_blend_color:           return D3D11_BLEND_BLEND_FACTOR;
	case vgfx_blend_factor_one_minus_blend_color: return D3D11_BLEND_INV_BLEND_FACTOR;
	case vgfx_blend_factor_blend_alpha:           return D3D11_BLEND_BLEND_FACTOR;
	case vgfx_blend_factor_one_minus_blend_alpha: return D3D11_BLEND_INV_BLEND_FACTOR;
	}
	return (D3D11_BLEND)0;
}

static D3D11_BLEND_OP d3d11_blend_op(vgfx_blend_op op)
{
	switch (op) {
	case vgfx_blend_op_add:              return D3D11_BLEND_OP_ADD;
	case vgfx_blend_op_subtract:         return D3D11_BLEND_OP_SUBTRACT;
	case vgfx_blend_op_reverse_subtract: return D3D11_BLEND_OP_REV_SUBTRACT;
	}
	return (D3D11_BLEND_OP)0;
}

static D3D11_FILTER d3d11_filter(vgfx_filter minf, vgfx_filter magf, unsigned max_anisotropy)
{
	if (max_anisotropy > 1) {
		return D3D11_FILTER_ANISOTROPIC;
	} else if (magf == vgfx_filter_nearest) {
		switch (minf) {
		case vgfx_filter_nearest:
		case vgfx_filter_nearest_mipmap_nearest:
			return D3D11_FILTER_MIN_MAG_MIP_POINT;
		case vgfx_filter_linear:
		case vgfx_filter_linear_mipmap_nearest:
			return D3D11_FILTER_MIN_LINEAR_MAG_MIP_POINT;
		case vgfx_filter_nearest_mipmap_linear:
			return D3D11_FILTER_MIN_MAG_POINT_MIP_LINEAR;
		case vgfx_filter_linear_mipmap_linear:
			return D3D11_FILTER_MIN_LINEAR_MAG_POINT_MIP_LINEAR;
		default:
			break;
		}
	} else if (magf == vgfx_filter_linear) {
		switch (minf) {
		case vgfx_filter_nearest:
		case vgfx_filter_nearest_mipmap_nearest:
			return D3D11_FILTER_MIN_POINT_MAG_LINEAR_MIP_POINT;
		case vgfx_filter_linear:
		case vgfx_filter_linear_mipmap_nearest:
			return D3D11_FILTER_MIN_MAG_LINEAR_MIP_POINT;
		case vgfx_filter_nearest_mipmap_linear:
			return D3D11_FILTER_MIN_POINT_MAG_MIP_LINEAR;
		case vgfx_filter_linear_mipmap_linear:
			return D3D11_FILTER_MIN_MAG_MIP_LINEAR;
		default:
			break;
		}
	}
	return D3D11_FILTER_MIN_MAG_LINEAR_MIP_POINT;
}

static D3D11_TEXTURE_ADDRESS_MODE d3d11_address_mode(vgfx_wrap m)
{
	switch (m) {
	case vgfx_wrap_repeat:           return D3D11_TEXTURE_ADDRESS_WRAP;
	case vgfx_wrap_clamp_to_edge:    return D3D11_TEXTURE_ADDRESS_CLAMP;
	case vgfx_wrap_clamp_to_border:  return D3D11_TEXTURE_ADDRESS_BORDER;
	case vgfx_wrap_mirrored_repeat:  return D3D11_TEXTURE_ADDRESS_MIRROR;
	}
	return (D3D11_TEXTURE_ADDRESS_MODE)0;
}

uint8_t d3d11_color_write_mask(uint8_t colormask)
{
	uint8_t res = 0;
	if (colormask & VGFX_COLORMASK_R) {
		res |= D3D11_COLOR_WRITE_ENABLE_RED;
	}
	if (colormask & VGFX_COLORMASK_G) {
		res |= D3D11_COLOR_WRITE_ENABLE_GREEN;
	}
	if (colormask & VGFX_COLORMASK_B) {
		res |= D3D11_COLOR_WRITE_ENABLE_BLUE;
	}
	if (colormask & VGFX_COLORMASK_A) {
		res |= D3D11_COLOR_WRITE_ENABLE_ALPHA;
	}
	return res;
}

enum d3d11_error_context
{
	d3d11_err_unallocated,
	d3d11_err_uninitialized,
	d3d11_err_initialized,
	d3d11_err_create_failed,
	d3d11_err_invalid
};

static void d3d11_error_msg(const char* msg)
{
	vp_gfx_impl::instance._app->fail(msg);
}

static void d3d11_error(const char* object, d3d11_error_context ctx)
{
	const char* ex_title = "ViperGFX Direct3D11 exception";
	const char* err_ctx = "";
	char outbuf[256];

	switch (ctx) {
		case d3d11_err_unallocated:
			err_ctx = "unallocated";
			break;
		case d3d11_err_uninitialized: 
			err_ctx = "uninitialized"; 
			break;
		case d3d11_err_initialized:
			err_ctx = "initialized";
			break;
		case d3d11_err_create_failed:
			err_ctx = "creation failed";
			break;
	}

#ifdef _MSC_VER
	if (ctx == d3d11_err_invalid) {
		sprintf_s(outbuf, "%s, invalid %s", ex_title, object);
	} else if (ctx == d3d11_err_create_failed) {
		sprintf_s(outbuf, "%s, %s creation failed", ex_title, object);
	} else {
		sprintf_s(outbuf, "%s, %s was %s", ex_title, object, err_ctx);
	}
#else
	if (ctx == d3d11_err_invalid) {
		sprintf(outbuf, "%s, invalid %s", ex_title, object);
	} else {
		sprintf(outbuf, "%s, %s was %s", ex_title, object, err_ctx);
	}
#endif
	d3d11_error_msg(outbuf);
}

static void d3d11_init_caps()
{
	vp_gfx_impl::instance._backend = vgfx_backend_d3d11;

	vp_gfx_impl::instance._features.instancing = true;
	vp_gfx_impl::instance._features.origin_top_left = true;
	vp_gfx_impl::instance._features.multiple_render_targets = true;
	vp_gfx_impl::instance._features.msaa_render_targets = true;
	vp_gfx_impl::instance._features.imagetype_3d = true;
	vp_gfx_impl::instance._features.imagetype_array = true;
	vp_gfx_impl::instance._features.image_clamp_to_border = true;

	vp_gfx_impl::instance._limits.max_image_size_2d = 16 * 1024;
	vp_gfx_impl::instance._limits.max_image_size_cube = 16 * 1024;
	vp_gfx_impl::instance._limits.max_image_size_3d = 2 * 1024;
	vp_gfx_impl::instance._limits.max_image_size_array = 16 * 1024;
	vp_gfx_impl::instance._limits.max_image_array_layers = 2 * 1024;
	vp_gfx_impl::instance._limits.max_vertex_attrs = VGFX_MAX_VERTEX_ATTRIBUTES;

	UINT dxgi_fmt_caps = 0;
	for (int fmt = (vgfx_pixel_format_none + 1); fmt < (VGFX_NUM_PIXEL_FORMAT); fmt++) {
		DXGI_FORMAT dxgi_fmt = d3d11_pixel_format((vgfx_pixel_format)fmt);
		HRESULT hr = _d3d11.dev->CheckFormatSupport(dxgi_fmt, &dxgi_fmt_caps);
		if (FAILED(hr)) {
			d3d11_error_msg("Failed to check d3d11 pixel format support");	
		}
		vgfx_pixelformat_info* info = &vp_gfx_impl::instance._formats[fmt];
		info->sample = (dxgi_fmt_caps & D3D11_FORMAT_SUPPORT_TEXTURE2D) != 0;
		info->filter = (dxgi_fmt_caps & D3D11_FORMAT_SUPPORT_SHADER_SAMPLE) != 0;
		info->render = (dxgi_fmt_caps & D3D11_FORMAT_SUPPORT_RENDER_TARGET) != 0;
		info->blend = (dxgi_fmt_caps & D3D11_FORMAT_SUPPORT_BLENDABLE) != 0;
		info->msaa = (dxgi_fmt_caps & D3D11_FORMAT_SUPPORT_MULTISAMPLE_RENDERTARGET) != 0;
		info->depth = (dxgi_fmt_caps & D3D11_FORMAT_SUPPORT_DEPTH_STENCIL) != 0;
		if (info->depth) {
			info->render = true;
		}
	}
}

static void d3d11_clear_state()
{
	_d3d11.ctx->OMSetRenderTargets(VGFX_MAX_COLOR_ATTACHMENTS, _d3d11.zero_rtvs, NULL);
	_d3d11.ctx->RSSetState(NULL);
	_d3d11.ctx->OMSetDepthStencilState(NULL, 0);
	_d3d11.ctx->OMSetBlendState(NULL, NULL, 0xFFFFFFFF);
	_d3d11.ctx->IASetVertexBuffers(0, VGFX_MAX_SHADERSTAGE_BUFFERS, _d3d11.zero_vbs, _d3d11.zero_vb_strides, _d3d11.zero_vb_offsets);
	_d3d11.ctx->IASetIndexBuffer(NULL, DXGI_FORMAT_UNKNOWN, 0);
	_d3d11.ctx->IASetInputLayout(NULL);
	_d3d11.ctx->VSSetShader(NULL, NULL, 0);
	_d3d11.ctx->PSSetShader(NULL, NULL, 0);
	_d3d11.ctx->VSSetConstantBuffers(0, VGFX_MAX_SHADERSTAGE_UBS, _d3d11.zero_cbs);
	_d3d11.ctx->PSSetConstantBuffers(0, VGFX_MAX_SHADERSTAGE_UBS, _d3d11.zero_cbs);
	_d3d11.ctx->VSSetShaderResources(0, VGFX_MAX_SHADERSTAGE_IMAGES, _d3d11.zero_srvs);
	_d3d11.ctx->PSSetShaderResources(0, VGFX_MAX_SHADERSTAGE_IMAGES, _d3d11.zero_srvs);
	_d3d11.ctx->VSSetSamplers(0, VGFX_MAX_SHADERSTAGE_IMAGES, _d3d11.zero_smps);
	_d3d11.ctx->PSSetSamplers(0, VGFX_MAX_SHADERSTAGE_IMAGES, _d3d11.zero_smps);
}

static void d3d11_setup_backend(const vgfx_prop* prop)
{
	if (!prop) {
		d3d11_error("ViperGFX prop", d3d11_err_uninitialized);
	}
	if (!prop->d3d11_device ||
		!prop->d3d11_device_context ||
		!prop->d3d11_render_target_view_cb ||
		!prop->d3d11_depth_stencil_view_cb ||
		(prop->d3d11_render_target_view_cb == prop->d3d11_depth_stencil_view_cb)) {
		d3d11_error("ViperGFX prop", d3d11_err_invalid);
	}
	_d3d11 = {};
	_d3d11.valid = true;
	_d3d11.dev = (ID3D11Device*)prop->d3d11_device;
	_d3d11.ctx = (ID3D11DeviceContext*)prop->d3d11_device_context;
	_d3d11.rtv_cb = prop->d3d11_render_target_view_cb;
	_d3d11.dsv_cb = prop->d3d11_depth_stencil_view_cb;
	d3d11_init_caps();
}

static void d3d11_discard_backend()
{
	if (!_d3d11.valid) {
		d3d11_error(VGFX_D3D11_BACKEND_LOG_OBJECT, d3d11_err_uninitialized);
	}
	_d3d11.valid = false;
}

static vgfx_resource_state d3d11_create_context(native_context* ctx)
{
	if (!ctx) {
		d3d11_error(VGFX_D3D11_CONTEXT_LOG_OBJECT, d3d11_err_uninitialized);
	}
	return vgfx_resource_state_valid;
}

static void d3d11_destroy_context(native_context* ctx)
{
	if (!ctx) {
		d3d11_error(VGFX_D3D11_CONTEXT_LOG_OBJECT, d3d11_err_uninitialized);
	}
}

static void d3d11_activate_context(native_context* ctx)
{
	d3d11_clear_state();
}

static vgfx_resource_state d3d11_create_buffer(native_buffer* buf, const vgfx_buffer_prop* prop)
{
	if (!buf) {
		d3d11_error(VGFX_D3D11_BUFFER_LOG_OBJECT, d3d11_err_unallocated);
	}
	if (!prop) {
		d3d11_error("prop", d3d11_err_uninitialized);
	}
	if (buf->d3d11.buf) {
		d3d11_error(VGFX_D3D11_BUFFER_LOG_OBJECT, d3d11_err_initialized);
	}

	buf->d3d11 = {};
	buffer_common_init(&buf->cmn, prop);

	const bool injected = (prop->d3d11_buffer != NULL);
	if (injected) {
		buf->d3d11.buf = (ID3D11Buffer*)prop->d3d11_buffer;
		buf->d3d11.buf->AddRef();
	} else {
		D3D11_BUFFER_DESC d3d11_prop = {};
		d3d11_prop.ByteWidth = buf->cmn.size;
		d3d11_prop.Usage = d3d11_usage(buf->cmn.usage);
		d3d11_prop.BindFlags = buf->cmn.type == vgfx_buffer_type_vertex_buffer ? D3D11_BIND_VERTEX_BUFFER : D3D11_BIND_INDEX_BUFFER;
		d3d11_prop.CPUAccessFlags = d3d11_cpu_access_flags(buf->cmn.usage);

		D3D11_SUBRESOURCE_DATA* invgfx_index_type_data_ptr = NULL;
		D3D11_SUBRESOURCE_DATA invgfx_index_type_data = {};
		if (buf->cmn.usage == vgfx_usage_immutable) {
			if (!prop->content) {
				d3d11_error("buffer content", d3d11_err_uninitialized);
			}
			invgfx_index_type_data.pSysMem = prop->content;
			invgfx_index_type_data_ptr = &invgfx_index_type_data;
		}

		HRESULT hr = _d3d11.dev->CreateBuffer(&d3d11_prop, invgfx_index_type_data_ptr, &buf->d3d11.buf);
		if (FAILED(hr) || !buf->d3d11.buf) {
			d3d11_error(VGFX_D3D11_BUFFER_LOG_OBJECT, d3d11_err_create_failed);
		}
	}

	return vgfx_resource_state_valid;
}

static void d3d11_destroy_buffer(native_buffer* buf)
{
	if (!buf) {
		d3d11_error(VGFX_D3D11_BUFFER_LOG_OBJECT, d3d11_err_uninitialized);
	}
	if (buf->d3d11.buf) {
		buf->d3d11.buf->Release();
	}
}

static void d3d11_fill_subres_data(const native_image* img, const vgfx_image_content* content)
{
	const int num_faces = (img->cmn.type == vgfx_image_type_cube) ? 6 : 1;
	const int num_slices = (img->cmn.type == vgfx_image_type_array) ? img->cmn.depth : 1;
	int subres_index = 0;
	for (int face_index = 0; face_index < num_faces; face_index++) {
		for (int slice_index = 0; slice_index < num_slices; slice_index++) {
			for (int mip_index = 0; mip_index < img->cmn.num_mipmaps; mip_index++, subres_index++) {
				if (subres_index >= (VGFX_MAX_MIPMAPS * VGFX_MAX_TEXTUREARRAY_LAYERS)) {
					d3d11_error_msg("d3d11 subresource data overflow");
				}
				D3D11_SUBRESOURCE_DATA* subres_data = &_d3d11.subres_data[subres_index];
				const int mip_width = ((img->cmn.width >> mip_index) > 0) ? img->cmn.width >> mip_index : 1;
				const int mip_height = ((img->cmn.height >> mip_index) > 0) ? img->cmn.height >> mip_index : 1;
				const vgfx_subimage_content* subimg_content = &(content->subimage[face_index][mip_index]);
				const int slice_size = subimg_content->size / num_slices;
				const int slice_offset = slice_size * slice_index;
				const uint8_t* ptr = (const uint8_t*)subimg_content->ptr;
				subres_data->pSysMem = ptr + slice_offset;
				subres_data->SysMemPitch = row_pitch(img->cmn.pixel_format, mip_width);
				if (img->cmn.type == vgfx_image_type_3d) {
					subres_data->SysMemSlicePitch = surface_pitch(img->cmn.pixel_format, mip_width, mip_height);
				} else {
					subres_data->SysMemSlicePitch = 0;
				}
			}
		}
	}
}

vgfx_resource_state d3d11_create_image(native_image* img, const vgfx_image_prop* prop)
{
	if (!img) {
		d3d11_error(VGFX_D3D11_IMAGE_LOG_OBJECT, d3d11_err_unallocated);
	}
	if (!prop) {
		d3d11_error("prop", d3d11_err_uninitialized);
	}
	if (img->d3d11.tex2d || img->d3d11.tex3d || img->d3d11.texds || img->d3d11.texmsaa) {
		d3d11_error(VGFX_D3D11_IMAGE_LOG_OBJECT, d3d11_err_initialized);
	}
	if (img->d3d11.srv || img->d3d11.smp) {
		d3d11_error(VGFX_D3D11_IMAGE_LOG_OBJECT, d3d11_err_initialized);
	}

	HRESULT hr;
	const char* err_unsupported = "Trying to create d3d11 texture with unsupported format";
	const char* srv_object = "d3d11 shader resource view";

	img->d3d11 = {};
	image_common_init(&img->cmn, prop);
	const bool injected = (prop->d3d11_texture != NULL);
	const bool msaa = (img->cmn.sample_count > 1);

	if (vp_gfx_impl::instance.is_valid_rendertarget_depth_format(img->cmn.pixel_format)) {
		if (injected) {
			d3d11_error_msg("d3d11 image is not a depth-texture format");
		}
		img->d3d11.format = d3d11_pixel_format(img->cmn.pixel_format);
		if (img->d3d11.format == DXGI_FORMAT_UNKNOWN) {
			vp_gfx_impl::instance._app->log(vapp_log_message_type_error, err_unsupported);
			return vgfx_resource_state_failed;
		}
		D3D11_TEXTURE2D_DESC d3d11_prop = {};
		d3d11_prop.Width = img->cmn.width;
		d3d11_prop.Height = img->cmn.height;
		d3d11_prop.MipLevels = 1;
		d3d11_prop.ArraySize = 1;
		d3d11_prop.Format = img->d3d11.format;
		d3d11_prop.Usage = D3D11_USAGE_DEFAULT;
		d3d11_prop.BindFlags = D3D11_BIND_DEPTH_STENCIL;
		d3d11_prop.SampleDesc.Count = img->cmn.sample_count;
		d3d11_prop.SampleDesc.Quality = msaa ? D3D11_STANDARD_MULTISAMPLE_PATTERN : 0;
		hr = _d3d11.dev->CreateTexture2D(&d3d11_prop, NULL, &img->d3d11.texds);
		if (FAILED(hr) || !img->d3d11.texds) {
			d3d11_error("d3d11 depth texture", d3d11_err_create_failed);
		}
	} else {
		D3D11_SUBRESOURCE_DATA* invgfx_index_type_data = NULL;
		if (!injected && (img->cmn.usage == vgfx_usage_immutable) && !img->cmn.render_target) {
			d3d11_fill_subres_data(img, &prop->content);
			invgfx_index_type_data = _d3d11.subres_data;
		}
		if (img->cmn.type != vgfx_image_type_3d) {
			D3D11_TEXTURE2D_DESC d3d11_tex_prop = {};
			d3d11_tex_prop.Width = img->cmn.width;
			d3d11_tex_prop.Height = img->cmn.height;
			d3d11_tex_prop.MipLevels = img->cmn.num_mipmaps;
			switch (img->cmn.type) {
			case vgfx_image_type_array:
				d3d11_tex_prop.ArraySize = img->cmn.depth;
				break;
			case vgfx_image_type_cube:
				d3d11_tex_prop.ArraySize = 6;
				break;
			default:
				d3d11_tex_prop.ArraySize = 1;
				break;
			}
			d3d11_tex_prop.BindFlags = D3D11_BIND_SHADER_RESOURCE;
			if (img->cmn.render_target) {
				img->d3d11.format = d3d11_pixel_format(img->cmn.pixel_format);
				d3d11_tex_prop.Format = img->d3d11.format;
				d3d11_tex_prop.Usage = D3D11_USAGE_DEFAULT;
				if (!msaa) {
					d3d11_tex_prop.BindFlags |= D3D11_BIND_RENDER_TARGET;
				}
				d3d11_tex_prop.CPUAccessFlags = 0;
			} else {
				img->d3d11.format = d3d11_pixel_format(img->cmn.pixel_format);
				d3d11_tex_prop.Format = img->d3d11.format;
				d3d11_tex_prop.Usage = d3d11_usage(img->cmn.usage);
				d3d11_tex_prop.CPUAccessFlags = d3d11_cpu_access_flags(img->cmn.usage);
			}
			if (img->d3d11.format == DXGI_FORMAT_UNKNOWN) {
				vp_gfx_impl::instance._app->log(vapp_log_message_type_error, err_unsupported);
				return vgfx_resource_state_failed;
			}
			d3d11_tex_prop.SampleDesc.Count = 1;
			d3d11_tex_prop.SampleDesc.Quality = 0;
			d3d11_tex_prop.MiscFlags = (img->cmn.type == vgfx_image_type_cube) ? D3D11_RESOURCE_MISC_TEXTURECUBE : 0;
			if (injected) {
				img->d3d11.tex2d = (ID3D11Texture2D*)prop->d3d11_texture;
				img->d3d11.tex2d->AddRef();
			} else {
				hr = _d3d11.dev->CreateTexture2D(&d3d11_tex_prop, invgfx_index_type_data, &img->d3d11.tex2d);
				if (FAILED(hr) || !img->d3d11.tex2d) {
					d3d11_error("d3d11 2D texture", d3d11_err_create_failed);
				}
			}
			D3D11_SHADER_RESOURCE_VIEW_DESC d3d11_srv_prop = {};
			d3d11_srv_prop.Format = d3d11_tex_prop.Format;
			switch (img->cmn.type) {
			case vgfx_image_type_2d:
				d3d11_srv_prop.ViewDimension = D3D11_SRV_DIMENSION_TEXTURE2D;
				d3d11_srv_prop.Texture2D.MipLevels = img->cmn.num_mipmaps;
				break;
			case vgfx_image_type_cube:
				d3d11_srv_prop.ViewDimension = D3D11_SRV_DIMENSION_TEXTURECUBE;
				d3d11_srv_prop.TextureCube.MipLevels = img->cmn.num_mipmaps;
				break;
			case vgfx_image_type_array:
				d3d11_srv_prop.ViewDimension = D3D11_SRV_DIMENSION_TEXTURE2DARRAY;
				d3d11_srv_prop.Texture2DArray.MipLevels = img->cmn.num_mipmaps;
				d3d11_srv_prop.Texture2DArray.ArraySize = img->cmn.depth;
				break;
			default:
				break;
			}
			hr = _d3d11.dev->CreateShaderResourceView((ID3D11Resource*)img->d3d11.tex2d, &d3d11_srv_prop, &img->d3d11.srv);
			if (FAILED(hr) || !img->d3d11.srv) {
				d3d11_error(srv_object, d3d11_err_create_failed);
			}
		} else {
			D3D11_TEXTURE3D_DESC d3d11_tex_prop = {};
			d3d11_tex_prop.Width = img->cmn.width;
			d3d11_tex_prop.Height = img->cmn.height;
			d3d11_tex_prop.Depth = img->cmn.depth;
			d3d11_tex_prop.MipLevels = img->cmn.num_mipmaps;
			d3d11_tex_prop.BindFlags = D3D11_BIND_SHADER_RESOURCE;
			if (img->cmn.render_target) {
				img->d3d11.format = d3d11_pixel_format(img->cmn.pixel_format);
				d3d11_tex_prop.Format = img->d3d11.format;
				d3d11_tex_prop.Usage = D3D11_USAGE_DEFAULT;
				if (!msaa) {
					d3d11_tex_prop.BindFlags |= D3D11_BIND_RENDER_TARGET;
				}
				d3d11_tex_prop.CPUAccessFlags = 0;
			} else {
				img->d3d11.format = d3d11_pixel_format(img->cmn.pixel_format);
				d3d11_tex_prop.Format = img->d3d11.format;
				d3d11_tex_prop.Usage = d3d11_usage(img->cmn.usage);
				d3d11_tex_prop.CPUAccessFlags = d3d11_cpu_access_flags(img->cmn.usage);
			}
			if (img->d3d11.format == DXGI_FORMAT_UNKNOWN) {
				vp_gfx_impl::instance._app->log(vapp_log_message_type_error, err_unsupported);
				return vgfx_resource_state_failed;
			}
			if (injected) {
				img->d3d11.tex3d = (ID3D11Texture3D*)prop->d3d11_texture;
				img->d3d11.tex3d->AddRef();
			} else {
				hr = _d3d11.dev->CreateTexture3D(&d3d11_tex_prop, invgfx_index_type_data, &img->d3d11.tex3d);
				if (FAILED(hr) || !img->d3d11.tex3d) {
					d3d11_error("d3d11 3D texture", d3d11_err_create_failed);
				}
			}
			D3D11_SHADER_RESOURCE_VIEW_DESC d3d11_srv_prop = {};
			d3d11_srv_prop.Format = d3d11_tex_prop.Format;
			d3d11_srv_prop.ViewDimension = D3D11_SRV_DIMENSION_TEXTURE3D;
			d3d11_srv_prop.Texture3D.MipLevels = img->cmn.num_mipmaps;
			hr = _d3d11.dev->CreateShaderResourceView((ID3D11Resource*)img->d3d11.tex3d, &d3d11_srv_prop, &img->d3d11.srv);
			if (FAILED(hr) || !img->d3d11.srv) {
				d3d11_error(srv_object, d3d11_err_create_failed);
			}
		}
		if (msaa) {
			D3D11_TEXTURE2D_DESC d3d11_tex_prop = {};
			d3d11_tex_prop.Width = img->cmn.width;
			d3d11_tex_prop.Height = img->cmn.height;
			d3d11_tex_prop.MipLevels = 1;
			d3d11_tex_prop.ArraySize = 1;
			d3d11_tex_prop.Format = img->d3d11.format;
			d3d11_tex_prop.Usage = D3D11_USAGE_DEFAULT;
			d3d11_tex_prop.BindFlags = D3D11_BIND_RENDER_TARGET;
			d3d11_tex_prop.CPUAccessFlags = 0;
			d3d11_tex_prop.SampleDesc.Count = img->cmn.sample_count;
			d3d11_tex_prop.SampleDesc.Quality = (UINT)D3D11_STANDARD_MULTISAMPLE_PATTERN;
			hr = _d3d11.dev->CreateTexture2D(&d3d11_tex_prop, NULL, &img->d3d11.texmsaa);
			if (FAILED(hr) || !img->d3d11.texmsaa) {
				d3d11_error("d3d11 multisample texture", d3d11_err_create_failed);
			}
		}
		D3D11_SAMPLER_DESC d3d11_smp_prop = {};
		d3d11_smp_prop.Filter = d3d11_filter(img->cmn.min_filter, img->cmn.mag_filter, img->cmn.max_anisotropy);
		d3d11_smp_prop.AddressU = d3d11_address_mode(img->cmn.wrap_u);
		d3d11_smp_prop.AddressV = d3d11_address_mode(img->cmn.wrap_v);
		d3d11_smp_prop.AddressW = d3d11_address_mode(img->cmn.wrap_w);
		switch (img->cmn.border_color) {
		case vgfx_border_color_transparent_black:
			break;
		case vgfx_border_color_opaque_white:
			for (int i = 0; i < 4; i++) {
				d3d11_smp_prop.BorderColor[i] = 1.0f;
			}
			break;
		default:
			d3d11_smp_prop.BorderColor[3] = 1.0f;
			break;
		}
		d3d11_smp_prop.MaxAnisotropy = img->cmn.max_anisotropy;
		d3d11_smp_prop.ComparisonFunc = D3D11_COMPARISON_NEVER;
		d3d11_smp_prop.MinLOD = prop->min_lod;
		d3d11_smp_prop.MaxLOD = prop->max_lod;
		hr = _d3d11.dev->CreateSamplerState(&d3d11_smp_prop, &img->d3d11.smp);
		if (FAILED(hr) || !img->d3d11.smp) {
			d3d11_error("d3d11 sampler state", d3d11_err_create_failed);
		}
	}

	return vgfx_resource_state_valid;
}

static void d3d11_destroy_image(native_image* img)
{
	if (!img) {
		d3d11_error(VGFX_D3D11_IMAGE_LOG_OBJECT, d3d11_err_uninitialized);
	}
	if (img->d3d11.tex2d) {
		img->d3d11.tex2d->Release();
	}
	if (img->d3d11.tex3d) {
		img->d3d11.tex3d->Release();
	}
	if (img->d3d11.texds) {
		img->d3d11.texds->Release();
	}
	if (img->d3d11.texmsaa) {
		img->d3d11.texmsaa->Release();
	}
	if (img->d3d11.srv) {
		img->d3d11.srv->Release();
	}
	if (img->d3d11.smp) {
		img->d3d11.smp->Release();
	}
}

static bool d3d11_load_d3dcompiler_dll()
{
#if (defined(WINAPI_FAMILY_PARTITION) && !WINAPI_FAMILY_PARTITION(WINAPI_PARTITION_DESKTOP))
	return true;
#else
	if ((_d3d11.d3dcompiler_dll == nullptr) && !_d3d11.d3dcompiler_dll_load_failed) {
		_d3d11.d3dcompiler_dll = LoadLibrary("d3dcompiler_47.dll");
		if (_d3d11.d3dcompiler_dll == nullptr) {
			d3d11_error_msg("Failed to load d3dcompiler_47.dll!");
			_d3d11.d3dcompiler_dll_load_failed = true;
			return false;
		}
		_d3d11.D3DCompile_func = (pD3DCompile)GetProcAddress(_d3d11.d3dcompiler_dll, "D3DCompile");
		if (!_d3d11.D3DCompile_func) {
			d3d11_error_msg("D3DCompile function not found");
		}
	}
	return _d3d11.d3dcompiler_dll != nullptr;
#endif
}

#if (defined(WINAPI_FAMILY_PARTITION) && !WINAPI_FAMILY_PARTITION(WINAPI_PARTITION_DESKTOP))
#define d3d11_D3DCompile D3DCompile
#else
#define d3d11_D3DCompile _d3d11.D3DCompile_func
#endif

static ID3DBlob* d3d11_compile_shader(const vgfx_shader_stage_prop* stage_prop, const char* target)
{
	if (!d3d11_load_d3dcompiler_dll()) {
		return nullptr;
	}
	ID3DBlob* output = NULL;
	ID3DBlob* error_or_warnings = NULL;
	HRESULT hr = d3d11_D3DCompile(
		stage_prop->source,
		strlen(stage_prop->source),
		NULL,
		NULL,
		NULL,
		stage_prop->entry ? stage_prop->entry : "main",
		target,
		D3DCOMPILE_PACK_MATRIX_COLUMN_MAJOR | D3DCOMPILE_OPTIMIZATION_LEVEL3,
		0,
		&output,
		&error_or_warnings
	);
	if (error_or_warnings) {
		vp_gfx_impl::instance._app->log(vapp_log_message_type_error, (LPCSTR)error_or_warnings->GetBufferPointer());
		error_or_warnings->Release();
		error_or_warnings = nullptr;
	}
	if (FAILED(hr)) {
		if (output) {
			output->Release();
			output = nullptr;
		}
	}
	return output;
}

UINT d3d11_roundup(int val, int round_to)
{
	return (((val)+((round_to)-1)) & ~((round_to)-1));
}

static vgfx_resource_state d3d11_create_shader(native_shader* shd, const vgfx_shader_prop* prop)
{
	if (!shd) {
		d3d11_error(VGFX_D3D11_SHADER_LOG_OBJECT, d3d11_err_unallocated);
	}
	if (!prop) {
		d3d11_error("prop", d3d11_err_uninitialized);
	}
	if (shd->d3d11.vs || shd->d3d11.fs || shd->d3d11.vs_blob) {
		d3d11_error(VGFX_D3D11_SHADER_LOG_OBJECT, d3d11_err_initialized);
	}

	const char* cbuffer_obj = "d3d11 shader constant buffer";

	shd->d3d11 = {};
	shader_common_init(&shd->cmn, prop);

	for (int i = 0; i < VGFX_MAX_VERTEX_ATTRIBUTES; i++) {
		vp_strcpy(&shd->d3d11.attrs[i].sem_name, prop->attrs[i].sem_name);
		shd->d3d11.attrs[i].sem_index = prop->attrs[i].sem_index;
	}

	for (int i = 0; i < VGFX_NUM_SHADER_STAGES; i++) {
		vgfx_shader_stage* cmn_stage = &shd->cmn.stage[i];
		d3d11_shader_stage* d3d11_stage = &shd->d3d11.stage[i];
		for (int j = 0; j < cmn_stage->num_uniform_blocks; j++) {
			const vgfx_uniform_block* ub = &cmn_stage->uniform_blocks[j];
			if (d3d11_stage->cbufs[j] != NULL) {
				d3d11_error(cbuffer_obj, d3d11_err_initialized);
			}
			D3D11_BUFFER_DESC cb_prop = {};
			cb_prop.ByteWidth = d3d11_roundup(ub->size, 16);
			cb_prop.Usage = D3D11_USAGE_DEFAULT;
			cb_prop.BindFlags = D3D11_BIND_CONSTANT_BUFFER;
			HRESULT hr = _d3d11.dev->CreateBuffer(&cb_prop, NULL, &d3d11_stage->cbufs[j]);
			if (FAILED(hr) || !d3d11_stage->cbufs[j]) {
				d3d11_error(cbuffer_obj, d3d11_err_create_failed);
			}
		}
	}

	const void* vs_ptr = NULL;
	const void* fs_ptr = NULL;
	SIZE_T vs_length = 0;
	SIZE_T fs_length = 0;
	ID3DBlob* vs_blob = NULL;
	ID3DBlob* fs_blob = NULL;

	if (prop->vs.byte_code && prop->fs.byte_code) {
		vs_ptr = prop->vs.byte_code;
		fs_ptr = prop->fs.byte_code;
		vs_length = prop->vs.byte_code_size;
		fs_length = prop->fs.byte_code_size;
	} else {
		vs_blob = d3d11_compile_shader(&prop->vs, "vs_5_0");
		fs_blob = d3d11_compile_shader(&prop->fs, "ps_5_0");
		if (vs_blob && fs_blob) {
			vs_ptr = vs_blob->GetBufferPointer();
			vs_length = vs_blob->GetBufferSize();
			fs_ptr = fs_blob->GetBufferPointer();
			fs_length = fs_blob->GetBufferSize();
		}
	}

	vgfx_resource_state result = vgfx_resource_state_failed;
	if (vs_ptr && fs_ptr && (vs_length > 0) && (fs_length > 0)) {
		HRESULT hr = _d3d11.dev->CreateVertexShader(vs_ptr, vs_length, NULL, &shd->d3d11.vs);
		if (FAILED(hr) || !shd->d3d11.vs) {
			d3d11_error("d3d11 vertex shader", d3d11_err_create_failed);
		}
		hr = _d3d11.dev->CreatePixelShader(fs_ptr, fs_length, NULL, &shd->d3d11.fs);
		if (FAILED(hr) || !shd->d3d11.fs) {
			d3d11_error("d3d11 pixel shader", d3d11_err_create_failed);
		}
		shd->d3d11.vs_blob_length = (int)vs_length;
		shd->d3d11.vs_blob = malloc(vs_length);
		if (!shd->d3d11.vs_blob) {
			d3d11_error("d3d11 vertex shader blob", d3d11_err_unallocated);
		}
		memcpy(shd->d3d11.vs_blob, vs_ptr, vs_length);
		result = vgfx_resource_state_valid;
	}
	if (vs_blob) {
		vs_blob->Release();
		vs_blob = nullptr;
	}
	if (fs_blob) {
		fs_blob->Release();
		fs_blob = nullptr;
	}
	return result;
}

static void d3d11_destroy_shader(native_shader* shd)
{
	if (!shd) {
		d3d11_error(VGFX_D3D11_SHADER_LOG_OBJECT, d3d11_err_uninitialized);
	}
	if (shd->d3d11.vs) {
		shd->d3d11.vs->Release();
	}
	if (shd->d3d11.fs) {
		shd->d3d11.fs->Release();
	}
	if (shd->d3d11.vs_blob) {
		free(shd->d3d11.vs_blob);
	}
	for (int i = 0; i < VGFX_NUM_SHADER_STAGES; i++) {
		auto cmn_stage = &shd->cmn.stage[i];
		auto d3d11_stage = &shd->d3d11.stage[i];
		for (int j = 0; j < cmn_stage->num_uniform_blocks; j++) {
			if (d3d11_stage->cbufs[j]) {
				d3d11_stage->cbufs[j]->Release();
			}
		}
	}
}

vgfx_resource_state d3d11_create_pipeline(native_pipeline* pip, native_shader* shd, const vgfx_pipeline_prop* prop)
{
	if (!pip) {
		d3d11_error(VGFX_D3D11_PIPELINE_LOG_OBJECT, d3d11_err_unallocated);
	}
	if (!shd) {
		d3d11_error(VGFX_D3D11_SHADER_LOG_OBJECT, d3d11_err_uninitialized);
	}
	if (!prop) {
		d3d11_error("pipeline prop", d3d11_err_uninitialized);
	}
	if (prop->shader.id != shd->slot.id) {
		d3d11_error("shader id", d3d11_err_invalid);
	}
	if (shd->slot.state != vgfx_resource_state_valid) {
		d3d11_error("resource state", d3d11_err_invalid);
	}
	if (!shd->d3d11.vs_blob || shd->d3d11.vs_blob_length <= 0) {
		d3d11_error("d3d11 vertex shader blob", d3d11_err_initialized);
	}
	if (pip->d3d11.il || pip->d3d11.rs || pip->d3d11.dss || pip->d3d11.bs) {
		d3d11_error(VGFX_D3D11_PIPELINE_LOG_OBJECT, d3d11_err_initialized);
	}

	pip->d3d11 = {};
	pip->shader = shd;
	pipeline_common_init(&pip->cmn, prop);
	pip->d3d11.index_format = d3d11_index_format(pip->cmn.index_type);
	pip->d3d11.topology = d3d11_primitive_topology(prop->primitive_type);
	pip->d3d11.stencil_ref = prop->depth_stencil.stencil_ref;

	D3D11_INPUT_ELEMENT_DESC d3d11_comps[VGFX_MAX_VERTEX_ATTRIBUTES] = { 0 };
	int attr_index = 0;
	for (; attr_index < VGFX_MAX_VERTEX_ATTRIBUTES; attr_index++) {
		const vgfx_vertex_attr_prop* a_prop = &prop->layout.attrs[attr_index];
		if (a_prop->format == vgfx_vertex_format_invalid) {
			break;
		}
		if ((a_prop->buffer_index < 0) || (a_prop->buffer_index >= VGFX_MAX_SHADERSTAGE_BUFFERS)) {
			d3d11_error("vertex shader buffer index", d3d11_err_invalid);
		}
		const vgfx_buffer_layout_prop* l_prop = &prop->layout.buffers[a_prop->buffer_index];
		vgfx_vertex_step step_func = l_prop->step_func;
		int step_rate = l_prop->step_rate;
		D3D11_INPUT_ELEMENT_DESC* d3d11_comp = &d3d11_comps[attr_index];
		d3d11_comp->SemanticName = vp_strptr(&shd->d3d11.attrs[attr_index].sem_name);
		d3d11_comp->SemanticIndex = shd->d3d11.attrs[attr_index].sem_index;
		d3d11_comp->Format = d3d11_vertex_format(a_prop->format);
		d3d11_comp->InputSlot = a_prop->buffer_index;
		d3d11_comp->AlignedByteOffset = a_prop->offset;
		d3d11_comp->InputSlotClass = d3d11_input_classification(step_func);
		if (step_func == vgfx_vertex_step_per_instance) {
			d3d11_comp->InstanceDataStepRate = step_rate;
		}
		pip->cmn.vertex_layout_valid[a_prop->buffer_index] = true;
	}
	for (int li = 0; li < VGFX_MAX_SHADERSTAGE_BUFFERS; li++) {
		if (pip->cmn.vertex_layout_valid[li]) {
			const vgfx_buffer_layout_prop* l_prop = &prop->layout.buffers[li];
			if (l_prop->stride <= 0) {
				d3d11_error("buffer layout stride", d3d11_err_invalid);
			}
			pip->d3d11.vb_strides[li] = l_prop->stride;
		} else {
			pip->d3d11.vb_strides[li] = 0;
		}
	}

	HRESULT hr = _d3d11.dev->CreateInputLayout(d3d11_comps, attr_index, shd->d3d11.vs_blob, shd->d3d11.vs_blob_length, &pip->d3d11.il);
	if (FAILED(hr) || !pip->d3d11.il) {
		d3d11_error("d3d11 input layout", d3d11_err_create_failed);
	}

	D3D11_RASTERIZER_DESC vgfx_resource_stateprop = {};
	vgfx_resource_stateprop.FillMode = D3D11_FILL_SOLID;
	vgfx_resource_stateprop.CullMode = d3d11_vgfx_cull_mode(prop->rasterizer.cull_mode);
	vgfx_resource_stateprop.FrontCounterClockwise = (prop->rasterizer.face_winding == vgfx_face_winding_ccw) ? TRUE : FALSE;
	vgfx_resource_stateprop.DepthBias = (int)pip->cmn.depth_bias;
	vgfx_resource_stateprop.DepthBiasClamp = pip->cmn.depth_bias_clamp;
	vgfx_resource_stateprop.SlopeScaledDepthBias = pip->cmn.depth_bias_slope_scale;
	vgfx_resource_stateprop.DepthClipEnable = TRUE;
	vgfx_resource_stateprop.ScissorEnable = TRUE;
	vgfx_resource_stateprop.MultisampleEnable = (prop->rasterizer.sample_count > 1) ? TRUE : FALSE;
	vgfx_resource_stateprop.AntialiasedLineEnable = FALSE;
	hr = _d3d11.dev->CreateRasterizerState(&vgfx_resource_stateprop, &pip->d3d11.rs);
	if (FAILED(hr) || !pip->d3d11.rs) {
		d3d11_error("d3d11 rasterizer state", d3d11_err_create_failed);
	}

	D3D11_DEPTH_STENCIL_DESC dss_prop = {};
	dss_prop.DepthEnable = TRUE;
	dss_prop.DepthWriteMask = (prop->depth_stencil.depth_write_enabled) ? D3D11_DEPTH_WRITE_MASK_ALL : D3D11_DEPTH_WRITE_MASK_ZERO;
	dss_prop.DepthFunc = d3d11_compare_func(prop->depth_stencil.depth_compare_func);
	dss_prop.StencilEnable = prop->depth_stencil.stencil_enabled ? TRUE : FALSE;
	dss_prop.StencilReadMask = prop->depth_stencil.stencil_read_mask;
	dss_prop.StencilWriteMask = prop->depth_stencil.stencil_write_mask;

	const vgfx_stencil_state* sf = &prop->depth_stencil.stencil_front;
	dss_prop.FrontFace.StencilFailOp = d3d11_stencil_op(sf->fail_op);
	dss_prop.FrontFace.StencilDepthFailOp = d3d11_stencil_op(sf->depth_fail_op);
	dss_prop.FrontFace.StencilPassOp = d3d11_stencil_op(sf->pass_op);
	dss_prop.FrontFace.StencilFunc = d3d11_compare_func(sf->compare_func);

	const vgfx_stencil_state* sb = &prop->depth_stencil.stencil_back;
	dss_prop.BackFace.StencilFailOp = d3d11_stencil_op(sb->fail_op);
	dss_prop.BackFace.StencilDepthFailOp = d3d11_stencil_op(sb->depth_fail_op);
	dss_prop.BackFace.StencilPassOp = d3d11_stencil_op(sb->pass_op);
	dss_prop.BackFace.StencilFunc = d3d11_compare_func(sb->compare_func);
	hr = _d3d11.dev->CreateDepthStencilState(&dss_prop, &pip->d3d11.dss);
	if (FAILED(hr) || !pip->d3d11.dss) {
		d3d11_error("d3d11 depth stencil state", d3d11_err_create_failed);
	}

	D3D11_BLEND_DESC bs_prop = {};
	bs_prop.AlphaToCoverageEnable = (prop->rasterizer.alpha_to_coverage_enabled) ? TRUE : FALSE;
	bs_prop.IndependentBlendEnable = FALSE;
	bs_prop.RenderTarget[0].BlendEnable = prop->blend.enabled ? TRUE : FALSE;
	bs_prop.RenderTarget[0].SrcBlend = d3d11_blend_factor(prop->blend.src_factor_rgb);
	bs_prop.RenderTarget[0].DestBlend = d3d11_blend_factor(prop->blend.dst_factor_rgb);
	bs_prop.RenderTarget[0].BlendOp = d3d11_blend_op(prop->blend.op_rgb);
	bs_prop.RenderTarget[0].SrcBlendAlpha = d3d11_blend_factor(prop->blend.src_factor_alpha);
	bs_prop.RenderTarget[0].DestBlendAlpha = d3d11_blend_factor(prop->blend.dst_factor_alpha);
	bs_prop.RenderTarget[0].BlendOpAlpha = d3d11_blend_op(prop->blend.op_alpha);
	bs_prop.RenderTarget[0].RenderTargetWriteMask = d3d11_color_write_mask(prop->blend.color_write_mask);
	hr = _d3d11.dev->CreateBlendState(&bs_prop, &pip->d3d11.bs);
	if (FAILED(hr) || !pip->d3d11.bs) {
		d3d11_error("d3d11 blend state", d3d11_err_create_failed);
	}

	return vgfx_resource_state_valid;
}

void d3d11_destroy_pipeline(native_pipeline* pip)
{
	if (!pip) {
		d3d11_error(VGFX_D3D11_PIPELINE_LOG_OBJECT, d3d11_err_uninitialized);
	}
	if (pip->d3d11.il) {
		pip->d3d11.il->Release();
	}
	if (pip->d3d11.rs) {
		pip->d3d11.rs->Release();
	}
	if (pip->d3d11.dss) {
		pip->d3d11.dss->Release();
	}
	if (pip->d3d11.bs) {
		pip->d3d11.bs->Release();
	}
}

vgfx_resource_state d3d11_create_pass(native_pass* pass, native_image** att_images, const vgfx_pass_prop* prop)
{
	if (!pass) {
		d3d11_error(VGFX_D3D11_PASS_LOG_OBJECT, d3d11_err_unallocated);
	}
	if (!prop) {
		d3d11_error("prop", d3d11_err_unallocated);
	}
	if (!att_images || !att_images[0]) {
		d3d11_error(VGFX_D3D11_IMAGE_LOG_OBJECT, d3d11_err_unallocated);
	}
	if (!_d3d11.dev) {
		d3d11_error(VGFX_D3D11_BACKEND_LOG_OBJECT, d3d11_err_uninitialized);
	}

	pass->d3d11 = {};
	pass_common_init(&pass->cmn, prop);

	for (int i = 0; i < pass->cmn.num_color_atts; i++) {
		const vgfx_attachment_prop* att_prop = &prop->color_attachments[i];
		if (att_prop->image.id == INVALID_ID) {
			d3d11_error("image id", d3d11_err_invalid);
		}
		native_image* att_img = att_images[i];
		if (!att_img) {
			d3d11_error(VGFX_D3D11_IMAGE_LOG_OBJECT, d3d11_err_unallocated);
		}
		if (att_img->slot.id != att_prop->image.id) {
			d3d11_error("slot id", d3d11_err_invalid);
		}
		if (!vp_gfx_impl::instance.is_valid_rendertarget_color_format(att_img->cmn.pixel_format)) {
			d3d11_error("pixel format", d3d11_err_invalid);
		}
		if (pass->d3d11.color_atts[i].image) {
			d3d11_error(VGFX_D3D11_IMAGE_LOG_OBJECT, d3d11_err_initialized);
		}
		pass->d3d11.color_atts[i].image = att_img;

		const vgfx_attachment_common* cmn_att = &pass->cmn.color_atts[i];
		if (pass->d3d11.color_atts[i].rtv) {
			d3d11_error(VGFX_D3D11_PASS_LOG_OBJECT, d3d11_err_initialized);
		}
		ID3D11Resource* d3d11_res = NULL;
		const bool is_msaa = att_img->cmn.sample_count > 1;

		D3D11_RENDER_TARGET_VIEW_DESC d3d11_rtv_prop = {};
		d3d11_rtv_prop.Format = att_img->d3d11.format;
		if ((att_img->cmn.type == vgfx_image_type_2d) || is_msaa) {
			if (is_msaa) {
				d3d11_res = (ID3D11Resource*)att_img->d3d11.texmsaa;
				d3d11_rtv_prop.ViewDimension = D3D11_RTV_DIMENSION_TEXTURE2DMS;
			} else {
				d3d11_res = (ID3D11Resource*)att_img->d3d11.tex2d;
				d3d11_rtv_prop.ViewDimension = D3D11_RTV_DIMENSION_TEXTURE2D;
				d3d11_rtv_prop.Texture2D.MipSlice = cmn_att->mip_level;
			}
		} else if ((att_img->cmn.type == vgfx_image_type_cube) || (att_img->cmn.type == vgfx_image_type_array)) {
			d3d11_res = (ID3D11Resource*)att_img->d3d11.tex2d;
			d3d11_rtv_prop.ViewDimension = D3D11_RTV_DIMENSION_TEXTURE2DARRAY;
			d3d11_rtv_prop.Texture2DArray.MipSlice = cmn_att->mip_level;
			d3d11_rtv_prop.Texture2DArray.FirstArraySlice = cmn_att->slice;
			d3d11_rtv_prop.Texture2DArray.ArraySize = 1;
		} else {
			if (att_img->cmn.type != vgfx_image_type_3d) {
				d3d11_error("image type", d3d11_err_invalid);
			}
			d3d11_res = (ID3D11Resource*)att_img->d3d11.tex3d;
			d3d11_rtv_prop.ViewDimension = D3D11_RTV_DIMENSION_TEXTURE3D;
			d3d11_rtv_prop.Texture3D.MipSlice = cmn_att->mip_level;
			d3d11_rtv_prop.Texture3D.FirstWSlice = cmn_att->slice;
			d3d11_rtv_prop.Texture3D.WSize = 1;
		}
		if (!d3d11_res) {
			d3d11_error("d3d11 resource", d3d11_err_uninitialized);
		}
		HRESULT hr = _d3d11.dev->CreateRenderTargetView(d3d11_res, &d3d11_rtv_prop, &pass->d3d11.color_atts[i].rtv);
		if (FAILED(hr) || !pass->d3d11.color_atts[i].rtv) {
			d3d11_error("d3d11 render target view", d3d11_err_create_failed);
		}
	}
	if (pass->d3d11.ds_att.image || pass->d3d11.ds_att.dsv) {
		d3d11_error(VGFX_D3D11_PASS_LOG_OBJECT, d3d11_err_initialized);
	}
	if (prop->depth_stencil_attachment.image.id != INVALID_ID) {
		const int ds_img_index = VGFX_MAX_COLOR_ATTACHMENTS;
		const vgfx_attachment_prop* att_prop = &prop->depth_stencil_attachment;
		native_image* att_img = att_images[ds_img_index];
		if (!att_img) {
			d3d11_error(VGFX_D3D11_IMAGE_LOG_OBJECT, d3d11_err_uninitialized);
		}
		if (att_img->slot.id != att_prop->image.id) {
			d3d11_error("slot id", d3d11_err_invalid);
		}
		if (!vp_gfx_impl::instance.is_valid_rendertarget_depth_format(att_img->cmn.pixel_format)) {
			d3d11_error("pixel format", d3d11_err_invalid);
		}
		if (pass->d3d11.ds_att.image) {
			d3d11_error(VGFX_D3D11_PASS_LOG_OBJECT, d3d11_err_initialized);
		}
		pass->d3d11.ds_att.image = att_img;

		D3D11_DEPTH_STENCIL_VIEW_DESC d3d11_dsv_prop = {};
		d3d11_dsv_prop.Format = att_img->d3d11.format;
		const bool is_msaa = att_img->cmn.sample_count > 1;
		if (is_msaa) {
			d3d11_dsv_prop.ViewDimension = D3D11_DSV_DIMENSION_TEXTURE2DMS;
		} else {
			d3d11_dsv_prop.ViewDimension = D3D11_DSV_DIMENSION_TEXTURE2D;
		}
		ID3D11Resource* d3d11_res = (ID3D11Resource*)att_img->d3d11.texds;
		if (!d3d11_res) {
			d3d11_error("d3d11 resource", d3d11_err_uninitialized);
		}
		HRESULT hr = _d3d11.dev->CreateDepthStencilView(d3d11_res, &d3d11_dsv_prop, &pass->d3d11.ds_att.dsv);
		if (FAILED(hr) || !pass->d3d11.ds_att.dsv) {
			d3d11_error("d3d11 depth stencil view", d3d11_err_create_failed);
		}
	}

	return vgfx_resource_state_valid;
}

static void d3d11_destroy_pass(native_pass* pass)
{
	if (!pass) {
		d3d11_error(VGFX_D3D11_PASS_LOG_OBJECT, d3d11_err_uninitialized);
	}
	for (int i = 0; i < VGFX_MAX_COLOR_ATTACHMENTS; i++) {
		if (pass->d3d11.color_atts[i].rtv) {
			pass->d3d11.color_atts[i].rtv->Release();
		}
	}
	if (pass->d3d11.ds_att.dsv) {
		pass->d3d11.ds_att.dsv->Release();
	}
}

static const char* VGFX_D3D11_MSG_PASS_RUNNING = "Pass is already starting";
static const char* VGFX_D3D11_MSG_PASS_NOT_RUNNING = "Pass is not started";
static const char* VGFX_D3D11_MSG_PASS_MUST_BE_CALLED = "Pass must be started before do this action";

static void d3d11_begin_pass(native_pass* pass, const vgfx_pass_action* action, int w, int h)
{
	if (!action) {
		d3d11_error("pass action", d3d11_err_uninitialized);
	}
	if (_d3d11.in_pass) {
		d3d11_error_msg(VGFX_D3D11_MSG_PASS_RUNNING);
	}
	_d3d11.in_pass = true;
	_d3d11.cur_width = w;
	_d3d11.cur_height = h;
	if (pass) {
		_d3d11.cur_pass = pass;
		_d3d11.cur_pass_id.id = pass->slot.id;
		_d3d11.num_rtvs = 0;
		for (int i = 0; i < VGFX_MAX_COLOR_ATTACHMENTS; i++) {
			_d3d11.cur_rtvs[i] = pass->d3d11.color_atts[i].rtv;
			if (_d3d11.cur_rtvs[i]) {
				_d3d11.num_rtvs++;
			}
		}
		_d3d11.cur_dsv = pass->d3d11.ds_att.dsv;
	} else {
		_d3d11.cur_pass = NULL;
		_d3d11.cur_pass_id.id = INVALID_ID;
		_d3d11.num_rtvs = 1;
		_d3d11.cur_rtvs[0] = (ID3D11RenderTargetView*)_d3d11.rtv_cb();
		for (int i = 1; i < VGFX_MAX_COLOR_ATTACHMENTS; i++) {
			_d3d11.cur_rtvs[i] = NULL;
		}
		_d3d11.cur_dsv = (ID3D11DepthStencilView*)_d3d11.dsv_cb();
		if (!_d3d11.cur_rtvs[0] || !_d3d11.cur_dsv) {
			d3d11_error("d3d11 render target view", d3d11_err_uninitialized);
		}
		if (!_d3d11.cur_dsv) {
			d3d11_error("d3d11 depth stencil view", d3d11_err_uninitialized);
		}
	}

	_d3d11.ctx->OMSetRenderTargets(VGFX_MAX_COLOR_ATTACHMENTS, _d3d11.cur_rtvs, _d3d11.cur_dsv);

	D3D11_VIEWPORT vp = {};
	vp.Width = (float)w;
	vp.Height = (float)h;
	vp.MaxDepth = 1.0f;
	_d3d11.ctx->RSSetViewports(1, &vp);

	D3D11_RECT rect = {};
	rect.left = 0;
	rect.top = 0;
	rect.right = w;
	rect.bottom = h;
	_d3d11.ctx->RSSetScissorRects(1, &rect);

	for (int i = 0; i < _d3d11.num_rtvs; i++) {
		if (action->colors[i].action == vgfx_action_clear) {
			_d3d11.ctx->ClearRenderTargetView(_d3d11.cur_rtvs[i], action->colors[i].val);
		}
	}

	UINT ds_flags = 0;
	if (action->depth.action == vgfx_action_clear) {
		ds_flags |= D3D11_CLEAR_DEPTH;
	}
	if (action->stencil.action == vgfx_action_clear) {
		ds_flags |= D3D11_CLEAR_STENCIL;
	}
	if ((ds_flags != 0) && _d3d11.cur_dsv) {
		_d3d11.ctx->ClearDepthStencilView(_d3d11.cur_dsv, ds_flags, action->depth.val, action->stencil.val);
	}
}

static uint32_t d3d11_calcsubresource(uint32_t mip_slice, uint32_t array_slice, UINT mip_levels)
{
	return mip_slice + array_slice * mip_levels;
}

static void d3d11_end_pass()
{
	if (!_d3d11.ctx) {
		d3d11_error(VGFX_D3D11_BACKEND_LOG_OBJECT, d3d11_err_uninitialized);
	}
	if (!_d3d11.in_pass) {
		d3d11_error_msg(VGFX_D3D11_MSG_PASS_NOT_RUNNING);
	}
	_d3d11.in_pass = false;

	if (_d3d11.cur_pass) {
		if (_d3d11.cur_pass->slot.id != _d3d11.cur_pass_id.id) {
			d3d11_error("slot id", d3d11_err_invalid);
		}
		for (int i = 0; i < _d3d11.num_rtvs; i++) {
			const vgfx_attachment_common* cmn_att = &_d3d11.cur_pass->cmn.color_atts[i];
			native_image* att_img = _d3d11.cur_pass->d3d11.color_atts[i].image;
			if (!att_img) {
				d3d11_error(VGFX_D3D11_IMAGE_LOG_OBJECT, d3d11_err_uninitialized);
			}
			if (att_img->slot.id != cmn_att->image_id.id) {
				d3d11_error("slot id", d3d11_err_invalid);
			}
			if (att_img->cmn.sample_count > 1) {
				if (!att_img->d3d11.tex2d || !att_img->d3d11.texmsaa || !att_img->d3d11.tex3d) {
					d3d11_error("image format", d3d11_err_invalid);
				}
				if (att_img->d3d11.format == DXGI_FORMAT_UNKNOWN) {
					d3d11_error("pixel format", d3d11_err_invalid);
				}
				uint32_t dst_subres = d3d11_calcsubresource(cmn_att->mip_level, cmn_att->slice, att_img->cmn.num_mipmaps);
				_d3d11.ctx->ResolveSubresource((ID3D11Resource*)att_img->d3d11.tex2d, dst_subres, (ID3D11Resource*)att_img->d3d11.texmsaa, 0, att_img->d3d11.format);
			}
		}
	}

	_d3d11.cur_pass = NULL;
	_d3d11.cur_pass_id.id = INVALID_ID;
	_d3d11.cur_pipeline = NULL;
	_d3d11.cur_pipeline_id.id = INVALID_ID;
	for (int i = 0; i < VGFX_MAX_COLOR_ATTACHMENTS; i++) {
		_d3d11.cur_rtvs[i] = NULL;
	}
	_d3d11.cur_dsv = NULL;
	d3d11_clear_state();
}

static native_image* d3d11_pass_color_image(const native_pass* pass, int index)
{
	if (!pass) {
		d3d11_error(VGFX_D3D11_PASS_LOG_OBJECT, d3d11_err_uninitialized);
	}
	if ((index <= 0) || (index > VGFX_MAX_COLOR_ATTACHMENTS)) {
		d3d11_error("color attachment index", d3d11_err_invalid);
	}
	return pass->d3d11.color_atts[index].image;
}

static void d3d11_commit()
{
	if (_d3d11.in_pass) {
		d3d11_error_msg("ViperGFX pass was not ended");
	}
}

static void d3d11_update_buffer(native_buffer* buf, const void* data_ptr, int data_size)
{
	if (!buf) {
		d3d11_error(VGFX_D3D11_BUFFER_LOG_OBJECT, d3d11_err_uninitialized);
	}
	if (!data_ptr) {
		d3d11_error("data pointer", d3d11_err_uninitialized);
	}
	if (data_size <= 0) {
		d3d11_error("data size", d3d11_err_invalid);
	}
	if (!_d3d11.ctx) {
		d3d11_error(VGFX_D3D11_BACKEND_LOG_OBJECT, d3d11_err_uninitialized);
	}
	if (!buf->d3d11.buf) {
		d3d11_error(VGFX_D3D11_BUFFER_LOG_OBJECT, d3d11_err_uninitialized);
	}
	D3D11_MAPPED_SUBRESOURCE d3d11_msr = {};
	HRESULT hr = _d3d11.ctx->Map((ID3D11Resource*)buf->d3d11.buf, 0, D3D11_MAP_WRITE_DISCARD, 0, &d3d11_msr);
	if (FAILED(hr)) {
		d3d11_error_msg("Update buffer failed");
	}
	memcpy(d3d11_msr.pData, data_ptr, data_size);
	_d3d11.ctx->Unmap((ID3D11Resource*)buf->d3d11.buf, 0);
}

static void d3d11_append_buffer(native_buffer* buf, const void* data_ptr, int data_size, bool new_frame)
{
	if (!buf) {
		d3d11_error(VGFX_D3D11_BUFFER_LOG_OBJECT, d3d11_err_uninitialized);
	}
	if (!data_ptr) {
		d3d11_error("data pointer", d3d11_err_uninitialized);
	}
	if (data_size <= 0) {
		d3d11_error("data size", d3d11_err_invalid);
	}
	if (!_d3d11.ctx) {
		d3d11_error(VGFX_D3D11_BACKEND_LOG_OBJECT, d3d11_err_uninitialized);
	}
	if (!buf->d3d11.buf) {
		d3d11_error(VGFX_D3D11_BUFFER_LOG_OBJECT, d3d11_err_uninitialized);
	}
	D3D11_MAP map_type = new_frame ? D3D11_MAP_WRITE_DISCARD : D3D11_MAP_WRITE_NO_OVERWRITE;
	D3D11_MAPPED_SUBRESOURCE d3d11_msr = {};
	HRESULT hr = _d3d11.ctx->Map((ID3D11Resource*)buf->d3d11.buf, 0, map_type, 0, &d3d11_msr);
	if (FAILED(hr)) {
		d3d11_error_msg("Append buffer failed");
	}
	uint8_t* dst_ptr = (uint8_t*)d3d11_msr.pData + buf->cmn.append_pos;
	memcpy(dst_ptr, data_ptr, data_size);
	_d3d11.ctx->Unmap((ID3D11Resource*)buf->d3d11.buf, 0);
}

static void d3d11_update_image(native_image* img, const vgfx_image_content* data)
{
	if (!img) {
		d3d11_error(VGFX_D3D11_IMAGE_LOG_OBJECT, d3d11_err_uninitialized);
	}
	if (!data) {
		d3d11_error("image content", d3d11_err_uninitialized);
	}
	if (!_d3d11.ctx) {
		d3d11_error(VGFX_D3D11_BACKEND_LOG_OBJECT, d3d11_err_uninitialized);
	}
	if (!img->d3d11.tex2d && !img->d3d11.tex3d) {
		d3d11_error("image format", d3d11_err_invalid);
	}
	ID3D11Resource* d3d11_res = NULL;
	if (img->d3d11.tex3d) {
		d3d11_res = (ID3D11Resource*)img->d3d11.tex3d;
	} else {
		d3d11_res = (ID3D11Resource*)img->d3d11.tex2d;
	}
	if (!d3d11_res) {
		d3d11_error("d3d11 resource", d3d11_err_uninitialized);
	}
	const int num_faces = (img->cmn.type == vgfx_image_type_cube) ? 6 : 1;
	const int num_slices = (img->cmn.type == vgfx_image_type_array) ? img->cmn.depth : 1;
	int subres_index = 0;
	HRESULT hr;
	D3D11_MAPPED_SUBRESOURCE d3d11_msr = {};
	for (int face_index = 0; face_index < num_faces; face_index++) {
		for (int slice_index = 0; slice_index < num_slices; slice_index++) {
			for (int mip_index = 0; mip_index < img->cmn.num_mipmaps; mip_index++, subres_index++) {
				if (subres_index >= (VGFX_MAX_MIPMAPS * VGFX_MAX_TEXTUREARRAY_LAYERS)) {
					d3d11_error_msg("Subresource overflow");
				}
				const int mip_width = ((img->cmn.width >> mip_index) > 0) ? img->cmn.width >> mip_index : 1;
				const int mip_height = ((img->cmn.height >> mip_index) > 0) ? img->cmn.height >> mip_index : 1;
				const int src_pitch = row_pitch(img->cmn.pixel_format, mip_width);
				const auto subimg_content = &(data->subimage[face_index][mip_index]);
				const int slice_size = subimg_content->size / num_slices;
				const int slice_offset = slice_size * slice_index;
				const uint8_t* slice_ptr = ((const uint8_t*)subimg_content->ptr) + slice_offset;
				hr = _d3d11.ctx->Map(d3d11_res, subres_index, D3D11_MAP_WRITE_DISCARD, 0, &d3d11_msr);
				if (FAILED(hr)) {
					d3d11_error_msg("Update image failed");
				}
				if (src_pitch == (int)d3d11_msr.RowPitch) {
					memcpy(d3d11_msr.pData, slice_ptr, slice_size);
				} else {
					if (src_pitch >= (int)d3d11_msr.RowPitch) {
						d3d11_error_msg("Source pitch overflow");
					}
					const uint8_t* src_ptr = slice_ptr;
					uint8_t* dst_ptr = (uint8_t*)d3d11_msr.pData;
					for (int row_index = 0; row_index < mip_height; row_index++) {
						memcpy(dst_ptr, src_ptr, src_pitch);
						src_ptr += src_pitch;
						dst_ptr += d3d11_msr.RowPitch;
					}
				}
				_d3d11.ctx->Unmap(d3d11_res, subres_index);
			}
		}
	}
}

static void d3d11_apply_viewport(int x, int y, int w, int h, bool origin_top_left)
{
	if (!_d3d11.ctx) {
		d3d11_error(VGFX_D3D11_BACKEND_LOG_OBJECT, d3d11_err_uninitialized);
	}
	if (!_d3d11.in_pass) {
		d3d11_error_msg(VGFX_D3D11_MSG_PASS_MUST_BE_CALLED);
	}
	D3D11_VIEWPORT vp = {};
	vp.TopLeftX = (float)x;
	vp.TopLeftY = (float)(origin_top_left ? y : (_d3d11.cur_height - (y + h)));
	vp.Width = (float)w;
	vp.Height = (float)h;
	vp.MinDepth = 0.0f;
	vp.MaxDepth = 1.0f;
	_d3d11.ctx->RSSetViewports(1, &vp);
}

static void d3d11_apply_scissor_rect(int x, int y, int w, int h, bool origin_top_left)
{
	if (!_d3d11.ctx) {
		d3d11_error(VGFX_D3D11_BACKEND_LOG_OBJECT, d3d11_err_uninitialized);
	}
	if (!_d3d11.in_pass) {
		d3d11_error_msg(VGFX_D3D11_MSG_PASS_MUST_BE_CALLED);
	}
	D3D11_RECT rect = {};
	rect.left = x;
	rect.top = (origin_top_left ? y : (_d3d11.cur_height - (y + h)));
	rect.right = x + w;
	rect.bottom = origin_top_left ? (y + h) : (_d3d11.cur_height - y);
	_d3d11.ctx->RSSetScissorRects(1, &rect);
}

static void d3d11_apply_pipeline(native_pipeline* pip)
{
	if (!pip) {
		d3d11_error(VGFX_D3D11_PIPELINE_LOG_OBJECT, d3d11_err_uninitialized);
	}
	if (!pip->shader) {
		d3d11_error(VGFX_D3D11_SHADER_LOG_OBJECT, d3d11_err_uninitialized);
	}
	if (!_d3d11.ctx) {
		d3d11_error(VGFX_D3D11_BACKEND_LOG_OBJECT, d3d11_err_uninitialized);
	}
	if (!_d3d11.in_pass) {
		d3d11_error_msg(VGFX_D3D11_MSG_PASS_MUST_BE_CALLED);
	}
	if (!pip->d3d11.rs || !pip->d3d11.bs || !pip->d3d11.dss || !pip->d3d11.il) {
		d3d11_error(VGFX_D3D11_PIPELINE_LOG_OBJECT, d3d11_err_unallocated);
	}
	_d3d11.cur_pipeline = pip;
	_d3d11.cur_pipeline_id.id = pip->slot.id;
	_d3d11.use_indexed_draw = (pip->d3d11.index_format != DXGI_FORMAT_UNKNOWN);

	_d3d11.ctx->RSSetState(pip->d3d11.rs);
	_d3d11.ctx->OMSetDepthStencilState(pip->d3d11.dss, pip->d3d11.stencil_ref);
	_d3d11.ctx->OMSetBlendState(pip->d3d11.bs, pip->cmn.blend_color, 0xFFFFFFFF);
	_d3d11.ctx->IASetPrimitiveTopology(pip->d3d11.topology);
	_d3d11.ctx->IASetInputLayout(pip->d3d11.il);
	_d3d11.ctx->VSSetShader(pip->shader->d3d11.vs, NULL, 0);
	_d3d11.ctx->VSSetConstantBuffers(0, VGFX_MAX_SHADERSTAGE_UBS, pip->shader->d3d11.stage[vgfx_shader_stage_vs].cbufs);
	_d3d11.ctx->PSSetShader(pip->shader->d3d11.fs, NULL, 0);
	_d3d11.ctx->PSSetConstantBuffers(0, VGFX_MAX_SHADERSTAGE_UBS, pip->shader->d3d11.stage[vgfx_shader_stage_fs].cbufs);
}

static void d3d11_apply_bindings(native_pipeline* pip, native_buffer** vbs, const int* vb_offsets, int num_vbs, native_buffer* ib, int ib_offset, native_image** vs_imgs, int num_vs_imgs, native_image** fs_imgs, int num_fs_imgs)
{
	if (!pip) {
		d3d11_error(VGFX_D3D11_PIPELINE_LOG_OBJECT, d3d11_err_uninitialized);
	}
	if (!_d3d11.ctx) {
		d3d11_error(VGFX_D3D11_BACKEND_LOG_OBJECT, d3d11_err_uninitialized);
	}
	if (!_d3d11.in_pass) {
		d3d11_error_msg(VGFX_D3D11_MSG_PASS_MUST_BE_CALLED);
	}
	ID3D11Buffer* d3d11_ib = ib ? ib->d3d11.buf : NULL;
	ID3D11Buffer* d3d11_vbs[VGFX_MAX_SHADERSTAGE_BUFFERS] = { 0 };

	UINT d3d11_vb_offsets[VGFX_MAX_SHADERSTAGE_BUFFERS] = { 0 };
	ID3D11ShaderResourceView* d3d11_vs_srvs[VGFX_MAX_SHADERSTAGE_IMAGES] = { 0 };
	ID3D11SamplerState* d3d11_vs_smps[VGFX_MAX_SHADERSTAGE_IMAGES] = { 0 };
	ID3D11ShaderResourceView* d3d11_fs_srvs[VGFX_MAX_SHADERSTAGE_IMAGES] = { 0 };
	ID3D11SamplerState* d3d11_fs_smps[VGFX_MAX_SHADERSTAGE_IMAGES] = { 0 };

	int i;
	for (i = 0; i < num_vbs; i++) {
		if (!vbs[i]->d3d11.buf) {
			d3d11_error("d3d11 vertex buffer", d3d11_err_uninitialized);
		}
		d3d11_vbs[i] = vbs[i]->d3d11.buf;
		d3d11_vb_offsets[i] = vb_offsets[i];
	}
	for (; i < VGFX_MAX_SHADERSTAGE_BUFFERS; i++) {
		d3d11_vbs[i] = NULL;
		d3d11_vb_offsets[i] = 0;
	}
	for (i = 0; i < num_vs_imgs; i++) {
		if (!vs_imgs[i]->d3d11.srv) {
			d3d11_error("d3d11 shader resource view", d3d11_err_uninitialized);
		}
		if (!vs_imgs[i]->d3d11.smp) {
			d3d11_error("d3d11 sampler state", d3d11_err_uninitialized);
		}
		d3d11_vs_srvs[i] = vs_imgs[i]->d3d11.srv;
		d3d11_vs_smps[i] = vs_imgs[i]->d3d11.smp;
	}
	for (; i < VGFX_MAX_SHADERSTAGE_IMAGES; i++) {
		d3d11_vs_srvs[i] = NULL;
		d3d11_vs_smps[i] = NULL;
	}
	for (i = 0; i < num_fs_imgs; i++) {
		if (!fs_imgs[i]->d3d11.srv) {
			d3d11_error("d3d11 shader resource view", d3d11_err_uninitialized);
		}
		if (!fs_imgs[i]->d3d11.smp) {
			d3d11_error("d3d11 sampler state", d3d11_err_uninitialized);
		}
		d3d11_fs_srvs[i] = fs_imgs[i]->d3d11.srv;
		d3d11_fs_smps[i] = fs_imgs[i]->d3d11.smp;
	}
	for (; i < VGFX_MAX_SHADERSTAGE_IMAGES; i++) {
		d3d11_fs_srvs[i] = NULL;
		d3d11_fs_smps[i] = NULL;
	}

	_d3d11.ctx->IASetVertexBuffers(0, VGFX_MAX_SHADERSTAGE_BUFFERS, d3d11_vbs, pip->d3d11.vb_strides, d3d11_vb_offsets);
	_d3d11.ctx->IASetIndexBuffer(d3d11_ib, pip->d3d11.index_format, ib_offset);
	_d3d11.ctx->VSSetShaderResources(0, VGFX_MAX_SHADERSTAGE_IMAGES, d3d11_vs_srvs);
	_d3d11.ctx->VSSetSamplers(0, VGFX_MAX_SHADERSTAGE_IMAGES, d3d11_vs_smps);
	_d3d11.ctx->PSSetShaderResources(0, VGFX_MAX_SHADERSTAGE_IMAGES, d3d11_fs_srvs);
	_d3d11.ctx->PSSetSamplers(0, VGFX_MAX_SHADERSTAGE_IMAGES, d3d11_fs_smps);
}

static void d3d11_apply_uniforms(vgfx_shader_stages stage_index, int ub_index, const void* data, int num_bytes)
{
	if (!_d3d11.in_pass) {
		d3d11_error_msg(VGFX_D3D11_MSG_PASS_MUST_BE_CALLED);
	}
	if (!_d3d11.ctx) {
		d3d11_error(VGFX_D3D11_BACKEND_LOG_OBJECT, d3d11_err_uninitialized);
	}
	if (!data) {
		d3d11_error("uniform data", d3d11_err_uninitialized);
	}
	if (num_bytes <= 0) {
		d3d11_error("uniform data size", d3d11_err_invalid);
	}
	if ((stage_index < 0) || ((int)stage_index >= VGFX_NUM_SHADER_STAGES)) {
		d3d11_error("shader stage", d3d11_err_invalid);
	}
	if ((ub_index < 0) || (ub_index >= VGFX_MAX_SHADERSTAGE_UBS)) {
		d3d11_error("uniform buffer index", d3d11_err_invalid);
	}
	if (!_d3d11.cur_pipeline) {
		d3d11_error(VGFX_D3D11_PIPELINE_LOG_OBJECT, d3d11_err_uninitialized);
	}
	if (_d3d11.cur_pipeline->slot.id != _d3d11.cur_pipeline_id.id) {
		d3d11_error("pipeline slot id", d3d11_err_invalid);
	}
	if (!_d3d11.cur_pipeline->shader) {
		d3d11_error(VGFX_D3D11_SHADER_LOG_OBJECT, d3d11_err_uninitialized);
	}
	if (_d3d11.cur_pipeline->shader->slot.id != _d3d11.cur_pipeline->cmn.shader_id.id) {
		d3d11_error("pipeline slot id", d3d11_err_invalid);
	}
	if (ub_index >= _d3d11.cur_pipeline->shader->cmn.stage[stage_index].num_uniform_blocks) {
		d3d11_error("uniform buffer index", d3d11_err_invalid);
	}
	if (num_bytes != _d3d11.cur_pipeline->shader->cmn.stage[stage_index].uniform_blocks[ub_index].size) {
		d3d11_error("uniform data size", d3d11_err_invalid);
	}
	ID3D11Buffer* cb = _d3d11.cur_pipeline->shader->d3d11.stage[stage_index].cbufs[ub_index];
	if (!cb) {
		d3d11_error("d3d11 buffer", d3d11_err_uninitialized);
	}
	_d3d11.ctx->UpdateSubresource((ID3D11Resource*)cb, 0, NULL, data, 0, 0);
}

static void d3d11_draw(int base_element, int num_elements, int num_instances)
{
	if (!_d3d11.in_pass) {
		d3d11_error_msg(VGFX_D3D11_MSG_PASS_MUST_BE_CALLED);
	}
	if (_d3d11.use_indexed_draw) {
		if (num_instances == 1) {
			_d3d11.ctx->DrawIndexed(num_elements, base_element, 0);
		} else {
			_d3d11.ctx->DrawIndexedInstanced(num_elements, num_instances, base_element, 0, 0);
		}
	} else {
		if (num_instances == 1) {
			_d3d11.ctx->Draw(num_elements, base_element);
		} else {
			_d3d11.ctx->DrawInstanced(num_elements, num_instances, base_element, 0);
		}
	}
}
