#include "stdafx.h"
#pragma hdrstop

#pragma warning(push)
#pragma warning(disable:4995)
#include <d3dx9.h>
#pragma warning(pop)

#include "ResourceManager.h"
#include "blenders\Blender_Recorder.h"
#include "blenders\Blender.h"

#include "../../xrEngine/igame_persistent.h"
#include "../../xrEngine/environment.h"

#include "dxRenderDeviceRender.h"

// matrices
#define	BIND_DECLARE(xf)	\
class cl_xform_##xf	: public R_constant_setup {	virtual void setup (R_constant* C) { RCache.xforms.set_c_##xf (C); } }; \
	static cl_xform_##xf	binder_##xf
BIND_DECLARE(w);
BIND_DECLARE(invw);
BIND_DECLARE(v);
BIND_DECLARE(p);
BIND_DECLARE(wv);
BIND_DECLARE(vp);
BIND_DECLARE(wvp);

#define DECLARE_TREE_BIND(c)	\
	class cl_tree_##c: public R_constant_setup	{virtual void setup(R_constant* C) {RCache.tree.set_c_##c(C);} };	\
	static cl_tree_##c	tree_binder_##c

DECLARE_TREE_BIND(m_xform_v);
DECLARE_TREE_BIND(m_xform);
DECLARE_TREE_BIND(consts);
DECLARE_TREE_BIND(wave);
DECLARE_TREE_BIND(wind);
DECLARE_TREE_BIND(c_scale);
DECLARE_TREE_BIND(c_bias);
DECLARE_TREE_BIND(c_sun);

static class cl_hemi_cube_pos_faces : public R_constant_setup
{
	virtual void setup(R_constant* C) {RCache.hemi.set_c_pos_faces(C);}
} binder_hemi_cube_pos_faces;

static class cl_hemi_cube_neg_faces : public R_constant_setup
{
	virtual void setup(R_constant* C) {RCache.hemi.set_c_neg_faces(C);}
} binder_hemi_cube_neg_faces;

static class cl_material : public R_constant_setup
{
	virtual void setup(R_constant* C) {RCache.hemi.set_c_material(C);}
} binder_material;

static class cl_texgen : public R_constant_setup
{
	virtual void setup(R_constant* C)
	{
		Fmatrix mTexgen;

#ifdef	USE_DX10
		Fmatrix			mTexelAdjust		= 
		{
			0.5f,				0.0f,				0.0f,			0.0f,
			0.0f,				-0.5f,				0.0f,			0.0f,
			0.0f,				0.0f,				1.0f,			0.0f,
			0.5f,				0.5f,				0.0f,			1.0f
		};
#else	//	USE_DX10
		float	_w						= float(Device.dwWidth);
		float	_h						= float(Device.dwHeight);
		float	o_w						= (.5f / _w);
		float	o_h						= (.5f / _h);
		Fmatrix			mTexelAdjust		= 
		{
			0.5f,				0.0f,				0.0f,			0.0f,
			0.0f,				-0.5f,				0.0f,			0.0f,
			0.0f,				0.0f,				1.0f,			0.0f,
			0.5f + o_w,			0.5f + o_h,			0.0f,			1.0f
		};
#endif	//	USE_DX10

		mTexgen.mul	(mTexelAdjust,RCache.xforms.m_wvp);

		RCache.set_c(C, mTexgen);
	}
} binder_texgen;

static class cl_VPtexgen : public R_constant_setup
{
	virtual void setup(R_constant* C)
	{
		Fmatrix mTexgen;

#ifdef	USE_DX10
		Fmatrix			mTexelAdjust		= 
		{
			0.5f,				0.0f,				0.0f,			0.0f,
			0.0f,				-0.5f,				0.0f,			0.0f,
			0.0f,				0.0f,				1.0f,			0.0f,
			0.5f,				0.5f,				0.0f,			1.0f
		};
#else	//	USE_DX10
		float	_w						= float(Device.dwWidth);
		float	_h						= float(Device.dwHeight);
		float	o_w						= (.5f / _w);
		float	o_h						= (.5f / _h);
		Fmatrix			mTexelAdjust		= 
		{
			0.5f,				0.0f,				0.0f,			0.0f,
			0.0f,				-0.5f,				0.0f,			0.0f,
			0.0f,				0.0f,				1.0f,			0.0f,
			0.5f + o_w,			0.5f + o_h,			0.0f,			1.0f
		};
#endif	//	USE_DX10

		mTexgen.mul	(mTexelAdjust,RCache.xforms.m_vp);

		RCache.set_c(C, mTexgen);
	}
} binder_VPtexgen;

