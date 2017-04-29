#include "stdafx.h"
#include "../../xrEngine/igame_persistent.h"
#include "../../xrEngine/irenderable.h"
#include "../xrRender/FBasicVisual.h"

const	float	tweak_COP_initial_offs			= 1200.f	;
const	float	tweak_ortho_xform_initial_offs	= 1000.f	;	//. ?
const	float	tweak_guaranteed_range			= 20.f		;	//. ?

//////////////////////////////////////////////////////////////////////////
// tables to calculate view-frustum bounds in world space
// note: D3D uses [0..1] range for Z
static Fvector3		corners [8]			= {
	{ -1, -1,  0 },		{ -1, -1, +1},
	{ -1, +1, +1 },		{ -1, +1,  0},
	{ +1, +1, +1 },		{ +1, +1,  0},
	{ +1, -1, +1 },		{ +1, -1,  0}
};
static int			facetable[6][4]		= {
	{ 6, 7, 5, 4 },		{ 1, 0, 7, 6 },
	{ 1, 2, 3, 0 },		{ 3, 2, 4, 5 },		
	// near and far planes
	{ 0, 3, 5, 7 },		{  1, 6, 4, 2 },
};
//////////////////////////////////////////////////////////////////////////
#define DW_AS_FLT(DW) (*(FLOAT*)&(DW))
#define FLT_AS_DW(F) (*(DWORD*)&(F))
#define FLT_SIGN(F) ((FLT_AS_DW(F) & 0x80000000L))
#define ALMOST_ZERO(F) ((FLT_AS_DW(F) & 0x7f800000L)==0)
#define IS_SPECIAL(F) ((FLT_AS_DW(F) & 0x7f800000L)==0x7f800000L)

//////////////////////////////////////////////////////////////////////////
// CoP
//////////////////////////////////////////////////////////////////////////
const u32 LIGHT_CUBOIDSIDEPOLYS_COUNT = 4;
const u32 LIGHT_CUBOIDVERTICES_COUNT = 2 * LIGHT_CUBOIDSIDEPOLYS_COUNT;

template <bool _debug>
class	FixedConvexVolume
{
public:

	struct	_poly
	{
		int				points[4];
		Fplane			plane;
	};

	xr_vector<sun::ray>			view_frustum_rays;
	sun::ray					view_ray;
	sun::ray					light_ray;
	Fvector3					light_cuboid_points[LIGHT_CUBOIDVERTICES_COUNT];
	_poly						light_cuboid_polys[LIGHT_CUBOIDSIDEPOLYS_COUNT];

public:

	void				compute_planes()
	{
		for (u32 it = 0; it< LIGHT_CUBOIDSIDEPOLYS_COUNT; it++)
		{
			_poly&			P = light_cuboid_polys[it];

			P.plane.build(light_cuboid_points[P.points[0]], light_cuboid_points[P.points[2]], light_cuboid_points[P.points[1]]);

			// verify
			if (_debug)
			{
				Fvector&		p0 = light_cuboid_points[P.points[0]];
				Fvector&		p1 = light_cuboid_points[P.points[1]];
				Fvector&		p2 = light_cuboid_points[P.points[2]];
				Fvector&		p3 = light_cuboid_points[P.points[3]];
				Fplane	p012;	p012.build(p0, p1, p2);
				Fplane	p123;	p123.build(p1, p2, p3);
				Fplane	p230;	p230.build(p2, p3, p0);
				Fplane	p301;	p301.build(p3, p0, p1);
				VERIFY(p012.n.similar(p123.n) && p012.n.similar(p230.n) && p012.n.similar(p301.n));
			}
		}
	}

