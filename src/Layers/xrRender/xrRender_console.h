#pragma once

// Common
extern ECORE_API	u32			ps_r_sun_shafts;	//=	0;
extern ECORE_API	xr_token	qsun_shafts_token[];

extern ECORE_API	u32			ps_r_ssao;			//	=	0;
extern ECORE_API	xr_token	qssao_token[];

extern ECORE_API	u32			ps_r_sun_quality;	//	=	0;
extern ECORE_API	xr_token	qsun_quality_token[];

extern ENGINE_API	int			ps_r__Supersample;
extern ECORE_API	int			ps_r__LightSleepFrames;

extern ECORE_API	float		ps_r__Tree_w_rot;
extern ECORE_API	float		ps_r__Tree_w_speed;
extern ECORE_API	float		ps_r__Tree_w_amp;
extern ECORE_API	float		ps_r__Tree_SBC;		// scale bias correct
extern ECORE_API	Fvector		ps_r__Tree_Wave;

extern ECORE_API	float		ps_r__WallmarkTTL;
extern ECORE_API	float		ps_r__WallmarkSHIFT;
extern ECORE_API	float		ps_r__WallmarkSHIFT_V;

extern ECORE_API	float		ps_r__GLOD_ssa_start;
extern ECORE_API	float		ps_r__GLOD_ssa_end;
extern ECORE_API	float		ps_r__LOD;
//.extern ECORE_API	float		ps_r__LOD_Power;
extern ECORE_API	float		ps_r__ssaDISCARD;
extern ECORE_API	float		ps_r__ssaDONTSORT;
extern ECORE_API	float		ps_r__ssaHZBvsTEX;
extern ECORE_API	float		ps_r__ssaLOD_A;
extern ECORE_API	float		ps_r__ssaLOD_B;
extern ECORE_API	float		ps_r__dtex_range;
extern ECORE_API	int			ps_r__tf_Anisotropic;
extern ECORE_API	float		ps_r__tf_Mipbias;
// DETAILS
extern ECORE_API	float		ps_r__Detail_l_ambient;
extern ECORE_API	float		ps_r__Detail_l_aniso;
extern ECORE_API	float		ps_r__Detail_density;
extern ECORE_API	Fvector		ps_r__details_opt;
//

// R1 //////////////////////////////////////////////////////////////////////////
extern ECORE_API	float		ps_r1_lmodel_lerp;
extern ECORE_API	float		ps_r1_dlights_clip;
extern ECORE_API	float		ps_r1_pps_u;
extern ECORE_API	float		ps_r1_pps_v;
extern ECORE_API	int			ps_r1_GlowsPerFrame;	// r1-only
extern ECORE_API	Flags32		ps_r1_flags;			// r1-only

extern ECORE_API	float		ps_r1_fog_luminance;	//1.f r1-only
extern ECORE_API	int			ps_r1_SoftwareSkinning;	// r1-only

enum
{
	R1FLAG_DLIGHTS				= (1<<0),
	R1FLAG_DETAIL_TEXTURES		= (2<<0),
};

// R2 //////////////////////////////////////////////////////////////////////////

extern ECORE_API	u32			ps_r2_reflections;
extern ECORE_API	xr_token	r2_reflection_token[];

extern ECORE_API	u32			ps_r2_smap;
extern ECORE_API	xr_token	r2_shadow_map_token[];

extern ECORE_API	u32			ps_r_sun_shafts;	//=	0;
extern ECORE_API	xr_token	qsun_shafts_token[];

extern ECORE_API	u32			ps_r_ssao;			//	=	0;
extern ECORE_API	xr_token	qssao_token[];

extern ECORE_API	u32			ps_r_sun_quality;	//	=	0;
extern ECORE_API	xr_token	qsun_quality_token[];