// fog
#ifndef _EDITOR
static class cl_fog_plane : public R_constant_setup{
	u32			marker;
	Fvector4	result;
	virtual void setup(R_constant* C)
	{
		if (marker!=Device.dwFrame)
		{
			// Plane
			Fvector4		plane;
			Fmatrix&	M	= Device.mFullTransform;
			plane.x			= -(M._14 + M._13);
			plane.y			= -(M._24 + M._23);
			plane.z			= -(M._34 + M._33);
			plane.w			= -(M._44 + M._43);
			float denom		= -1.0f / _sqrt(_sqr(plane.x)+_sqr(plane.y)+_sqr(plane.z));
			plane.mul		(denom);

			// Near/Far
			float A			= g_pGamePersistent->Environment().CurrentEnv->fog_near;
			float B			= 1/(g_pGamePersistent->Environment().CurrentEnv->fog_far - A);
			result.set		(-plane.x*B, -plane.y*B, -plane.z*B, 1 - (plane.w-A)*B	);								// view-plane
		}
		RCache.set_c	(C,result);
	}
} binder_fog_plane;

// fog-params
static class cl_fog_params : public R_constant_setup{
	u32			marker;
	Fvector4	result;
	virtual void setup(R_constant* C)
	{
		if (marker!=Device.dwFrame)
		{
			// Near/Far
			float	n		= g_pGamePersistent->Environment().CurrentEnv->fog_near;
			float	f		= g_pGamePersistent->Environment().CurrentEnv->fog_far;
			float	r		= 1/(f-n);
			result.set		(-n*r, r, r, r);
		}
		RCache.set_c	(C,result);
	}
} binder_fog_params;

// fog-color
static class cl_fog_color : public R_constant_setup{
	u32			marker;
	Fvector4	result;
	virtual void setup	(R_constant* C)	{
		if (marker!=Device.dwFrame)	{
			CEnvDescriptor&	desc	= *g_pGamePersistent->Environment().CurrentEnv;
			result.set				(desc.fog_color.x,	desc.fog_color.y, desc.fog_color.z,	0);
		}
		RCache.set_c	(C,result);
	}
} binder_fog_color;
#endif

// times
static class cl_times : public R_constant_setup{
	virtual void setup(R_constant* C)
	{
		float 		t	= Device.fTimeGlobal;
		RCache.set_c	(C,t,t*10,t/10,_sin(t))	;
	}
} binder_times;

// eye-params
static class cl_eye_P : public R_constant_setup{
	virtual void setup(R_constant* C)
	{
		Fvector&		V	= Device.vCameraPosition;
		RCache.set_c	(C,V.x,V.y,V.z,1);
	}
} binder_eye_P;

// eye-params
static class cl_eye_D : public R_constant_setup{
	virtual void setup(R_constant* C)
	{
		Fvector&		V	= Device.vCameraDirection;
		RCache.set_c	(C,V.x,V.y,V.z,0);
	}
} binder_eye_D;

// eye-params
static class cl_eye_N : public R_constant_setup{
	virtual void setup(R_constant* C)
	{
		Fvector&		V	= Device.vCameraTop;
		RCache.set_c	(C,V.x,V.y,V.z,0);
	}
} binder_eye_N;

#ifndef _EDITOR
// D-Light0
static class cl_sun0_color : public R_constant_setup{
	u32			marker;
	Fvector4	result;
	virtual void setup	(R_constant* C)	{
		if (marker!=Device.dwFrame)	{
			CEnvDescriptor&	desc	= *g_pGamePersistent->Environment().CurrentEnv;
			result.set				(desc.sun_color.x,	desc.sun_color.y, desc.sun_color.z,	0);
		}
		RCache.set_c	(C,result);
	}
} binder_sun0_color;

