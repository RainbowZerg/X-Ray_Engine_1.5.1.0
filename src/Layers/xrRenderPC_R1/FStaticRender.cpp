// CRender.cpp: implementation of the CRender class.
//
//////////////////////////////////////////////////////////////////////

#include "stdafx.h"
#include "../../xrEngine/igame_persistent.h"
#include "../../xrEngine/environment.h"
#include "../xrRender/fbasicvisual.h"
#include "../../xrEngine/CustomHUD.h"
#include "../../xrEngine/xr_object.h"
#include "../../xrEngine/fmesh.h"
#include "../xrRender/SkeletonCustom.h"
#include "../xrRender/lighttrack.h"
#include "../xrRender/dxRenderDeviceRender.h"
#include "../xrRender/dxWallMarkArray.h"
#include "../xrRender/dxUIShader.h"
//#include "../../xrServerEntities/smart_cast.h"
 
using	namespace		R_dsgraph;

CRender													RImplementation;

//////////////////////////////////////////////////////////////////////////
ShaderElement*			CRender::rimp_select_sh_dynamic	(dxRender_Visual	*pVisual, float cdist_sq)
{
	switch (phase)		{
	case PHASE_NORMAL:	return (RImplementation.L_Projector->shadowing()?pVisual->shader->E[SE_R1_NORMAL_HQ]:pVisual->shader->E[SE_R1_NORMAL_LQ])._get();
	case PHASE_POINT:	return pVisual->shader->E[SE_R1_LPOINT]._get();
	case PHASE_SPOT:	return pVisual->shader->E[SE_R1_LSPOT]._get();
	default:			NODEFAULT;
	}
#ifdef DEBUG
	return	0;
#endif
}
//////////////////////////////////////////////////////////////////////////
ShaderElement*			CRender::rimp_select_sh_static	(dxRender_Visual	*pVisual, float cdist_sq)
{
	switch (phase)		{
	case PHASE_NORMAL:	return (((_sqrt(cdist_sq) - pVisual->vis.sphere.R)<44)?pVisual->shader->E[SE_R1_NORMAL_HQ]:pVisual->shader->E[SE_R1_NORMAL_LQ])._get();
	case PHASE_POINT:	return pVisual->shader->E[SE_R1_LPOINT]._get();
	case PHASE_SPOT:	return pVisual->shader->E[SE_R1_LSPOT]._get();
	default:			NODEFAULT;
	}
#ifdef DEBUG
	return	0;
#endif
}

//////////////////////////////////////////////////////////////////////////
void					CRender::create					()
{
	L_DB				= 0;
	L_Shadows			= 0;
	L_Projector			= 0;

	Device.seqFrame.Add	(this,REG_PRIORITY_HIGH+0x12345678);

	// c-setup
	dxRenderDeviceRender::Instance().Resources->RegisterConstantSetup("L_dynamic_pos",		&r1_dlight_binder_PR);
	dxRenderDeviceRender::Instance().Resources->RegisterConstantSetup("L_dynamic_color",	&r1_dlight_binder_color);
	dxRenderDeviceRender::Instance().Resources->RegisterConstantSetup("L_dynamic_xform",	&r1_dlight_binder_xform);

	// distortion
	u32		v_dev	= CAP_VERSION(HW.Caps.raster_major, HW.Caps.raster_minor);
	u32		v_need	= CAP_VERSION(1,4);
	if ( v_dev >= v_need )						o.distortion = TRUE;
	else										o.distortion = FALSE;
	if (strstr(Core.Params,"-nodistort"))		o.distortion = FALSE;
	Msg				("* distortion: %s, dev(%d),need(%d)",o.distortion?"used":"unavailable",v_dev,v_need);
	m_skinning					= -1;

	// disasm
	o.disasm					= (strstr(Core.Params,"-disasm"))?		TRUE	:FALSE	;
	o.forceskinw				= (strstr(Core.Params,"-skinw"))?		TRUE	:FALSE	;
	o.detail_textures			= !ps_r2_ls_flags.test(R1FLAG_DETAIL_TEXTURES);
	c_ldynamic_props			= "L_dynamic_props";

	m_bMakeAsyncSS				= false;

//---------
	Target						= xr_new<CRenderTarget>		();
//---------
	//
	Models						= xr_new<CModelPool>		();
	L_Dynamic					= xr_new<CLightR_Manager>	();
	PSLibrary.OnCreate			();
//.	HWOCC.occq_create			(occq_size);

	::PortalTraverser.initialize();
}

