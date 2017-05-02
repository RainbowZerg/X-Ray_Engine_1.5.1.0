#include "stdafx.h"

#if RENDER == R_R1
#include "../../xrEngine/CustomHUD.h"
#include "../../xrEngine/xr_object.h"
#include "lighttrack.h"
#include "fbasicvisual.h"

static bool gm_Nearer = false;

IC	void gm_SetNearer(bool bNearer)
{
	if (bNearer != gm_Nearer)
	{
		gm_Nearer = bNearer;
		if (gm_Nearer)	RImplementation.rmNear();
		else			RImplementation.rmNormal();
	}
}

ICF bool pred_sp_sort (ISpatial* _1, ISpatial* _2)
{
	float	d1	= _1->spatial.sphere.P.distance_to_sqr(Device.vCameraPosition);
	float	d2	= _2->spatial.sphere.P.distance_to_sqr(Device.vCameraPosition);
	return	d1 < d2;
}
#endif

extern float		r_ssaDISCARD;
extern float		r_ssaDONTSORT;
extern float		r_ssaLOD_A;
extern float		r_ssaLOD_B;
extern float		r_ssaHZBvsTEX;
extern float		r_ssaGLOD_start,	r_ssaGLOD_end;
#if RENDER != R_R1
extern float		r_dtex_range;
#endif