static class cl_sun0_dir_w : public R_constant_setup{
	u32			marker;
	Fvector4	result;
	virtual void setup	(R_constant* C)	{
		if (marker!=Device.dwFrame)	{
			CEnvDescriptor&	desc	= *g_pGamePersistent->Environment().CurrentEnv;
			result.set				(desc.sun_dir.x,	desc.sun_dir.y, desc.sun_dir.z,	0);
		}
		RCache.set_c	(C,result);
	}
} binder_sun0_dir_w;

static class cl_sun0_dir_e : public R_constant_setup{
	u32			marker;
	Fvector4	result;
	virtual void setup	(R_constant* C)	{
		if (marker!=Device.dwFrame)	{
			Fvector D;
			CEnvDescriptor&	desc		= *g_pGamePersistent->Environment().CurrentEnv;
			Device.mView.transform_dir	(D,desc.sun_dir);
			D.normalize					();
			result.set					(D.x,D.y,D.z,0);
		}
		RCache.set_c	(C,result);
	}
} binder_sun0_dir_e;

//
static class cl_amb_color : public R_constant_setup{
	u32			marker;
	Fvector4	result;
	virtual void setup	(R_constant* C)	{
		if (marker!=Device.dwFrame)	{
			CEnvDescriptorMixer&	desc	= *g_pGamePersistent->Environment().CurrentEnv;
			result.set				(desc.ambient.x, desc.ambient.y, desc.ambient.z, desc.weight);
		}
		RCache.set_c	(C,result);
	}
} binder_amb_color;

static class cl_hemi_color : public R_constant_setup{
	u32			marker;
	Fvector4	result;
	virtual void setup	(R_constant* C)	{
		if (marker!=Device.dwFrame)	{
			CEnvDescriptor&	desc	= *g_pGamePersistent->Environment().CurrentEnv;
			result.set				(desc.hemi_color.x, desc.hemi_color.y, desc.hemi_color.z, desc.hemi_color.w);
		}
		RCache.set_c	(C,result);
	}
} binder_hemi_color;
#endif

#if RENDER != R_R1
extern float r_dtex_range;
static class cl_parallax : public R_constant_setup		
{	
	virtual void setup	(R_constant* C)
	{
		float			h =	ps_r2_df_parallax_h;
		RCache.set_c	(C,h,-h/2.f,1.f/r_dtex_range,1.f/r_dtex_range);
	}
}binder_parallax;

static class cl_pos_decompress_params		: public R_constant_setup		
{	
	virtual void setup	(R_constant* C)
	{
		float VertTan =  -1.0f * tanf(deg2rad(Device.fFOV/2.0f));
		float HorzTan =  - VertTan / Device.fASPECT;

		RCache.set_c	(C, HorzTan, VertTan, (2.0f * HorzTan )/(float)Device.dwWidth, (2.0f * VertTan) /(float)Device.dwHeight);
	}
}	binder_pos_decompress_params;

static class cl_water_intensity : public R_constant_setup		
{	
	virtual void setup	(R_constant* C)
	{
		CEnvDescriptor&	E = *g_pGamePersistent->Environment().CurrentEnv;
		float fValue = E.m_fWaterIntensity;
		RCache.set_c	(C, fValue, fValue, fValue, 0);
	}
}	binder_water_intensity;

static class cl_sun_shafts_intensity : public R_constant_setup		
{	
	virtual void setup	(R_constant* C)
	{
		CEnvDescriptor&	E = *g_pGamePersistent->Environment().CurrentEnv;
		float fValue = E.m_fSunShaftsIntensity;
		RCache.set_c	(C, fValue, fValue, fValue, 0);
	}
}	binder_sun_shafts_intensity;
#endif

static class cl_screen_res : public R_constant_setup		
{	
	u32			marker;
	Fvector4	result;
	virtual void setup(R_constant* C)	{
		if (marker != Device.dwFrame)	{
			float _w = float(Device.dwWidth);
			float _h = float(Device.dwHeight);
			result.set(_w, _h, (float)1.0 / _w, (float)1.0 / _h);
		}
		RCache.set_c(C, result);
	}
} binder_screen_res;


static class cl_refl_params : public R_constant_setup
{
	virtual void setup(R_constant* C)	{
		
		RCache.set_c(C, ps_r2_refl_intensity, ps_r2_refl_env_intensity, ps_r2_refl_water_gloss, 0);
	}
} binder_refl_params;