void					CRender::destroy				()
{
	m_bMakeAsyncSS				= false;
	::PortalTraverser.destroy	();
//.	HWOCC.occq_destroy			();
	PSLibrary.OnDestroy			();
	
	xr_delete					(L_Dynamic);
	xr_delete					(Models);
	
	//*** Components
	xr_delete					(Target);
	Device.seqFrame.Remove		(this);

	r_dsgraph_destroy			();
}

void					CRender::reset_begin			()
{
	// KD: let's reload details while changed details options on vid_restart
	if (b_loaded && ((dm_current_size != dm_size) || (ps_r__Detail_density != ps_current_detail_density)))
	{
		Details->Unload();
		xr_delete(Details);
	}

	xr_delete					(Target);
}

void					CRender::reset_end				()
{
	Target						=	xr_new<CRenderTarget>	();

	// KD: let's reload details while changed details options on vid_restart
	if (b_loaded && ((dm_current_size != dm_size) || (ps_r__Detail_density	!= ps_current_detail_density)))
	{
		Details					=	xr_new<CDetailManager>	();
		Details->Load();
	}

	if (L_Projector)			L_Projector->invalidate		();
}

void					CRender::OnFrame				()
{
	Models->DeleteQueue	();
}

// Implementation
IRender_ObjectSpecific*	CRender::ros_create				(IRenderable* parent)					{ return xr_new<CROS_impl>();			}
void					CRender::ros_destroy			(IRender_ObjectSpecific* &p)			{ xr_delete(p);							}
IRenderVisual*			CRender::model_Create			(LPCSTR name, IReader* data)			{ return Models->Create(name,data);		}
IRenderVisual*			CRender::model_CreateChild		(LPCSTR name, IReader* data)			{ return Models->CreateChild(name,data);}
IRenderVisual*			CRender::model_Duplicate		(IRenderVisual* V)						{ return Models->Instance_Duplicate((dxRender_Visual*)V);	}
void					CRender::model_Delete			(IRenderVisual* &V, BOOL bDiscard)		
{ 
	dxRender_Visual* pVisual = (dxRender_Visual*)V;
	Models->Delete(pVisual, bDiscard);
	V = 0;
}
IRender_DetailModel*	CRender::model_CreateDM			(IReader*F)
{
	CDetail*	D		= xr_new<CDetail> ();
	D->Load				(F);
	return D;
}
void					CRender::model_Delete			(IRender_DetailModel* & F)
{
	if (F)
	{
		CDetail*	D	= (CDetail*)F;
		D->Unload		();
		xr_delete		(D);
		F				= NULL;
	}
}
IRenderVisual*			CRender::model_CreatePE			(LPCSTR name)	
{ 
	PS::CPEDef*	SE		= PSLibrary.FindPED	(name);		R_ASSERT3(SE,"Particle effect doesn't exist",name);
	return				Models->CreatePE	(SE);
}

IRenderVisual*			CRender::model_CreateParticles	(LPCSTR name)	
{ 
	PS::CPEDef*	SE		= PSLibrary.FindPED	(name);
	if (SE) return		Models->CreatePE	(SE);
	else{
		PS::CPGDef*	SG	= PSLibrary.FindPGD	(name);		R_ASSERT3(SG,"Particle effect or group doesn't exist",name);
		return			Models->CreatePG	(SG);
	}
}
void					CRender::models_Prefetch		()					{ Models->Prefetch	();}
void					CRender::models_Clear			(BOOL b_complete)	{ Models->ClearPool	(b_complete);}