extern ECORE_API Flags32		ps_r2_ls_flags;				// r2-only
extern ECORE_API Flags32		ps_r2_ls_flags_ext;
extern ECORE_API float			ps_r2_df_parallax_h;		// r2-only
extern ECORE_API float			ps_r2_df_parallax_range;	// r2-only
extern ECORE_API float			ps_r2_gmaterial;			// r2-only
extern ECORE_API float			ps_r2_tonemap_middlegray;	// r2-only
extern ECORE_API float			ps_r2_tonemap_adaptation;	// r2-only
extern ECORE_API float			ps_r2_tonemap_low_lum;		// r2-only
extern ECORE_API float			ps_r2_tonemap_amount;		// r2-only
extern ECORE_API float			ps_r2_ls_bloom_kernel_scale;// r2-only	// gauss
extern ECORE_API float			ps_r2_ls_bloom_kernel_g;	// r2-only	// gauss
extern ECORE_API float			ps_r2_ls_bloom_kernel_b;	// r2-only	// bilinear
extern ECORE_API float			ps_r2_ls_bloom_threshold;	// r2-only
extern ECORE_API float			ps_r2_ls_bloom_speed;		// r2-only
extern ECORE_API float			ps_r2_ls_dsm_kernel;		// r2-only
extern ECORE_API float			ps_r2_ls_psm_kernel;		// r2-only
extern ECORE_API float			ps_r2_ls_ssm_kernel;		// r2-only
extern ECORE_API Fvector		ps_r2_aa_barier;			// r2-only
extern ECORE_API Fvector		ps_r2_aa_weight;			// r2-only
extern ECORE_API float			ps_r2_aa_kernel;			// r2-only
extern ECORE_API float			ps_r2_mblur;				// .5f
extern ECORE_API int			ps_r2_GI_depth;				// 1..5
extern ECORE_API int			ps_r2_GI_photons;			// 8..256
extern ECORE_API float			ps_r2_GI_clip;				// EPS
extern ECORE_API float			ps_r2_GI_refl;				// .9f
extern ECORE_API float			ps_r2_ls_depth_scale;		// 1.0f
extern ECORE_API float			ps_r2_ls_depth_bias;		// -0.0001f
extern ECORE_API float			ps_r2_ls_squality;			// 1.0f
extern ECORE_API float			ps_r2_sun_near;				// 10.0f
extern ECORE_API float			ps_r2_sun_near_border;		// 1.0f
extern ECORE_API float			ps_r2_sun_far;				// 100.0f
extern ECORE_API float			ps_r2_sun_tsm_bias;			// 0.0001f
extern ECORE_API float			ps_r2_sun_depth_far_scale;	// 1.00001f
extern ECORE_API float			ps_r2_sun_depth_near_scale;	// 1.00001f
extern ECORE_API float			ps_r2_sun_lumscale;			// 0.5f
extern ECORE_API float			ps_r2_sun_lumscale_hemi;	// 1.0f
extern ECORE_API float			ps_r2_sun_lumscale_amb;		// 1.0f
extern ECORE_API float			ps_r2_zfill;				// .1f

extern ECORE_API float			ps_r2_dhemi_sky_scale;		// 1.5f
extern ECORE_API float			ps_r2_dhemi_light_scale;	// 1.f
extern ECORE_API float			ps_r2_dhemi_light_flow;		// .1f
extern ECORE_API int			ps_r2_dhemi_count;			// 5
extern ECORE_API float			ps_r2_slight_fade;			// 1.f
extern ECORE_API int			ps_r2_wait_sleep;

//	x - min (0), y - focus (1.4), z - max (100)
extern ECORE_API Fvector3		ps_r2_dof;
extern ECORE_API float			ps_r2_dof_sky;				//	distance to sky
extern ECORE_API float			ps_r2_dof_kernel_size;		//	7.0f

// ZergO:
extern ECORE_API float			ps_r2_ao_intensity; // OGSE
extern ECORE_API Fvector4		ps_r2_color_grading_params; // OGSE

extern ECORE_API float			ps_r2_refl_intensity;
extern ECORE_API float			ps_r2_refl_env_intensity;
extern ECORE_API float			ps_r2_refl_water_gloss;
extern ECORE_API float			ps_r2_refl_fresnel1;
extern ECORE_API float			ps_r2_refl_fresnel2;
extern ECORE_API float			ps_r2_refl_fresnel3;
//