void CRender::Calculate		()
{
	Device.Statistic->RenderCALC.Begin();

	// Transfer to global space to avoid deep pointer access
	IRender_Target* T				= getTarget	();
	float fov_factor				= _sqr (90.f / Device.fFOV);
	float g_fSCREEN					= float(T->get_width()*T->get_height())*fov_factor*(EPS_S + ps_r__LOD);
	r_ssaDISCARD					= _sqr(ps_r__ssaDISCARD)		/g_fSCREEN;
	r_ssaDONTSORT					= _sqr(ps_r__ssaDONTSORT/3)		/g_fSCREEN;
	r_ssaLOD_A						= _sqr(ps_r__ssaLOD_A/3)		/g_fSCREEN;
	r_ssaLOD_B						= _sqr(ps_r__ssaLOD_B/3)		/g_fSCREEN;
	r_ssaGLOD_start					= _sqr(ps_r__GLOD_ssa_start/3)	/g_fSCREEN;
	r_ssaGLOD_end					= _sqr(ps_r__GLOD_ssa_end/3)	/g_fSCREEN;
	r_ssaHZBvsTEX					= _sqr(ps_r__ssaHZBvsTEX/3)		/g_fSCREEN;
#if RENDER != R_R1
	r_dtex_range					= ps_r2_df_parallax_range * g_fSCREEN / (1024.f * 768.f);
#else
	// Frustum & HOM rendering
	ViewBase.CreateFromMatrix		(Device.mFullTransform,FRUSTUM_P_LRTB|FRUSTUM_P_FAR);
	View							= 0;
	HOM.Enable						();
	HOM.Render						(ViewBase);
	gm_SetNearer					(FALSE);
	phase							= PHASE_NORMAL;
#endif

	// Detect camera-sector
	if (!vLastCameraPos.similar(Device.vCameraPosition,EPS_S)) 
	{
		CSector* pSector		= (CSector*)detectSector(Device.vCameraPosition);
		if (pSector && (pSector!=pLastSector))
			g_pGamePersistent->OnSectorChanged( translateSector(pSector) );

		if (0==pSector) pSector = pLastSector;
		pLastSector = pSector;
		vLastCameraPos.set(Device.vCameraPosition);
	}

	// Check if camera is too near to some portal - if so force DualRender
	if (rmPortals) 
	{
#if RENDER != R_R1
		float	eps			= VIEWPORT_NEAR+EPS_L;
#else
		float	eps			= EPS_L * 2;
#endif
		Fvector box_radius; box_radius.set(eps,eps,eps);
		Sectors_xrc.box_options	(CDB::OPT_FULL_TEST);
		Sectors_xrc.box_query	(rmPortals,Device.vCameraPosition,box_radius);
		for (int K=0; K<Sectors_xrc.r_count(); K++)	
		{
			CPortal*	pPortal		= (CPortal*) Portals[rmPortals->get_tris()[Sectors_xrc.r_begin()[K].id].dummy];
			pPortal->bDualRender	= TRUE;
		}
	}

	//
#if RENDER != R_R1
	Lights.Update();

	// Check if we touch some light even trough portal
	lstRenderables.clear();
	g_SpatialSpace->q_sphere(lstRenderables,0,STYPE_LIGHTSOURCE,Device.vCameraPosition,EPS_L);
	for (u32 _it=0; _it<lstRenderables.size(); _it++)	
	{
		ISpatial*	spatial		= lstRenderables[_it];		spatial->spatial_updatesector	();
		CSector*	sector		= (CSector*)spatial->spatial.sector;
		if	(0==sector)										continue;	// disassociated from S/P structure

		VERIFY							(spatial->spatial.type & STYPE_LIGHTSOURCE);
		// lightsource
		light*			L				= (light*)	(spatial->dcast_Light());
		VERIFY							(L);
		Lights.add_light				(L);
	}
#else
	if (L_DB)
		L_DB->Update();

		// Main process
	marker	++;
	if (pLastSector)
	{
		// Traverse sector/portal structure
		PortalTraverser.traverse	
			(
			pLastSector,
			ViewBase,
			Device.vCameraPosition,
			Device.mFullTransform,
			CPortalTraverser::VQ_HOM + CPortalTraverser::VQ_SSA + CPortalTraverser::VQ_FADE
			);

		// Determine visibility for static geometry hierrarhy
		if  (psDeviceFlags.test(rsDrawStatic))	{
			for (u32 s_it=0; s_it<PortalTraverser.r_sectors.size(); s_it++)
			{
				CSector*	sector		= (CSector*)PortalTraverser.r_sectors[s_it];
				dxRender_Visual*	root	= sector->root();
				for (u32 v_it=0; v_it<sector->r_frustums.size(); v_it++)
				{
					set_Frustum			(&(sector->r_frustums[v_it]));
					add_Geometry		(root);
				}
			}
		}

		// Traverse object database
		if  (psDeviceFlags.test(rsDrawDynamic))	{
			g_SpatialSpace->q_frustum
				(
				lstRenderables,
				ISpatial_DB::O_ORDERED,
				STYPE_RENDERABLE + STYPE_LIGHTSOURCE,
				ViewBase
				);

			// Exact sorting order (front-to-back)
			std::sort							(lstRenderables.begin(),lstRenderables.end(),pred_sp_sort);

			// Determine visibility for dynamic part of scene
			set_Object							(0);
			g_hud->Render_First					();	// R1 shadows
			g_hud->Render_Last					();

			u32 uID_LTRACK						= 0xffffffff;
			if (phase==PHASE_NORMAL)			{
				uLastLTRACK	++;
				if (lstRenderables.size())		uID_LTRACK	= uLastLTRACK%lstRenderables.size();

				// update light-vis for current entity / actor
				CObject*	O					= g_pGameLevel->CurrentViewEntity();
				if (O)		{
					CROS_impl*	R					= (CROS_impl*) O->ROS();
					if (R)		R->update			(O);
				}
			}
			for (u32 o_it=0; o_it<lstRenderables.size(); o_it++)
			{
				ISpatial*	spatial		= lstRenderables[o_it];		spatial->spatial_updatesector	();
				CSector*	sector		= (CSector*)spatial->spatial.sector	;
				if	(0==sector)										continue;	// disassociated from S/P structure
				if	(PortalTraverser.i_marker != sector->r_marker)	continue;	// inactive (untouched) sector
				for (u32 v_it=0; v_it<sector->r_frustums.size(); v_it++)
				{
					set_Frustum			(&(sector->r_frustums[v_it]));
					if (!View->testSphere_dirty(spatial->spatial.sphere.P,spatial->spatial.sphere.R))	continue;

					if (spatial->spatial.type & STYPE_RENDERABLE)
					{
						// renderable
						IRenderable*	renderable		= spatial->dcast_Renderable	();
						if (0==renderable)	{
							// It may be an glow
							CGlow*		glow				= dynamic_cast<CGlow*>(spatial);
							VERIFY							(glow);
							L_Glows->add					(glow);
						} else {
							// Occlusion
							vis_data&		v_orig			= renderable->renderable.visual->getVisData();
							vis_data		v_copy			= v_orig;
							v_copy.box.xform				(renderable->renderable.xform);
							BOOL			bVisible		= HOM.visible(v_copy);
							v_orig.accept_frame				= v_copy.accept_frame;
							v_orig.marker					= v_copy.marker;
							v_orig.hom_frame				= v_copy.hom_frame;
							v_orig.hom_tested				= v_copy.hom_tested;
							if (!bVisible)					break;	// exit loop on frustums

							// rendering
							if (o_it==uID_LTRACK && renderable->renderable_ROS())	{
								// track lighting environment
								CROS_impl*		T = (CROS_impl*)renderable->renderable_ROS();
								T->update			(renderable);
							}
							set_Object						(renderable);
							renderable->renderable_Render	();
							set_Object						(0);	//? is it needed at all
						}
					} else {
						VERIFY								(spatial->spatial.type & STYPE_LIGHTSOURCE);
						// lightsource
						light*			L					= (light*)	spatial->dcast_Light	();
						VERIFY								(L);
						if (L->spatial.sector)				{
							vis_data&		vis		= L->get_homdata	( );
							if	(HOM.visible(vis))	L_DB->add_light		(L);
						}
					}
					break;	// exit loop on frustums
				}
			}
		}

		// Calculate miscelaneous stuff
		L_Shadows->calculate								();
		L_Projector->calculate								();
	}
	else
	{
		set_Object											(0);
		/*
		g_pGameLevel->pHUD->Render_First					();	
		g_pGameLevel->pHUD->Render_Last						();	

		// Calculate miscelaneous stuff
		L_Shadows->calculate								();
		L_Projector->calculate								();
		*/
	}
#endif

	// End calc
	Device.Statistic->RenderCALC.End();
}