ref_shader				CRender::getShader				(int id)			{ VERIFY(id<int(Shaders.size()));	return Shaders[id];	}
IRender_Portal*			CRender::getPortal				(int id)			{ VERIFY(id<int(Portals.size()));	return Portals[id];	}
IRender_Sector*			CRender::getSector				(int id)			{ VERIFY(id<int(Sectors.size()));	return Sectors[id];	}
IRender_Sector*			CRender::getSectorActive		()					{ return pLastSector;									}
IRenderVisual*			CRender::getVisual				(int id)			{ VERIFY(id<int(Visuals.size()));	return Visuals[id];	}
D3DVERTEXELEMENT9*		CRender::getVB_Format			(int id)			{ VERIFY(id<int(DCL.size()));		return DCL[id].begin();	}
IDirect3DVertexBuffer9*	CRender::getVB					(int id)			{ VERIFY(id<int(VB.size()));		return VB[id];		}
IDirect3DIndexBuffer9*	CRender::getIB					(int id)			{ VERIFY(id<int(IB.size()));		return IB[id];		}
IRender_Target*			CRender::getTarget				()					{ return Target;										}
FSlideWindowItem*		CRender::getSWI					(int id)			{ VERIFY(id<int(SWIs.size()));		return &SWIs[id];	}

IRender_Light*			CRender::light_create			()					{ return L_DB->Create();								}

IRender_Glow*			CRender::glow_create			()					{ return xr_new<CGlow>();								}

void					CRender::flush					()					{ r_dsgraph_render_graph	(0);						}

BOOL					CRender::occ_visible			(vis_data& P)		{ return HOM.visible(P);								}
BOOL					CRender::occ_visible			(sPoly& P)			{ return HOM.visible(P);								}
BOOL					CRender::occ_visible			(Fbox& P)			{ return HOM.visible(P);								}
ENGINE_API	extern BOOL g_bRendering;
void					CRender::add_Visual				(IRenderVisual* V )
{
	VERIFY				(g_bRendering);
	add_leafs_Dynamic	((dxRender_Visual*)V);									
}
void					CRender::add_Geometry			(IRenderVisual* V ){ add_Static((dxRender_Visual*)V,View->getMask());						}
void					CRender::add_StaticWallmark		(ref_shader& S, const Fvector& P, float s, CDB::TRI* T, Fvector* verts)
{
	if (T->suppress_wm)	return;
	VERIFY2							(_valid(P) && _valid(s) && T && verts && (s>EPS_L), "Invalid static wallmark params");
	Wallmarks->AddStaticWallmark	(T,verts,P,&*S,s);
}

void CRender::add_StaticWallmark			(IWallMarkArray *pArray, const Fvector& P, float s, CDB::TRI* T, Fvector* V)
{
	dxWallMarkArray *pWMA = (dxWallMarkArray *)pArray;
	ref_shader *pShader = pWMA->dxGenerateWallmark();
	if (pShader) add_StaticWallmark		(*pShader, P, s, T, V);
}

void CRender::add_StaticWallmark			(const wm_shader& S, const Fvector& P, float s, CDB::TRI* T, Fvector* V)
{
	dxUIShader* pShader = (dxUIShader*)&*S;
	add_StaticWallmark		(pShader->hShader, P, s, T, V);
}

void					CRender::clear_static_wallmarks	()
{
	if (Wallmarks)
		Wallmarks->clear				();
}

