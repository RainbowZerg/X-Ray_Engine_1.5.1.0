#include "stdafx.h"
#include "pch_script.h"
#include "torch.h"
#include "actor.h"
#include "../xrEngine/LightAnimLibrary.h"
#include "PhysicsShell.h"
#include "xrserver_objects_alife_items.h"
#include "ai_sounds.h"

#include "../Include/xrRender/Kinematics.h"
#include "../xrEngine/camerabase.h"
#include "inventory.h"

//#include "actorEffector.h"
#include "CustomOutfit.h"

static const float		TIME_2_HIDE					= 5.f;
static const float		TORCH_INERTION_CLAMP		= PI_DIV_6;
static const float		TORCH_INERTION_SPEED_MAX	= 7.5f;
static const float		TORCH_INERTION_SPEED_MIN	= 0.5f;
//static		 Fvector	TORCH_OFFSET				= {0,0,0};
//static		 Fvector	OMNI_OFFSET					= {-0.2f,+0.1f,-0.1f};
static const float		OPTIMIZATION_DISTANCE		= 100.f;

CTorch::CTorch(void) 
{
	light_render				= ::Render->light_create();
	light_render->set_type		(IRender_Light::SPOT);
	light_render->set_shadow	(true);
	light_render->set_volumetric_quality(1.0f);

	light_omni					= ::Render->light_create();
	light_omni->set_type		(IRender_Light::POINT);
	light_omni->set_shadow		(false);
	light_omni->set_flare		(false);

	m_switched_on				= false;
	glow_render					= ::Render->glow_create();
	lanim						= 0;
	lanim_flickering			= 0;
	fBrightness					= 1.f;

	m_prev_hp.set				(0,0);
	m_delta_h					= 0;
	m_is_flickering				= false;
	m_is_broken					= false;

	m_light_offset.set			(0,0,0);
	m_light_omni_offset.set		(0,0,0);

	m_light_color.set			(1, 1, 1, 1);
	m_light_range				= 0;

	m_light_is_volumetric		= false;
	m_light_volumetric_intensity = 0;
}

CTorch::~CTorch(void) 
{
	light_render.destroy	();
	light_omni.destroy		();
	glow_render.destroy		();

	HUD_SOUND_ITEM::DestroySound(m_FlashlightSwitchSnd);
}

inline bool CTorch::can_use_dynamic_lights	()
{
	if (!H_Parent())
		return				(true);

	CInventoryOwner			*owner = smart_cast<CInventoryOwner*>(H_Parent());
	if (!owner)
		return				(true);

	return					(owner->can_use_dynamic_lights());
}

void CTorch::Load(LPCSTR section) 
{
	inherited::Load			(section);
	light_trace_bone		= pSettings->r_string(section,"light_trace_bone");

	HUD_SOUND_ITEM::LoadSound(section, "snd_switch", m_FlashlightSwitchSnd, SOUND_TYPE_ITEM_USING);
}

void CTorch::Break(bool fatal)
{
	if (OnClient()) return;
	if (m_is_broken) return;

	if (m_switched_on)
		Switch (false);

	if (fatal)	
		m_is_broken = true;
	else		
		m_is_flickering = true;

//	sndBreaking.play_at_pos(0, Position());
}

bool CTorch::Enabled() const
{
	return m_switched_on;
}

bool CTorch::Broken(bool fatal) const
{
	return fatal ? m_is_broken : m_is_flickering;
}

void CTorch::Switch()
{
	if (OnClient()) return;

	Switch (!m_switched_on);
}

void CTorch::Switch	(bool light_on)
{
	if (!m_switched_on && m_is_broken) return;
	m_switched_on = light_on;
	if (can_use_dynamic_lights())
	{
		light_render->set_active(light_on);
		light_omni->set_active(light_on);
	}
	glow_render->set_active					(light_on);

	if (*light_trace_bone) 
	{
		IKinematics* pVisual				= smart_cast<IKinematics*>(Visual()); VERIFY(pVisual);
		u16 bi								= pVisual->LL_BoneID(light_trace_bone);

		pVisual->LL_SetBoneVisible			(bi,	light_on,	TRUE);
		pVisual->CalculateBones				(TRUE);
//.		pVisual->LL_SetBoneVisible			(bi,	light_on,	TRUE); //hack
	}

	if (light_on)
	{
		CActor *pA = smart_cast<CActor *>(H_Parent());

		if (pA)
			HUD_SOUND_ITEM::PlaySound(m_FlashlightSwitchSnd, pA->Position(), pA, true);
	}
}

