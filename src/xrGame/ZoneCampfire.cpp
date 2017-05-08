#include "stdafx.h"
#include "ZoneCampfire.h"
#include "Level.h"
#include "ParticlesObject.h"
#include "GamePersistent.h"
#include "../xrEngine/LightAnimLibrary.h"
//#include "../Include/xrRender/RenderVisual.h"
//#include "../Include/xrRender/Kinematics.h"
//#include "../Include/xrRender/KinematicsAnimated.h"
//#include "xrServer_Objects_ALife_Monsters.h"
//#include "zone_effector.h"

CZoneCampfire::CZoneCampfire():m_pDisabledParticles(NULL),m_pEnablingParticles(NULL),m_turned_on(true),m_turn_time(0)
{
//	m_light_bone		= BI_NONE;
//	m_visual_str		= "";
}

CZoneCampfire::~CZoneCampfire()
{
	CParticlesObject::Destroy	(m_pDisabledParticles);
	CParticlesObject::Destroy	(m_pEnablingParticles);
	m_disabled_sound.destroy	();
}

void CZoneCampfire::Load(LPCSTR section)
{
	inherited::Load(section);

#pragma todo("ZergO: Temp solution! Remove that hacks when i will be able to build LE")
//	m_visual_str				= pSettings->r_string(section, "visual");

	m_enabling_particles_str	= pSettings->r_string(section, "enabling_particles");
	m_disabled_particles_str	= pSettings->r_string(section, "disabled_particles");
	m_disabled_sound_str		= pSettings->r_string(section, "disabled_sound");
}

BOOL CZoneCampfire::net_Spawn(CSE_Abstract* DC)
{
	if (!inherited::net_Spawn(DC)) return FALSE;

//	CSE_ALifeZoneVisual	*pZ = smart_cast<CSE_ALifeZoneVisual*>(DC);

//	pZ->set_visual(m_visual_str);
//	cNameVisual_set(m_visual_str);

//	Msg("%s - visual_name = %s", cName().c_str(), pZ->visual_name.c_str());
//	R_ASSERT(Visual());

//	IKinematics* pK				= smart_cast<IKinematics*>(Visual());
//	m_light_bone				= pK->LL_BoneID("bone_lamp");
//	R_ASSERT2(m_light_bone != BI_NONE, "can't find bone_lamp in");

//	IKinematicsAnimated* pKA	= smart_cast<IKinematicsAnimated*>(Visual());
//	pKA->PlayCycle				("idle");

//	pK->CalculateBones_Invalidate();
//	pK->CalculateBones			(TRUE);

//	setVisible					(TRUE);
	return						TRUE;
}

void CZoneCampfire::GoEnabledState()
{
	inherited::GoEnabledState();
	
	if (m_pDisabledParticles)
	{
		m_pDisabledParticles->Stop	(FALSE);
		CParticlesObject::Destroy	(m_pDisabledParticles);
	}

	m_disabled_sound.stop			();
	m_disabled_sound.destroy		();

	m_pEnablingParticles			= CParticlesObject::Create(m_enabling_particles_str, FALSE);
	m_pEnablingParticles->UpdateParent(XFORM(),zero_vel);
	m_pEnablingParticles->Play		(false);
}

void CZoneCampfire::GoDisabledState()
{
	inherited::GoDisabledState		();

	R_ASSERT						(NULL==m_pDisabledParticles);
	m_pDisabledParticles			= CParticlesObject::Create(m_disabled_particles_str, FALSE);
	m_pDisabledParticles->UpdateParent(XFORM(),zero_vel);
	m_pDisabledParticles->Play		(false);
	
	m_disabled_sound.create			(m_disabled_sound_str, st_Effect, sg_SourceType);
	m_disabled_sound.play_at_pos	(0, Position(), true);
}

#define OVL_TIME 3000
void CZoneCampfire::Switch(bool enabled)
{
	if (psDeviceFlags.test(rsR2|rsR3))
	{
		m_turn_time = Device.dwTimeGlobal + OVL_TIME;
		m_turned_on = enabled;
		enabled ? GoEnabledState() : GoDisabledState();
	}
}

bool CZoneCampfire::Enabled() const
{
	return m_turned_on;
}

IRender_Light *CZoneCampfire::GetLight(int target) const
{
	switch (target)
	{
	case 0: return m_pIdleLight._get();
	case 1: return m_pLight._get();
	default: return NULL;
	}
}

void CZoneCampfire::shedule_Update(u32 dt)
{
	if (m_pIdleParticles)
	{
		Fvector vel;
		vel.mul(GamePersistent().Environment().wind_blast_direction,GamePersistent().Environment().wind_strength_factor);
		m_pIdleParticles->UpdateParent(XFORM(),vel);
	}
	inherited::shedule_Update(dt);
}