void					CRender::add_SkeletonWallmark	(intrusive_ptr<CSkeletonWallmark> wm)
{
	Wallmarks->AddSkeletonWallmark				(wm);
}
void					CRender::add_SkeletonWallmark	(const Fmatrix* xf, CKinematics* obj, ref_shader& sh, const Fvector& start, const Fvector& dir, float size)
{
	Wallmarks->AddSkeletonWallmark				(xf, obj, sh, start, dir, size);
}
void					CRender::add_SkeletonWallmark	(const Fmatrix* xf, IKinematics* obj, IWallMarkArray *pArray, const Fvector& start, const Fvector& dir, float size)
{
	dxWallMarkArray *pWMA = (dxWallMarkArray *)pArray;
	ref_shader *pShader = pWMA->dxGenerateWallmark();
	if (pShader) add_SkeletonWallmark(xf, (CKinematics*)obj, *pShader, start, dir, size);
}
void					CRender::add_Occluder			(Fbox2&	bb_screenspace	)
{
	VERIFY					(_valid(bb_screenspace));
	HOM.occlude				(bb_screenspace);
}

#include "../../xrEngine/PS_instance.h"
void					CRender::set_Object				(IRenderable*		O )	
{
	VERIFY					(g_bRendering);
	val_pObject				= O;		// NULL is OK, trust me :)
	if (val_pObject)		{
		VERIFY(dynamic_cast<CObject*>(O)||dynamic_cast<CPS_Instance*>(O));
		if (O->renderable.pROS) { VERIFY(dynamic_cast<CROS_impl*>(O->renderable.pROS)); }
	}
	if (PHASE_NORMAL==phase)	{
		if (L_Shadows)
			L_Shadows->set_object	(O);
		
		if (L_Projector)
			L_Projector->set_object	(O);
	} else {
		if (L_Shadows)
			L_Shadows->set_object(0);

		if (L_Projector)
			L_Projector->set_object	(0);
	}
}
void					CRender::apply_object			(IRenderable*		O )
{
	if (0==O)			return	;
	if (PHASE_NORMAL==phase	&& O->renderable_ROS())		{
		CROS_impl& LT		= *((CROS_impl*)O->renderable.pROS);
		VERIFY(dynamic_cast<CObject*>(O)||dynamic_cast<CPS_Instance*>(O));
		VERIFY(dynamic_cast<CROS_impl*>(O->renderable.pROS));
		float o_hemi		= 0.5f*LT.get_hemi						();
		float o_sun			= 0.5f*LT.get_sun						();
		RCache.set_c		(c_ldynamic_props,o_sun,o_sun,o_sun,o_hemi);
		// shadowing
		if ((LT.shadow_recv_frame==Device.dwFrame) && O->renderable_ShadowReceive())	
			RImplementation.L_Projector->setup	(LT.shadow_recv_slot);
	}
}

//////////////////////////////////////////////////////////////////////
// Construction/Destruction
//////////////////////////////////////////////////////////////////////
CRender::CRender	()
{
}

CRender::~CRender	()
{
}

void	CRender::rmNear		()
{
	IRender_Target* T	=	getTarget	();
	D3DVIEWPORT9 VP		=	{0,0,T->get_width(),T->get_height(),0,0.02f };
	CHK_DX				(HW.pDevice->SetViewport(&VP));
}
void	CRender::rmFar		()
{
	IRender_Target* T	=	getTarget	();
	D3DVIEWPORT9 VP		=	{0,0,T->get_width(),T->get_height(),0.99999f,1.f };
	CHK_DX				(HW.pDevice->SetViewport(&VP));
}
void	CRender::rmNormal	()
{
	IRender_Target* T	=	getTarget	();
	D3DVIEWPORT9 VP		= {0,0,T->get_width(),T->get_height(),0,1.f };
	CHK_DX				(HW.pDevice->SetViewport(&VP));
}

