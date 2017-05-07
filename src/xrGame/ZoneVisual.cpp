#include "stdafx.h"
#include "CustomZone.h"
#include "xrServer_Objects_ALife_Monsters.h"
#include "../Include/xrRender/Kinematics.h"
#include "../Include/xrRender/KinematicsAnimated.h"
#include "ZoneVisual.h"
#include "../xrEngine/LightAnimLibrary.h"

CVisualZone::CVisualZone()
{
	m_use_attack_animation	= false;
}

CVisualZone::~CVisualZone()
{
}

BOOL CVisualZone::net_Spawn(CSE_Abstract* DC)
{
	if (!inherited::net_Spawn(DC)) return FALSE;

	CSE_ALifeZoneVisual	*pZ		= smart_cast<CSE_ALifeZoneVisual*>(DC);

	IKinematics* pK				= smart_cast<IKinematics*>(Visual());
	IKinematicsAnimated	*pKA	= smart_cast<IKinematicsAnimated*>(Visual());

	m_idle_animation = pKA->ID_Cycle_Safe(pZ->startup_animation);
	R_ASSERT2(m_idle_animation.valid(), make_string("object[%s]: cannot find startup animation[%s] in model[%s]", cName().c_str(), pZ->startup_animation.c_str(), cNameVisual().c_str()));

	if (m_use_attack_animation)
	{
		m_attack_animation = pKA->ID_Cycle_Safe(pZ->attack_animation);
		R_ASSERT2(m_attack_animation.valid(), make_string("object[%s]: cannot find attack animation[%s] in model[%s]", cName().c_str(), pZ->attack_animation.c_str(), cNameVisual().c_str()));
	}

	pKA->PlayCycle(m_idle_animation);
	setVisible		(TRUE);

	return TRUE;
}

void CVisualZone::SwitchZoneState(EZoneState new_state)
{
	if (m_use_attack_animation)
	{
		IKinematicsAnimated* pKA = smart_cast<IKinematicsAnimated*>(Visual());

		if (m_eZoneState == eZoneStateBlowout && new_state != eZoneStateBlowout)
			pKA->PlayCycle(m_idle_animation);
		else if (m_eZoneState != eZoneStateBlowout && new_state == eZoneStateBlowout)
			pKA->PlayCycle(m_attack_animation);
	}

	inherited::SwitchZoneState(new_state);

}
void CVisualZone::Load(LPCSTR section)
{
	inherited::Load(section);

	if (pSettings->line_exist(section, "attack_animation_start") && pSettings->line_exist(section, "attack_animation_end"))
	{
		m_use_attack_animation	= true;
		m_dwAttackAnimaionStart	= pSettings->r_u32(section, "attack_animation_start");
		m_dwAttackAnimaionEnd	= pSettings->r_u32(section, "attack_animation_end");
		R_ASSERT2(m_dwAttackAnimaionStart < m_dwAttackAnimaionEnd, "attack_animation_start must be less then attack_animation_end");
	}
}

void CVisualZone::UpdateBlowout()
{
	inherited::UpdateBlowout();

	if (m_use_attack_animation)
	{
		IKinematicsAnimated* pKA = smart_cast<IKinematicsAnimated*>(Visual());

		if (m_dwAttackAnimaionStart >= (u32)m_iPreviousStateTime && m_dwAttackAnimaionStart < (u32)m_iStateTime)
			pKA->PlayCycle(m_attack_animation);

		if (m_dwAttackAnimaionEnd	>= (u32)m_iPreviousStateTime && m_dwAttackAnimaionEnd	< (u32)m_iStateTime)
			pKA->PlayCycle(m_idle_animation);
	}
}