enum
{
	R2FLAG_SUN					= (1<<0),
	R2FLAG_DETAILS_SHADOWS		= (1<<1),
	R2FLAG_TONEMAP				= (1<<2),
	R2FLAG_AA					= (1<<3),
	R2FLAG_GI					= (1<<4),
	R2FLAG_FASTBLOOM			= (1<<5),
	R2FLAG_GLOBALMATERIAL		= (1<<6),
	R2FLAG_ZFILL				= (1<<7),
	R2FLAG_R1LIGHTS				= (1<<8),
	R2FLAG_SUN_IGNORE_PORTALS	= (1<<9),

	R2FLAG_DETAIL_BUMP			= (1<<10),
	
	R2FLAG_EXP_SPLIT_SCENE					= (1<<11),
	R2FLAG_EXP_DONT_TEST_UNSHADOWED			= (1<<12),
	R2FLAG_EXP_DONT_TEST_SHADOWED			= (1<<13),

	R2FLAG_USE_NVDBT			= (1<<14),
	R2FLAG_USE_NVSTENCIL		= (1<<15),

	R2FLAG_EXP_MT_CALC			= (1<<16),

	R2FLAG_SOFT_WATER			= (1<<17),	//	Igor: need restart
	R2FLAG_SOFT_PARTICLES		= (1<<18),	//	Igor: need restart
	R2FLAG_VOLUMETRIC_LIGHTS	= (1<<19),
	R2FLAG_STEEP_PARALLAX		= (1<<20),
	R2FLAG_DOF					= (1<<21),

	// ZergO: new flags
	R2FLAG_AO					= (1<<22),
	R2FLAG_LENS_FLARES			= (1<<23),
	R2FLAG_REFLECTIONS			= (1<<24),
	R2FLAG_SOFT_SHADOWS			= (1<<25),
	R2FLAG_R1GLOWS				= (1<<26),

	R2FLAG_SUN_ZCULLING			= (1<<27),
};

enum
{
	R2FLAGEXT_SSAO_BLUR			= (1<<0),
	R2FLAGEXT_SSAO_OPT_DATA		= (1<<1),
	R2FLAGEXT_SSAO_HALF_DATA	= (1<<2),
	R2FLAGEXT_SSAO_HBAO			= (1<<3),
	R2FLAGEXT_SSAO_HDAO			= (1<<4)
};

// R3 //////////////////////////////////////////////////////////////////////////
extern ECORE_API Flags32		ps_r3_ls_flags;				// r3-only

extern ECORE_API u32			ps_r3_msaa;	//	=	0;
extern ECORE_API xr_token		qmsaa_token[];

extern ECORE_API u32			ps_r3_msaa_atest; //=	0;
extern ECORE_API xr_token		qmsaa__atest_token[];

extern ECORE_API u32			ps_r3_minmax_sm;//	=	0;
extern ECORE_API xr_token		qminmax_sm_token[];

extern ECORE_API float			ps_r3_dyn_wet_surf_near;	// 10.0f
extern ECORE_API float			ps_r3_dyn_wet_surf_far;		// 30.0f
extern ECORE_API int			ps_r3_dyn_wet_surf_sm_res;	// 256

enum 
{
	R3FLAG_DYN_WET_SURF			= (1<<0),
	R3FLAG_VOLUMETRIC_SMOKE		= (1<<1),

	//R3FLAG_MSAA				= (1<<28),
	R3FLAG_MSAA_HYBRID			= (1<<2),
	R3FLAG_MSAA_OPT				= (1<<3),
	R3FLAG_GBUFFER_OPT			= (1<<4),
	R3FLAG_USE_DX10_1			= (1<<5),
	//R3FLAG_MSAA_ALPHATEST		= (1<<31),
};

extern void						xrRender_initconsole	();
extern BOOL						xrRender_test_hw		();