extern u32 g_r;
void	CRender::Render		()
{
	g_r											= 1;
	Device.Statistic->RenderDUMP.Begin();
	// Begin
	Target->Begin								();
	o.vis_intersect								= FALSE			;
	phase										= PHASE_NORMAL	;
	r_dsgraph_render_hud						();				// hud
	r_dsgraph_render_graph						(0);			// normal level
	if(Details)Details->Render					();				// grass / details
	r_dsgraph_render_lods						(true,false);	// lods - FB

	g_pGamePersistent->Environment().RenderSky	();				// sky / sun
	g_pGamePersistent->Environment().RenderClouds	();				// clouds

	r_pmask										(true,false);	// disable priority "1"
	o.vis_intersect								= TRUE			;
	HOM.Disable									();
	L_Dynamic->render							();				// addititional light sources
	if(Wallmarks){
		g_r										= 0;
		Wallmarks->Render						();				// wallmarks has priority as normal geometry
	}
	HOM.Enable									();
	o.vis_intersect								= FALSE			;
	phase										= PHASE_NORMAL	;
	r_pmask										(true,true);	// enable priority "0" and "1"
	if(L_Shadows)L_Shadows->render				();				// ... and shadows
	r_dsgraph_render_lods						(false,true);	// lods - FB
	r_dsgraph_render_graph						(1);			// normal level, secondary priority
	PortalTraverser.fade_render					();				// faded-portals
	r_dsgraph_render_sorted						();				// strict-sorted geoms
	if(L_Glows)L_Glows->Render					();				// glows
	g_pGamePersistent->Environment().RenderFlares	();				// lens-flares
	g_pGamePersistent->Environment().RenderLast	();				// rain/thunder-bolts

	// Postprocess, if necessary
	Target->End									();
	if (L_Projector) L_Projector->finalize		();

	// HUD
	Device.Statistic->RenderDUMP.End	();
}

void	CRender::ApplyBlur4		(FVF::TL4uv* pv, u32 w, u32 h, float k)
{
	float	_w					= float(w);
	float	_h					= float(h);
	float	kw					= (1.f/_w)*k;
	float	kh					= (1.f/_h)*k;
	Fvector2					p0,p1;
	p0.set						(.5f/_w, .5f/_h);
	p1.set						((_w+.5f)/_w, (_h+.5f)/_h );
	u32		_c					= 0xffffffff;

	// Fill vertex buffer
	pv->p.set(EPS,			float(_h+EPS),	EPS,1.f); pv->color=_c; pv->uv[0].set(p0.x-kw,p1.y-kh);pv->uv[1].set(p0.x+kw,p1.y+kh);pv->uv[2].set(p0.x+kw,p1.y-kh);pv->uv[3].set(p0.x-kw,p1.y+kh);pv++;
	pv->p.set(EPS,			EPS,			EPS,1.f); pv->color=_c; pv->uv[0].set(p0.x-kw,p0.y-kh);pv->uv[1].set(p0.x+kw,p0.y+kh);pv->uv[2].set(p0.x+kw,p0.y-kh);pv->uv[3].set(p0.x-kw,p0.y+kh);pv++;
	pv->p.set(float(_w+EPS),float(_h+EPS),	EPS,1.f); pv->color=_c; pv->uv[0].set(p1.x-kw,p1.y-kh);pv->uv[1].set(p1.x+kw,p1.y+kh);pv->uv[2].set(p1.x+kw,p1.y-kh);pv->uv[3].set(p1.x-kw,p1.y+kh);pv++;
	pv->p.set(float(_w+EPS),EPS,			EPS,1.f); pv->color=_c; pv->uv[0].set(p1.x-kw,p0.y-kh);pv->uv[1].set(p1.x+kw,p0.y+kh);pv->uv[2].set(p1.x+kw,p0.y-kh);pv->uv[3].set(p1.x-kw,p0.y+kh);pv++;
}

#include "../../xrEngine/GameFont.h"
void	CRender::Statistics	(CGameFont* _F)
{
	CGameFont&	F	= *_F;
	F.OutNext	(" **** Occ-Q(%03.1f) **** ",100.f*f32(stats.o_culled)/f32(stats.o_queries?stats.o_queries:1));
	F.OutNext	(" total  : %2d",	stats.o_queries	);	stats.o_queries = 0;
	F.OutNext	(" culled : %2d",	stats.o_culled	);	stats.o_culled	= 0;
	F.OutSkip	();
#ifdef DEBUG
	HOM.stats	();
#endif
}