	void		compute_caster_model_fixed(xr_vector<Fplane>& dest, Fvector3& translation, float map_size, bool clip_by_view_near)
	{
		translation.set(0.f, 0.f, 0.f);

		if (fis_zero(1 - abs(view_ray.D.dotproduct(light_ray.D)), EPS_S))
			return;

		// compute planes for each polygon.
		compute_planes();

		for (u32 i = 0; i < LIGHT_CUBOIDSIDEPOLYS_COUNT; i++)
			VERIFY(light_cuboid_polys[i].plane.classify(light_ray.P) > 0);

		int	align_planes[2];
		int	align_planes_count = 0;

		// find one or two planes that align to view frustum from behind. 
		for (u32 i = 0; i < LIGHT_CUBOIDSIDEPOLYS_COUNT; i++)
		{
			float tmp_dot = view_ray.D.dotproduct(light_cuboid_polys[i].plane.n);
			if (tmp_dot <= EPS_L)
				continue;

			align_planes[align_planes_count] = i;
			++align_planes_count;

			if (align_planes_count == 2)
				break;
		}


		Fvector align_vector;
		align_vector.set(0.f, 0.f, 0.f);

		// Align ray points to the align planes.
		for (int p = 0; p < align_planes_count; ++p)
		{
			// Hack !
			float min_dist = 10000;
			for (u32 i = 0; i < view_frustum_rays.size(); ++i)
			{
				float tmp_dist = 0;
				Fvector tmp_point = view_frustum_rays[i].P;

				tmp_dist = light_cuboid_polys[align_planes[p]].plane.classify(tmp_point);
				min_dist = _min(tmp_dist, min_dist);
			}

			Fvector shift = light_cuboid_polys[align_planes[p]].plane.n;
			shift.mul(min_dist);
			align_vector.add(shift);
		}

		translation.add(align_vector);

		// Move light ray by the alignment shift.
		light_ray.P.add(align_vector);

		// Here we can skip this stage us in the next pass we need only normals of planes.
		// in the next translate_light_model call will contain this shift as well.
		// translate_light_model	( align_vector );

		// Reset to reuse.
		align_vector.set(0.f, 0.f, 0.f);

		// Check if view edges intersect, and push planes................ 
		for (int p = 0; p < align_planes_count; ++p)
		{
			float max_mag = 0;
			for (u32 i = 0; i < view_frustum_rays.size(); ++i)
			{
				float plane_dot_ray = view_frustum_rays[i].D.dotproduct(light_cuboid_polys[align_planes[p]].plane.n);
				if (plane_dot_ray < 0)
				{
					Fvector per_plane_view;
					per_plane_view.crossproduct(light_cuboid_polys[align_planes[p]].plane.n, view_ray.D);
					Fvector per_view_to_plane;
					per_view_to_plane.crossproduct(per_plane_view, view_ray.D);

					float tmp_mag = -plane_dot_ray / view_frustum_rays[i].D.dotproduct(per_view_to_plane);

					max_mag = (max_mag < tmp_mag) ? tmp_mag : max_mag;
				}
			}

			if (fis_zero(max_mag))
				continue;

			VERIFY(max_mag <= 1.f);

			float dist = -light_cuboid_polys[align_planes[p]].plane.n.dotproduct(translation);
			align_vector.mad(light_cuboid_polys[align_planes[p]].plane.n, dist*max_mag);
		}

		translation.add(align_vector);
		light_ray.P.add(align_vector);
		translate_light_model(translation);


		// compute culling planes by rays as edges
		for (u32 i = 0; i< view_frustum_rays.size(); ++i)
		{
			Fvector tmp_vector;
			tmp_vector.crossproduct(view_frustum_rays[i].D, light_ray.D);

			// check if the vectors are parallel
			if (fis_zero(tmp_vector.square_magnitude(), EPS))
				continue;

			Fplane tmp_plane;
			tmp_plane.build(view_frustum_rays[i].P, tmp_vector);

			float sign = 0;
			if (check_cull_plane_valid(tmp_plane, sign, 5))
			{
				tmp_plane.n.mul(-sign);
				tmp_plane.d *= -sign;
				dest.push_back(tmp_plane);
			}
		}

		// compute culling planes by ray points pairs as edges
		if (clip_by_view_near && abs(view_ray.D.dotproduct(light_ray.D)) < 0.8)
		{
			Fvector perp_light_view, perp_light_to_view;
			perp_light_view.crossproduct(view_ray.D, light_ray.D);
			perp_light_to_view.crossproduct(perp_light_view, light_ray.D);

			Fplane plane;
			plane.build(view_ray.P, perp_light_to_view);

			float max_dist = -1000;
			for (u32 i = 0; i< view_frustum_rays.size(); ++i)
				max_dist = _max(plane.classify(view_frustum_rays[i].P), max_dist);

			for (u32 i = 0; i< view_frustum_rays.size(); ++i)
			{
				Fvector P = view_frustum_rays[i].P;
				P.mad(view_frustum_rays[i].D, 5);

				if (plane.classify(P) > max_dist)
				{
					max_dist = 0.f;
					break;
				}
			}

			if (max_dist > -1000)
			{
				plane.d += max_dist;
				dest.push_back(plane);
			}
		}

		for (u32 i = 0; i < LIGHT_CUBOIDSIDEPOLYS_COUNT; i++)
		{
			dest.push_back(light_cuboid_polys[i].plane);
			dest.back().n.mul(-1);
			dest.back().d *= -1;
			VERIFY(light_cuboid_polys[i].plane.classify(light_ray.P) > 0);
		}

		// Compute ray intersection with light model, this is needed to next cascade to start it's placement.
		for (u32 i = 0; i < view_frustum_rays.size(); ++i)
		{
			float min_dist = 2 * map_size;
			for (int p = 0; p < 4; ++p)
			{
				float dist;
				if ((light_cuboid_polys[p].plane.n.dotproduct(view_frustum_rays[i].D)) > -0.1)
					dist = map_size;
				else
					light_cuboid_polys[p].plane.intersectRayDist(view_frustum_rays[i].P, view_frustum_rays[i].D, dist);

				if (dist > EPS_L &&  dist < min_dist)
					min_dist = dist;
			}

			view_frustum_rays[i].P.mad(view_frustum_rays[i].D, min_dist);
		}
	}