void CZoneCampfire::PlayIdleParticles(bool bIdleLight)
{
	if (m_turn_time == 0 || m_turn_time-Device.dwTimeGlobal < (OVL_TIME - 2000))
	{
		inherited::PlayIdleParticles(bIdleLight);
		if (m_pEnablingParticles)
		{
			m_pEnablingParticles->Stop	(FALSE);
			CParticlesObject::Destroy	(m_pEnablingParticles);
		}
	}
}

void CZoneCampfire::StopIdleParticles(bool bIdleLight)
{
	if (m_turn_time == 0 || m_turn_time-Device.dwTimeGlobal < (OVL_TIME - 500))
		inherited::StopIdleParticles(bIdleLight);
}

BOOL CZoneCampfire::AlwaysTheCrow()
{
	if (m_turn_time) return TRUE;

	return inherited::AlwaysTheCrow();
}

void CZoneCampfire::UpdateWorkload(u32 dt)
{
#if 0
	m_iPreviousStateTime	= m_iStateTime;
	m_iStateTime			+= (int)dt;

	if (!IsEnabled())		
	{
		if (m_actor_effector)
			m_actor_effector->Stop();

		return;
	}

	UpdateIdleLight();

	switch(m_eZoneState)
	{
	case eZoneStateIdle:
		IdleState();
		break;
	case eZoneStateAwaking:
		AwakingState();
		break;
	case eZoneStateBlowout:
		BlowoutState();
		break;
	case eZoneStateAccumulate:
		AccumulateState();
		break;
	case eZoneStateDisabled:
		break;
	default: NODEFAULT;
	}

	if (Level().CurrentEntity()) 
	{
		Fvector P			= Device.vCameraPosition;
		P.y					-= 0.9f;
		float radius		= 1.0f;
		CalcDistanceTo		(P, m_fDistanceToCurEntity, radius);

		if (m_actor_effector)
			m_actor_effector->Update(m_fDistanceToCurEntity, radius);
	}

	if (m_pLight && m_pLight->get_active())
		UpdateBlowoutLight	();
#else
	inherited::UpdateWorkload(dt);
#endif

	if (m_turn_time > Device.dwTimeGlobal)
	{
		float k = float(m_turn_time - Device.dwTimeGlobal) / float(OVL_TIME);

		if (m_turned_on)
		{
			k = 1.0f - k;
			PlayIdleParticles	(true);
			StartIdleLight		();
		}
		else
			StopIdleParticles(false);

		if (m_pIdleLight && m_pIdleLight->get_active())
		{
			VERIFY		(m_pIdleLAnim);
			int frame	= 0;
			u32 clr		= m_pIdleLAnim->CalculateBGR(Device.fTimeGlobal,frame);
			Fcolor		fclr;
			fclr.set	(((float)color_get_B(clr)/255.f)*k,((float)color_get_G(clr)/255.f)*k, ((float)color_get_R(clr)/255.f)*k, 1.f);
			
			float range = (m_fIdleLightRange + Random.randF(m_fIdleLightRangeRandMin, m_fIdleLightRangeRandMax)) * k;

			m_pIdleLight->set_range	(range);
			m_pIdleLight->set_color	(fclr);
		}
	}
	else if (m_turn_time)
	{
		m_turn_time = 0;
		if (m_turned_on)
			PlayIdleParticles(true);
		else
			StopIdleParticles(true);
	}
}
/*
void CZoneCampfire::UpdateIdleLight()
{
	if (!m_pIdleLight || !m_pIdleLight->get_active()) return;

	VERIFY(m_pIdleLAnim);

	int frame = 0;
	u32 clr = m_pIdleLAnim->CalculateBGR(Device.fTimeGlobal, frame); // возвращает в формате BGR
	Fcolor fclr; fclr.set((float)color_get_B(clr) / 255.f, (float)color_get_G(clr) / 255.f, (float)color_get_R(clr) / 255.f, 1.f);

	float range = m_fIdleLightRange + ::Random.randF(m_fIdleLightRangeRandMin, m_fIdleLightRangeRandMax);
	m_pIdleLight->set_range(range);
	m_pIdleLight->set_color(fclr);

	// ZergO: синхронизация позиции источника света с позицией кости
	IKinematics* pK = smart_cast<IKinematics*>(Visual());
	pK->CalculateBones();

	Fmatrix xf;
	Fmatrix& M					= pK->LL_GetTransform(m_light_bone);
	xf.mul						(XFORM(), M);
	m_pIdleLight->set_rotation	(xf.k, xf.i);
	xf.c.y						+= m_fIdleLightHeight;
	m_pIdleLight->set_position	(xf.c);
//	Msg("%s - light pos = %d,%d,%d", cName().c_str(), xf.c.x, xf.c.y, xf.c.z);
}
*/