#pragma comment(lib,"d3dx9.lib")
HRESULT	CRender::shader_compile			(
		LPCSTR							name,
		LPCSTR                          pSrcData,
		UINT                            SrcDataLen,
		void*							_pDefines,
		void*							_pInclude,
		LPCSTR                          pFunctionName,
		LPCSTR                          pTarget,
		DWORD                           Flags,
		void*							_ppShader,
		void*							_ppErrorMsgs,
		void*							_ppConstantTable)
{
	D3DXMACRO						defines			[128];
	int								def_it			= 0;
	CONST D3DXMACRO*                pDefines		= (CONST D3DXMACRO*)	_pDefines;
	if (pDefines)	{
		// transfer existing defines
		for (;;def_it++)	{
			if (0==pDefines[def_it].Name)	break;
			defines[def_it]			= pDefines[def_it];
		}
	}
	// options
	if (o.forceskinw)		{
		defines[def_it].Name		=	"SKIN_COLOR";
		defines[def_it].Definition	=	"1";
		def_it						++;
	}
	if (m_skinning<0)		{
		defines[def_it].Name		=	"SKIN_NONE";
		defines[def_it].Definition	=	"1";
		def_it						++;
	}
	if (0==m_skinning)		{
		defines[def_it].Name		=	"SKIN_0";
		defines[def_it].Definition	=	"1";
		def_it						++;
	}
	if (1==m_skinning)		{
		defines[def_it].Name		=	"SKIN_1";
		defines[def_it].Definition	=	"1";
		def_it						++;
	}
	if (2==m_skinning)		{
		defines[def_it].Name		=	"SKIN_2";
		defines[def_it].Definition	=	"1";
		def_it						++;
	}
	if (3==m_skinning)		{
		defines[def_it].Name		=	"SKIN_3";
		defines[def_it].Definition	=	"1";
		def_it						++;
	}
	if (4==m_skinning)		{
		defines[def_it].Name		=	"SKIN_4";
		defines[def_it].Definition	=	"1";
		def_it						++;
	}
	// finish
	defines[def_it].Name			=	0;
	defines[def_it].Definition		=	0;
	def_it							++;
	R_ASSERT						(def_it<128);

	LPD3DXINCLUDE                   pInclude		= (LPD3DXINCLUDE)		_pInclude;
	LPD3DXBUFFER*                   ppShader		= (LPD3DXBUFFER*)		_ppShader;
	LPD3DXBUFFER*                   ppErrorMsgs		= (LPD3DXBUFFER*)		_ppErrorMsgs;
	LPD3DXCONSTANTTABLE*            ppConstantTable	= (LPD3DXCONSTANTTABLE*)_ppConstantTable;
#ifdef	D3DXSHADER_USE_LEGACY_D3DX9_31_DLL	//	December 2006 and later
	HRESULT		_result	= D3DXCompileShader(pSrcData,SrcDataLen,defines,pInclude,pFunctionName,pTarget,Flags|D3DXSHADER_USE_LEGACY_D3DX9_31_DLL,ppShader,ppErrorMsgs,ppConstantTable);
#else
	HRESULT		_result	= D3DXCompileShader(pSrcData,SrcDataLen,defines,pInclude,pFunctionName,pTarget,Flags,ppShader,ppErrorMsgs,ppConstantTable);
#endif

	if (SUCCEEDED(_result) && o.disasm)
	{
		ID3DXBuffer*		code	= *((LPD3DXBUFFER*)_ppShader);
		ID3DXBuffer*		disasm	= 0;
		D3DXDisassembleShader		(LPDWORD(code->GetBufferPointer()), FALSE, 0, &disasm );
		string_path			dname;
		strconcat			(sizeof(dname),dname,"disasm\\",name,('v'==pTarget[0])?".vs":".ps" );
		IWriter*			W		= FS.w_open("$logs$",dname);
		W->w				(disasm->GetBufferPointer(),disasm->GetBufferSize());
		FS.w_close			(W);
		_RELEASE			(disasm);
	}
	return		_result;
}