	bool check_cull_plane_valid(Fplane const &plane, float &sign, float mad_factor = 0.f)
	{
		bool valid = false;
		bool oriented = false;
		float orient = 0;
		for (u32 j = 0; j< view_frustum_rays.size(); ++j)
		{
			float tmp_dist = 0.f;
			Fvector tmp_pt = view_frustum_rays[j].P;
			tmp_pt.mad(view_frustum_rays[j].D, mad_factor);
			tmp_dist = plane.classify(tmp_pt);

			if (fis_zero(tmp_dist, EPS_L))
				continue;

			if (!oriented)
			{
				orient = tmp_dist > 0.f ? 1.f : -1.f;
				valid = true;
				oriented = true;
				continue;
			}

			if (tmp_dist < 0 && orient < 0 || tmp_dist > 0 && orient > 0)
				continue;

			valid = false;
			break;
		}
		sign = orient;
		return valid;
	}

	void translate_light_model(Fvector translate)
	{
		Fmatrix trans_mat; trans_mat.translate(translate);
		for (int i = 0; i < LIGHT_CUBOIDSIDEPOLYS_COUNT; ++i)
			light_cuboid_polys[i].plane.d -= translate.dotproduct(light_cuboid_polys[i].plane.n);
	}
};

//////////////////////////////////////////////////////////////////////////
Fvector3		wform	(Fmatrix& m, Fvector3& v)
{
	Fvector4	r;
	r.x			= v.x*m._11 + v.y*m._21 + v.z*m._31 + m._41;
	r.y			= v.x*m._12 + v.y*m._22 + v.z*m._32 + m._42;
	r.z			= v.x*m._13 + v.y*m._23 + v.z*m._33 + m._43;
	r.w			= v.x*m._14 + v.y*m._24 + v.z*m._34 + m._44;
	// VERIFY		(r.w>0.f);
	float invW = 1.0f/r.w;
	Fvector3	r3 = { r.x*invW, r.y*invW, r.z*invW };
	return		r3;
}

void CRender::init_cascades()
{
	u32 cascade_count = 3;
	m_sun_cascades.resize(cascade_count);

	float fBias = -0.0000025f;

	m_sun_cascades[0].reset_chain = true;
	m_sun_cascades[0].size = 9;
	m_sun_cascades[0].bias = m_sun_cascades[0].size*fBias;

	m_sun_cascades[1].size = 40;
	m_sun_cascades[1].bias = m_sun_cascades[1].size*fBias;

 	m_sun_cascades[2].size = 160;
 	m_sun_cascades[2].bias = m_sun_cascades[2].size*fBias;
}

