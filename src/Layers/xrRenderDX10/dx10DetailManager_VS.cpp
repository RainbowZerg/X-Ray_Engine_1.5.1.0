#include "stdafx.h"
#include "../xrRender/DetailManager.h"

#include "../../xrEngine/igame_persistent.h"
#include "../../xrEngine/environment.h"

#include "../xrRenderDX10/dx10BufferUtils.h"

short QC (float v);
//{
//	int t=iFloor(v*float(quant)); clamp(t,-32768,32767);
//	return short(t&0xffff);
//}

void CDetailManager::hw_Render()
{
	// Render-prepare
	//	Update timer
	//	Can't use Device.fTimeDelta since it is smoothed! Don't know why, but smoothed value looks more choppy!
	float fDelta = Device.fTimeGlobal-m_global_time_old;
	if ( (fDelta<0) || (fDelta>1))	fDelta = 0.03;
	m_global_time_old = Device.fTimeGlobal;

	m_time_rot_1	+= (PI_MUL_2*fDelta/swing_current.rot1);
	m_time_rot_2	+= (PI_MUL_2*fDelta/swing_current.rot2);
	m_time_pos		+= fDelta*swing_current.speed;

	//float		tm_rot1		= (PI_MUL_2*Device.fTimeGlobal/swing_current.rot1);
	//float		tm_rot2		= (PI_MUL_2*Device.fTimeGlobal/swing_current.rot2);
	float		tm_rot1		= m_time_rot_1;
	float		tm_rot2		= m_time_rot_2;

	Fvector4	dir1,dir2;
	dir1.set				(_sin(tm_rot1),0,_cos(tm_rot1),0).normalize().mul(swing_current.amp1);
	dir2.set				(_sin(tm_rot2),0,_cos(tm_rot2),0).normalize().mul(swing_current.amp2);

	// Setup geometry and DMA
	RCache.set_Geometry		(hw_Geom);

	// Wave0
	float		scale			=	1.f/float(quant);
	Fvector4	wave;
	Fvector4	consts;
	consts.set				(scale,		scale,		ps_r__Detail_l_aniso,	ps_r__Detail_l_ambient);
	//wave.set				(1.f/5.f,		1.f/7.f,	1.f/3.f,	Device.fTimeGlobal*swing_current.speed);
	wave.set				(1.f/5.f,		1.f/7.f,	1.f/3.f,	m_time_pos);
	//RCache.set_c			(&*hwc_consts,	scale,		scale,		ps_r__Detail_l_aniso,	ps_r__Detail_l_ambient);				// consts
	//RCache.set_c			(&*hwc_wave,	wave.div(PI_MUL_2));	// wave
	//RCache.set_c			(&*hwc_wind,	dir1);																					// wind-dir
	//hw_Render_dump			(&*hwc_array,	1, 0, c_hdr );
	hw_Render_dump(consts, wave.div(PI_MUL_2), dir1, 1, 0);

	// Wave1
	//wave.set				(1.f/3.f,		1.f/7.f,	1.f/5.f,	Device.fTimeGlobal*swing_current.speed);
	wave.set				(1.f/3.f,		1.f/7.f,	1.f/5.f,	m_time_pos);
	//RCache.set_c			(&*hwc_wave,	wave.div(PI_MUL_2));	// wave
	//RCache.set_c			(&*hwc_wind,	dir2);																					// wind-dir
	//hw_Render_dump			(&*hwc_array,	2, 0, c_hdr );
	hw_Render_dump(consts, wave.div(PI_MUL_2), dir2, 2, 0);

	// Still
	consts.set				(scale,		scale,		scale,				1.f);
	//RCache.set_c			(&*hwc_s_consts,scale,		scale,		scale,				1.f);
	//RCache.set_c			(&*hwc_s_xform,	Device.mFullTransform);
	//hw_Render_dump			(&*hwc_s_array,	0, 1, c_hdr );
	hw_Render_dump(consts, wave.div(PI_MUL_2), dir2, 0, 1);
}