static class cl_refl_params2 : public R_constant_setup
{
	virtual void setup(R_constant* C)	{
		
		RCache.set_c(C, ps_r2_refl_fresnel1, ps_r2_refl_fresnel2, ps_r2_refl_fresnel3, 0);
	}
} binder_refl_params2;

static class cl_refl_various : public R_constant_setup
{
	virtual void setup(R_constant* C)	{
		BOOL sun = RImplementation.is_sun();
		RCache.set_c(C, g_pGamePersistent->Environment().CurrentEnv->moon_road_intensity, sun, ps_r2_sun_near, /*actor_torch_enabled*/ 0);
	}
} binder_refl_various;

// Standart constant-binding
void	CBlender_Compile::SetMapping	()
{
	// matrices
	r_Constant				("m_W",				&binder_w);
	r_Constant				("m_invW",			&binder_invw);
	r_Constant				("m_V",				&binder_v);
	r_Constant				("m_P",				&binder_p);
	r_Constant				("m_WV",			&binder_wv);
	r_Constant				("m_VP",			&binder_vp);
	r_Constant				("m_WVP",			&binder_wvp);

	r_Constant				("m_xform_v",		&tree_binder_m_xform_v);
	r_Constant				("m_xform",			&tree_binder_m_xform);
	r_Constant				("consts",			&tree_binder_consts);
	r_Constant				("wave",			&tree_binder_wave);
	r_Constant				("wind",			&tree_binder_wind);
	r_Constant				("c_scale",			&tree_binder_c_scale);
	r_Constant				("c_bias",			&tree_binder_c_bias);
	r_Constant				("c_sun",			&tree_binder_c_sun);

	//hemi cube
	r_Constant				("L_material",			&binder_material);
	r_Constant				("hemi_cube_pos_faces",			&binder_hemi_cube_pos_faces);
	r_Constant				("hemi_cube_neg_faces",			&binder_hemi_cube_neg_faces);

	//	Igor	temp solution for the texgen functionality in the shader
	r_Constant				("m_texgen",			&binder_texgen);
	r_Constant				("mVPTexgen",			&binder_VPtexgen);

#ifndef _EDITOR
	// fog-params
	r_Constant				("fog_plane",		&binder_fog_plane);
	r_Constant				("fog_params",		&binder_fog_params);
	r_Constant				("fog_color",		&binder_fog_color);
#endif
	// time
	r_Constant				("timers",			&binder_times);

	// eye-params
	r_Constant				("eye_position",	&binder_eye_P);
	r_Constant				("eye_direction",	&binder_eye_D);
	r_Constant				("eye_normal",		&binder_eye_N);

#ifndef _EDITOR
	// global-lighting (env params)
	r_Constant				("L_sun_color",		&binder_sun0_color);
	r_Constant				("L_sun_dir_w",		&binder_sun0_dir_w);
	r_Constant				("L_sun_dir_e",		&binder_sun0_dir_e);
//	r_Constant				("L_lmap_color",	&binder_lm_color);
	r_Constant				("L_hemi_color",	&binder_hemi_color);
	r_Constant				("L_ambient",		&binder_amb_color);
#endif

#if RENDER != R_R1
	r_Constant				("parallax",		&binder_parallax);
	r_Constant				("water_intensity",	&binder_water_intensity);
	r_Constant				("sun_shafts_intensity",		&binder_sun_shafts_intensity);
	r_Constant				("pos_decompression_params",	&binder_pos_decompress_params);
#endif

	r_Constant				("screen_res",		&binder_screen_res);

	r_Constant				("refl_params",		&binder_refl_params);
	r_Constant				("refl_params2",	&binder_refl_params2);
	r_Constant				("refl_various",	&binder_refl_various);

	// detail
	//if (bDetail	&& detail_scaler)
	//	Igor: bDetail can be overridden by no_detail_texture option.
	//	But shader can be deatiled implicitly, so try to set this parameter
	//	anyway.
	if (detail_scaler)
		r_Constant			("dt_params",		detail_scaler);

	// other common
	for (u32 it=0; it<DEV->v_constant_setup.size(); it++)
	{
		std::pair<shared_str,R_constant_setup*>	cs	= DEV->v_constant_setup[it];
		r_Constant			(*cs.first,cs.second);
	}
}