void CRender::render_sun_cascades ()
{ 
	bool b_need_to_render_sunshafts = (RImplementation.o.advancedpp && ps_r_sun_shafts && (g_pGamePersistent->Environment().CurrentEnv->m_fSunShaftsIntensity > 0.0001));
	bool last_cascade_chain_mode = m_sun_cascades.back().reset_chain;
	if (b_need_to_render_sunshafts)
		m_sun_cascades[m_sun_cascades.size()-1].reset_chain = true;

	for(u32 i = 0; i < m_sun_cascades.size(); ++i)
		render_sun_cascade (i);

	if (b_need_to_render_sunshafts)
		m_sun_cascades[m_sun_cascades.size()-1].reset_chain = last_cascade_chain_mode;
}

void CRender::render_sun_cascade (u32 cascade_ind)
{
	light* fuckingsun = (light*)Lights.sun._get();

	// calculate view-frustum bounds in world space
	Fmatrix	ex_project, ex_full, ex_full_inverse;
	{
		ex_project = Device.mProject;
		ex_full.mul					(ex_project,Device.mView);
		D3DXMatrixInverse			((D3DXMATRIX*)&ex_full_inverse,0,(D3DXMATRIX*)&ex_full);
	}

	// Compute volume(s) - something like a frustum for infinite directional light
	// Also compute virtual light position and sector it is inside
	CFrustum					cull_frustum;
	xr_vector<Fplane>			cull_planes;
	Fvector3					cull_COP;
	CSector*					cull_sector;
	Fmatrix						cull_xform;
	{
		FPU::m64r					();
		// Lets begin from base frustum
		Fmatrix		fullxform_inv	= ex_full_inverse;

		//******************************* Need to be placed after cuboid built **************************
		// Search for default sector - assume "default" or "outdoor" sector is the largest one
		//. hack: need to know real outdoor sector
		CSector*	largest_sector		= 0;
		float		largest_sector_vol	= 0;
		for	(u32 s=0; s<Sectors.size(); s++)
		{
			CSector*			S		= (CSector*)Sectors[s];
			dxRender_Visual*	V		= S->root();
			float				vol		= V->vis.box.getvolume();
			if (vol>largest_sector_vol)	{
				largest_sector_vol		= vol;
				largest_sector			= S;
			}
		}
		cull_sector	= largest_sector;

		// COP - 100 km away
		cull_COP.mad				(Device.vCameraPosition, fuckingsun->direction, -tweak_COP_initial_offs);

		// Create approximate ortho-xform
		// view: auto find 'up' and 'right' vectors
		Fmatrix						mdir_View, mdir_Project;
		Fvector						L_dir,L_up,L_right,L_pos;
		L_pos.set					(fuckingsun->position);
		L_dir.set					(fuckingsun->direction).normalize	();
		L_right.set					(1,0,0);					if (_abs(L_right.dotproduct(L_dir))>.99f)	L_right.set(0,0,1);
		L_up.crossproduct			(L_dir,L_right).normalize	();
		L_right.crossproduct		(L_up,L_dir).normalize		();
		mdir_View.build_camera_dir	(L_pos,L_dir,L_up);



		//////////////////////////////////////////////////////////////////////////
#ifdef	_DEBUG
		typedef		FixedConvexVolume<true>		t_cuboid;
#else
		typedef		FixedConvexVolume<false>	t_cuboid;
#endif

		t_cuboid light_cuboid;
		{
			// Initialize the first cascade rays, then each cascade will initialize rays for next one.
			if (cascade_ind == 0 || m_sun_cascades[cascade_ind].reset_chain)
			{
				Fvector3 near_p, edge_vec;
				for	(int p=0; p < 4; p++)	
				{
					near_p		= wform		(fullxform_inv,corners[facetable[4][p]]);
					edge_vec	= wform		(fullxform_inv,corners[facetable[5][p]]);
					edge_vec.sub(near_p);
					edge_vec.normalize();

					light_cuboid.view_frustum_rays.push_back (sun::ray(near_p,edge_vec));
				}
			}
			else
				light_cuboid.view_frustum_rays = m_sun_cascades[cascade_ind].rays;

			light_cuboid.view_ray.P		= Device.vCameraPosition;
			light_cuboid.view_ray.D		= Device.vCameraDirection;
			light_cuboid.light_ray.P	= L_pos;
			light_cuboid.light_ray.D	= L_dir;
		}

		// THIS NEED TO BE A CONSTATNT
		Fplane light_top_plane;
		light_top_plane.build_unit_normal(L_pos, L_dir);
		float dist = light_top_plane.classify(Device.vCameraPosition);

		float map_size = m_sun_cascades[cascade_ind].size;
		float map_size_d2 = map_size * 0.5f;

		D3DXMatrixOrthoOffCenterLH	((D3DXMATRIX*)&mdir_Project,-map_size_d2, map_size_d2, -map_size_d2, map_size_d2,  0.1, dist + map_size);

		// build viewport xform
		float	view_dim			= float(RImplementation.o.smapsize);
		Fmatrix	m_viewport			= {
			view_dim/2.f,	0.0f,				0.0f,		0.0f,
			0.0f,			-view_dim/2.f,		0.0f,		0.0f,
			0.0f,			0.0f,				1.0f,		0.0f,
			view_dim/2.f,	view_dim/2.f,		0.0f,		1.0f
		};
		Fmatrix				m_viewport_inv;
		D3DXMatrixInverse	((D3DXMATRIX*)&m_viewport_inv,0,(D3DXMATRIX*)&m_viewport);

		cull_xform.mul		(mdir_Project,mdir_View	);
		Fmatrix	cull_xform_inv; cull_xform_inv.invert(cull_xform);

		//		light_cuboid.light_cuboid_points.reserve		(9);
		for	(int p=0; p < 8; p++)	{
			Fvector3				xf	= wform		(cull_xform_inv,corners[p]);
			light_cuboid.light_cuboid_points[p] = xf;
		}

		// only side planes
		for (int plane=0; plane < 4; plane++)	
			for (int pt=0; pt < 4; pt++)	
			{
				int asd = facetable[plane][pt];
				light_cuboid.light_cuboid_polys[plane].points[pt] = asd;
			}


			Fvector lightXZshift;
			light_cuboid.compute_caster_model_fixed(cull_planes, lightXZshift, m_sun_cascades[cascade_ind].size, m_sun_cascades[cascade_ind].reset_chain);
			Fvector proj_view			= Device.vCameraDirection;
			proj_view.y					= 0;
			proj_view.normalize			();
//			lightXZshift.mad(proj_view, 20);

			// Initialize rays for the next cascade
			if (cascade_ind < m_sun_cascades.size()-1)
				m_sun_cascades[cascade_ind+1].rays = light_cuboid.view_frustum_rays;

#ifdef	_DEBUG

			static bool draw_debug = false;
			if( draw_debug && cascade_ind == 0 )
				for (u32 it=0; it<cull_planes.size(); it++)
					RImplementation.Target->dbg_addplane(cull_planes[it],it*0xFFF);
#endif

			Fvector cam_shifted			= L_pos;
			cam_shifted.add				(lightXZshift);

			// rebuild the view transform with the shift.
			mdir_View.identity			();
			mdir_View.build_camera_dir	(cam_shifted, L_dir, L_up);
			cull_xform.identity			();
			cull_xform.mul				(mdir_Project, mdir_View);
			cull_xform_inv.invert		(cull_xform);


			// Create frustum for query
			cull_frustum._clear			();
			for (u32 p=0; p<cull_planes.size(); p++)
				cull_frustum._add		(cull_planes[p]);

			{
				Fvector cam_proj = Device.vCameraPosition;
				const float		align_aim_step_coef = 4.f;
				cam_proj.set(floorf(cam_proj.x/align_aim_step_coef)+align_aim_step_coef/2, floorf(cam_proj.y/align_aim_step_coef)+align_aim_step_coef/2, floorf(cam_proj.z/align_aim_step_coef)+align_aim_step_coef/2);
				cam_proj.mul(align_aim_step_coef);
				Fvector	cam_pixel	= wform		(cull_xform, cam_proj);
				cam_pixel			= wform		(m_viewport, cam_pixel);
				Fvector shift_proj	= lightXZshift;
						cull_xform.transform_dir(shift_proj);
						m_viewport.transform_dir(shift_proj);

				const float	align_granularity = 4.f;
				shift_proj.x = shift_proj.x > 0 ? align_granularity : -align_granularity;
				shift_proj.y = shift_proj.y > 0 ? align_granularity : -align_granularity;
				shift_proj.z = 0;

				cam_pixel.x	= cam_pixel.x/align_granularity-floorf	(cam_pixel.x/align_granularity);
				cam_pixel.y	= cam_pixel.y/align_granularity-floorf	(cam_pixel.y/align_granularity);
				cam_pixel.x *= align_granularity;
				cam_pixel.y *= align_granularity;
				cam_pixel.z = 0;

				cam_pixel.sub		(shift_proj);

				m_viewport_inv.transform_dir	(cam_pixel);
				cull_xform_inv.transform_dir	(cam_pixel);
				Fvector diff		= cam_pixel;
				static float sign_test = -1.f;
				diff.mul			(sign_test);
				Fmatrix adjust;		adjust.translate(diff);
				cull_xform.mulB_44	(adjust);
			}

			m_sun_cascades[cascade_ind].xform = cull_xform;

			s32		limit					= RImplementation.o.smapsize-1;
			fuckingsun->X.D.minX			= 0;
			fuckingsun->X.D.maxX			= limit;
			fuckingsun->X.D.minY			= 0;
			fuckingsun->X.D.maxY			= limit;

			// full-xform
			FPU::m24r			();
	}

	// Begin SMAP-render
	{
		bool	bSpecialFull					= mapNormalPasses[1][0].size() || mapMatrixPasses[1][0].size() || mapSorted.size();
		VERIFY									(!bSpecialFull);
		HOM.Disable								();
		phase									= PHASE_SMAP;
		if (RImplementation.o.Tshadows)	r_pmask	(true,true);
		else							r_pmask	(true,false);
		//		fuckingsun->svis.begin					();
	}

	// Fill the database
	r_dsgraph_render_subspace				(cull_sector, &cull_frustum, cull_xform, cull_COP, TRUE);

	// Finalize & Cleanup
	fuckingsun->X.D.combine					= cull_xform;

	// Render shadow-map
	//. !!! We should clip based on shrinked frustum (again)
	{
		bool	bNormal							= mapNormalPasses[0][0].size() || mapMatrixPasses[0][0].size();
		bool	bSpecial						= mapNormalPasses[1][0].size() || mapMatrixPasses[1][0].size() || mapSorted.size();
		if (bNormal || bSpecial)	
		{
			Target->phase_smap_direct			(fuckingsun	, SE_SUN_FAR	);
			RCache.set_xform_world				(Fidentity					);
			RCache.set_xform_view				(Fidentity					);
			RCache.set_xform_project			(fuckingsun->X.D.combine	);	
			r_dsgraph_render_graph				(0);
			if (ps_r2_ls_flags.test(R2FLAG_DETAILS_SHADOWS))	
				Details->Render					();

			fuckingsun->X.D.transluent			= FALSE;
			if (bSpecial)						
			{
				fuckingsun->X.D.transluent			= TRUE;
				Target->phase_smap_direct_tsh		(fuckingsun, SE_SUN_FAR);
				r_dsgraph_render_graph				(1);			// normal level, secondary priority
				r_dsgraph_render_sorted				( );			// strict-sorted geoms
			}
		}
	}

	// End SMAP-render
	{
		//		fuckingsun->svis.end					();
		r_pmask									(true,false);
	}

	// Accumulate
	Target->phase_accumulator	();

	if (cascade_ind == 0)
		Target->accum_direct_cascade		(SE_SUN_NEAR, m_sun_cascades[cascade_ind].xform, m_sun_cascades[cascade_ind].xform, m_sun_cascades[cascade_ind].bias );
	else if (cascade_ind < m_sun_cascades.size()-1)
		Target->accum_direct_cascade		(SE_SUN_MIDDLE, m_sun_cascades[cascade_ind].xform, m_sun_cascades[cascade_ind-1].xform, m_sun_cascades[cascade_ind].bias);
	else
		Target->accum_direct_cascade		(SE_SUN_FAR, m_sun_cascades[cascade_ind].xform, m_sun_cascades[cascade_ind-1].xform, m_sun_cascades[cascade_ind].bias);


	// Restore XForms
	RCache.set_xform_world		(Fidentity			);
	RCache.set_xform_view		(Device.mView		);
	RCache.set_xform_project	(Device.mProject	);
}