void CDetailManager::hw_Render_dump(const Fvector4 &consts, const Fvector4 &wave, const Fvector4 &wind, u32 var_id, u32 lod_id)
{
	static shared_str strConsts("consts");
	static shared_str strWave("wave");
	static shared_str strDir2D("dir2D");
	static shared_str strArray("array");
	static shared_str strXForm("xform");

	Device.Statistic->RenderDUMP_DT_Count	= 0;

	// Matrices and offsets
	u32		vOffset	=	0;
	u32		iOffset	=	0;

	vis_list& list	=	m_visibles	[var_id];

	// Iterate
	for (u32 O=0; O<objects.size(); O++)
	{
		CDetail&	Object				= *objects	[O];
		xr_vector <SlotItemVec* >& vis	= list		[O];
		if (!vis.empty())
		{
			for ( u32 iPass=0; iPass<Object.shader->E[lod_id]->passes.size(); ++iPass)
			{
				// Setup matrices + colors (and flush it as necessary)
				//RCache.set_Element				(Object.shader->E[lod_id]);
				RCache.set_Element				(Object.shader->E[lod_id], iPass);
				RImplementation.apply_lmaterial	();

				//	This could be cached in the corresponding consatant buffer
				//	as it is done for DX9
				RCache.set_c(strConsts, consts);
				RCache.set_c(strWave, wave);
				RCache.set_c(strDir2D, wind);
				RCache.set_c(strXForm, Device.mFullTransform);

				//ref_constant constArray = RCache.get_c(strArray);
				//VERIFY(constArray);

				//u32			c_base				= x_array->vs.index;
				//Fvector4*	c_storage			= RCache.get_ConstantCache_Vertex().get_array_f().access(c_base);
				Fvector4*	c_storage=0;
				//	Map constants to memory directly
				{
					void*	pVData;
					RCache.get_ConstantDirect( strArray, 
						hw_BatchSize*sizeof(Fvector4)*2,
						&pVData, 0, 0);
					c_storage = (Fvector4*) pVData;
				}
				VERIFY(c_storage);

				u32 dwBatch	= 0;

				xr_vector <SlotItemVec* >::iterator _vI = vis.begin();
				xr_vector <SlotItemVec* >::iterator _vE = vis.end();
				for (; _vI!=_vE; _vI++)
				{
					SlotItemVec*	items		= *_vI;
					SlotItemVecIt _iI			= items->begin();
					SlotItemVecIt _iE			= items->end();
					for (; _iI!=_iE; _iI++){
						if (!(*_iI)) continue;
#ifdef DETAILS_OPT
						float dist = Device.vCameraPosition.distance_to((*_iI)->mRotY.c);

						if (dist > ps_r__details_opt.z) {
							if (!(*_iI)->need_to_render_anyway[2]) continue;
						} else if (dist > ps_r__details_opt.y) {
							if (!(*_iI)->need_to_render_anyway[1]) continue;
						} else if (dist > ps_r__details_opt.x) {
							if (!(*_iI)->need_to_render_anyway[0]) continue;
						}
#endif
						SlotItem&	Instance	= **_iI;
						u32			base		= dwBatch*2;

						// Build color & matrix
						float		scale		= Instance.scale_calculated;
						Fmatrix&	M			= Instance.mRotY;
						float		h			= Instance.c_hemi;
						float		s			= Instance.c_sun;

						c_storage[base+0].set	(M._11*scale,	M._31*scale,	s,		h	);
						c_storage[base+1].set	(M._41,			M._42,		M._43,	scale	);

						dwBatch	++;
						if (dwBatch == hw_BatchSize)	{
							// flush
							Device.Statistic->RenderDUMP_DT_Count					+=	dwBatch;
							u32 dwCNT_verts			= dwBatch * Object.number_vertices;
							u32 dwCNT_prims			= (dwBatch * Object.number_indices)/3;
							//RCache.get_ConstantCache_Vertex().b_dirty				=	TRUE;
							//RCache.get_ConstantCache_Vertex().get_array_f().dirty	(c_base,c_base+dwBatch*4);
							RCache.Render			(D3DPT_TRIANGLELIST,vOffset, 0, dwCNT_verts,iOffset,dwCNT_prims);
							RCache.stat.r.s_details.add	(dwCNT_verts);

							// restart
							dwBatch					= 0;

							//	Remap constants to memory directly (just in case anything goes wrong)
							{
								void*	pVData;
								RCache.get_ConstantDirect( strArray, 
									hw_BatchSize*sizeof(Fvector4)*2,
									&pVData, 0, 0);
								c_storage = (Fvector4*) pVData;
							}
							VERIFY(c_storage);
						}
					}
				}
				// flush if nessecary
				if (dwBatch)
				{
					Device.Statistic->RenderDUMP_DT_Count	+= dwBatch;
					u32 dwCNT_verts			= dwBatch * Object.number_vertices;
					u32 dwCNT_prims			= (dwBatch * Object.number_indices)/3;
					//RCache.get_ConstantCache_Vertex().b_dirty				=	TRUE;
					//RCache.get_ConstantCache_Vertex().get_array_f().dirty	(c_base,c_base+dwBatch*4);
					RCache.Render				(D3DPT_TRIANGLELIST,vOffset,0,dwCNT_verts,iOffset,dwCNT_prims);
					RCache.stat.r.s_details.add	(dwCNT_verts);

					// restart
					dwBatch = 0;
				}

			}
			// Clean up
/*
			// KD: we must not clear vis on r2 since we want details shadows
			if ((ps_r2_ls_flags.test(R2FLAG_SUN_DETAILS) && (RImplementation.PHASE_SMAP == RImplementation.phase))										// phase smap with shadows
			|| (ps_r2_ls_flags.test(R2FLAG_SUN_DETAILS) && (RImplementation.PHASE_NORMAL == RImplementation.phase) && (!RImplementation.is_sun()))		// phase normal with shadows without sun
			|| (!ps_r2_ls_flags.test(R2FLAG_SUN_DETAILS) && (RImplementation.PHASE_NORMAL == RImplementation.phase)))									// phase normal without shadows

			vis.clear_not_free			();
*/
		}
		vOffset		+=	hw_BatchSize * Object.number_vertices;
		iOffset		+=	hw_BatchSize * Object.number_indices;
	}
}