BOOL CTorch::net_Spawn(CSE_Abstract* DC) 
{
	CSE_Abstract			*e	= (CSE_Abstract*)(DC);
	CSE_ALifeItemTorch		*torch	= smart_cast<CSE_ALifeItemTorch*>(e);
	R_ASSERT				(torch);
	cNameVisual_set			(torch->get_visual());

	R_ASSERT				(!CFORM());
	R_ASSERT				(smart_cast<IKinematics*>(Visual()));
	collidable.model		= xr_new<CCF_Skeleton>	(this);

	if (!inherited::net_Spawn(DC))
		return				(FALSE);
	
	bool b_r2				= !!psDeviceFlags.test(rsR2);
	b_r2					|= !!psDeviceFlags.test(rsR3);

	IKinematics* K			= smart_cast<IKinematics*>(Visual());
	CInifile* pUserData		= K->LL_UserData(); 
	R_ASSERT3				(pUserData,"Empty Torch user data!",torch->get_visual());

	const char* torch_sect = "torch_definition";

	lanim					= LALib.FindItem(pUserData->r_string(torch_sect,"color_animator"));
	lanim_flickering		= LALib.FindItem(pUserData->r_string(torch_sect,"color_animator_f"));
	guid_bone				= K->LL_BoneID	(pUserData->r_string(torch_sect,"guide_bone"));	VERIFY(guid_bone!=BI_NONE);

	m_light_color			= pUserData->r_fcolor				(torch_sect,(b_r2)?"color_r2":"color");
	m_light_range			= pUserData->r_float(torch_sect, (b_r2) ? "range_r2" : "range");
	light_render->set_flare (!!pUserData->r_bool				(torch_sect, "lens_flare"));

	Fcolor clr_o			= pUserData->r_fcolor				(torch_sect,(b_r2)?"omni_color_r2":"omni_color");
	float range_o			= pUserData->r_float				(torch_sect,(b_r2)?"omni_range_r2":"omni_range");
	light_omni->set_color	(clr_o);
	light_omni->set_range	(range_o);

	if (::Render->get_generation() != ::Render->GENERATION_R1)
		m_light_offset		= pUserData->r_fvector3(torch_sect, "light_offset");

	m_light_omni_offset		= pUserData->r_fvector3(torch_sect, "light_omni_offset");

	light_render->set_cone	(deg2rad(pUserData->r_float			(torch_sect,"spot_angle")));

	if (pUserData->line_exist(torch_sect, "spot_texture"))
		light_render->set_texture(pUserData->r_string				(torch_sect,"spot_texture"));

	glow_render->set_texture(pUserData->r_string				(torch_sect,"glow_texture"));
	glow_render->set_radius	(pUserData->r_float					(torch_sect,"glow_radius"));

	//включить/выключить фонарик
	Switch					(torch->m_active);
	VERIFY					(!torch->m_active || (torch->ID_Parent != 0xffff));

	m_delta_h				= PI_DIV_2-atan((m_light_range*0.5f)/_abs(m_light_offset.x));

	return					(TRUE);
}

void CTorch::net_Destroy() 
{
	Switch					(false);

	inherited::net_Destroy	();
}

void CTorch::OnH_A_Chield() 
{
	inherited::OnH_A_Chield			();
}

void CTorch::OnH_B_Independent	(bool just_before_destroy) 
{
	inherited::OnH_B_Independent	(just_before_destroy);

	Switch						(false);
}

void CTorch::UpdateCL() 
{
	inherited::UpdateCL			();

	if (!m_switched_on)			return;

	CBoneInstance			&BI = smart_cast<IKinematics*>(Visual())->LL_GetBoneInstance(guid_bone);
	Fmatrix					M;

	if (H_Parent()) 
	{
		CActor*	actor = smart_cast<CActor*>(H_Parent());

#pragma todo("ZergO: исправить кривое положение фонарика на голове ГГ от 3-го лица")	
		smart_cast<IKinematics*>(H_Parent()->Visual())->CalculateBones_Invalidate();

		if (H_Parent()->XFORM().c.distance_to_sqr(Device.vCameraPosition)<_sqr(OPTIMIZATION_DISTANCE) || GameID() != eGameIDSingle) {
			// near camera
			smart_cast<IKinematics*>(H_Parent()->Visual())->CalculateBones	();
			M.mul_43				(XFORM(),BI.mTransform);
		} else {
			// approximately the same
			M		= H_Parent()->XFORM		();
			H_Parent()->Center				(M.c);
			M.c.y	+= H_Parent()->Radius	()*2.f/3.f;
		}

		if (actor && actor->g_Alive())
		{
			if (actor->active_cam() == eacLookAt)
			{
				m_prev_hp.x		= angle_inertion_var(m_prev_hp.x,-actor->cam_Active()->yaw,TORCH_INERTION_SPEED_MIN,TORCH_INERTION_SPEED_MAX,TORCH_INERTION_CLAMP,Device.fTimeDelta);
				m_prev_hp.y		= angle_inertion_var(m_prev_hp.y,-actor->cam_Active()->pitch,TORCH_INERTION_SPEED_MIN,TORCH_INERTION_SPEED_MAX,TORCH_INERTION_CLAMP,Device.fTimeDelta);
			}
			else
			{
				m_prev_hp.x		= angle_inertion_var(m_prev_hp.x,-actor->cam_FirstEye()->yaw,TORCH_INERTION_SPEED_MIN,TORCH_INERTION_SPEED_MAX,TORCH_INERTION_CLAMP,Device.fTimeDelta);
				m_prev_hp.y		= angle_inertion_var(m_prev_hp.y,-actor->cam_FirstEye()->pitch,TORCH_INERTION_SPEED_MIN,TORCH_INERTION_SPEED_MAX,TORCH_INERTION_CLAMP,Device.fTimeDelta);
			}

			Fvector			dir,right,up;	
			dir.setHP		(m_prev_hp.x+m_delta_h,m_prev_hp.y);
			Fvector::generate_orthonormal_basis_normalized(dir,up,right);

			glow_render->set_position	(M.c);
			glow_render->set_direction	(dir);

			Fvector offset				= M.c; 
			offset.mad					(M.i,m_light_offset.x);
			offset.mad					(M.j,m_light_offset.y);
			offset.mad					(M.k,m_light_offset.z);
			light_render->set_position	(offset);
			light_render->set_rotation	(dir, right);
		}
		else 
		{
			light_render->set_position	(M.c);
			light_render->set_rotation	(M.k,M.i);

			glow_render->set_position	(M.c);
			glow_render->set_direction	(M.k);
		}

		light_omni->set_position	(M.c);
		light_omni->set_rotation	(M.k,M.i);
	}
	else 
	{
		if (getVisible() && m_pPhysicsShell) 
		{
			M.mul					(XFORM(),BI.mTransform);

			m_switched_on			= false;
			light_render->set_active(false);
			light_omni->set_active	(false);
			glow_render->set_active	(false);
		}
	}

	if (!m_switched_on)					return;

	// ZergO: update light params
	light_render->set_range					(m_light_range);

	light_render->set_volumetric			(m_light_is_volumetric);
	light_render->set_volumetric_distance	(1.0f);
	light_render->set_volumetric_intensity	(m_light_volumetric_intensity);

	fBrightness = m_light_color.intensity();

	Fcolor fclr;

	// calc color animator
	if ((lanim_flickering && m_is_flickering) || lanim)
	{
		int	frame;
		u32 clr = 0;

		if (lanim_flickering && m_is_flickering)
			clr = lanim_flickering->CalculateBGR(Device.fTimeGlobal, frame);
		else if (lanim)
			clr = lanim->CalculateBGR(Device.fTimeGlobal, frame);

		fclr.set		((float)color_get_B(clr), (float)color_get_G(clr), (float)color_get_R(clr), 1.f);
		fclr.mul_rgb	(fBrightness / 255.f);
	}
	else
		fclr = m_light_color;

	light_render->set_color					(fclr);
	light_omni->set_color					(fclr);
	glow_render->set_color					(fclr);
}


void CTorch::create_physic_shell()
{
	CPhysicsShellHolder::create_physic_shell();
}

void CTorch::activate_physic_shell()
{
	CPhysicsShellHolder::activate_physic_shell();
}

void CTorch::setup_physic_shell	()
{
	CPhysicsShellHolder::setup_physic_shell();
}

void CTorch::net_Export			(NET_Packet& P)
{
	inherited::net_Export		(P);

#pragma todo ("ZergO: сохранять радиус, цвет и еще что-нибудь (аниматор? )")

	BYTE F = 0;
	F |= (m_switched_on ? eActive : 0);

	const CActor *pA = smart_cast<const CActor *>(H_Parent());
	if (pA)
		F |= (pA->attached(this) ? eAttached : 0);

	F |= (m_is_flickering ? eFlickering : 0);

	P.w_u8(F);
	P.w_float_q8(m_fCondition, 0.0f, 1.0f);
}

void CTorch::net_Import			(NET_Packet& P)
{
	inherited::net_Import		(P);
	
	BYTE F = P.r_u8();

	bool new_m_switched_on		= !!(F & eActive);
		 m_is_flickering		= !!(F & eFlickering); 	// ZergO: added

	if (new_m_switched_on != m_switched_on)			
		Switch (new_m_switched_on);

	P.r_float_q8(m_fCondition, 0.0f, 1.0f);
}

bool  CTorch::can_be_attached		() const
{
	const CActor *pA = smart_cast<const CActor *>(H_Parent());
	if (pA) 
	{
		if ((const CTorch*)smart_cast<CTorch*>(pA->inventory().m_slots[GetSlot()].m_pIItem) != this)
			return false;
	}
	return true;
}
void CTorch::afterDetach			()
{
	inherited::afterDetach	();
	Switch					(false);
}
void CTorch::renderable_Render()
{
	inherited::renderable_Render();
}

void CTorch::enable(bool value)
{
	inherited::enable(value);

	if(!enabled() && m_switched_on)
		Switch				(false);

}

#define  CLASS_IMPL		CTorch
#define  target_0		light_render
#define  target_1		light_omni
#define	 target_2		glow_render		
#include "script_light_ext.h"
#undef   target_0
#undef	 target_1
#undef	 target_2 
#undef CLASS_IMPL

using namespace luabind;

#pragma optimize("s",on)
void CTorch::script_register(lua_State *L)
{
	module(L)
	[
		class_<CTorch,					CGameObject>("CTorch")
		.def(							constructor<>())
		.def("on",						&CTorch::Enabled)
		.def("broken",					&CTorch::Broken)
		.def("break",					&CTorch::Break)
		.def("enable",					(void (CTorch::*)(bool)) (&CTorch::Switch))
		.def("switch",					(void (CTorch::*)())	 (&CTorch::Switch))		

		.def("get_light",				&CTorch::GetLight)
		.def("set_animation",			&CTorch::SetAnimation)
		.def("set_angle",				&CTorch::SetAngle)
		.def("set_brightness",			&CTorch::SetBrightness)
		.def("set_color",				&CTorch::SetColor)
		.def("set_rgb",					&CTorch::SetRGB)
		.def("set_range",				&CTorch::SetRange)			
		.def("set_texture",				&CTorch::SetTexture)
		.def("set_virtual_size",		&CTorch::SetVirtualSize)

		.def("set_flare",				&CTorch::SetFlare)

		.def("set_volumetric",			&CTorch::SetVolumetric)
		.def("set_volumetric_intensity",&CTorch::SetVolumetricIntensity)
		.def("set_volumetric_quality",	&CTorch::SetVolumetricQuality)
		.def("set_volumetric_distance", &CTorch::SetVolumetricDistance)
	];
}