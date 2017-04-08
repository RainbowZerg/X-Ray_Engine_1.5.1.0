#include "stdafx.h"
#include "mosquitobald.h"
#include "ParticlesObject.h"
#include "level.h"
#include "physicsshellholder.h"
#include "../xrEngine/xr_collide_form.h"

// ZergO
#include "entity_alive.h"
#include "CharacterPhysicsSupport.h"
#include "phmovementcontrol.h"
#include "actor.h"

CMosquitoBald::CMosquitoBald(void) 
{
	m_fHitImpulseScale		= 1.f;
	m_bLastBlowoutUpdate	= false;
}

CMosquitoBald::~CMosquitoBald(void) 
{
}

void CMosquitoBald::Load(LPCSTR section) 
{
	inherited::Load(section);

	m_fThrowImpulseAlive = READ_IF_EXISTS(pSettings, r_float, section, "throw_impulse_alive", 800.f);
}


bool CMosquitoBald::BlowoutState()
{
	bool result = inherited::BlowoutState();
	if(!result)
	{
		m_bLastBlowoutUpdate = false;
		UpdateBlowout();
	}
	else if(!m_bLastBlowoutUpdate)
	{
		m_bLastBlowoutUpdate = true;
		UpdateBlowout();
	}

	return result;
}

void CMosquitoBald::Affect(SZoneObjectInfo* O) 
{
	CPhysicsShellHolder *pGameObject = smart_cast<CPhysicsShellHolder*>(O->object);
	if(!pGameObject) return;

	if(O->zone_ignore) return;

	Fvector P; 
	XFORM().transform_tiny(P,CFORM()->getSphere().P);

	VERIFY(!pGameObject->getDestroy());

	float dist = pGameObject->Position().distance_to(P) - pGameObject->Radius();
	float power = Power(dist>0.f?dist:0.f);
	if (power > 0.01f) 
	{
		float power_critical = 0.0f;
		float impulse = m_fHitImpulseScale*power*pGameObject->GetMass();

		Fvector hit_dir; 
		hit_dir.set(::Random.randF(-0.5f,0.5f), 
					::Random.randF( 0.8f,1.0f), // ( 0.0f,1.0f), // ZergO: нужно чтобы импульс был направлен более вверх
					::Random.randF(-0.5f,0.5f)); 

		Fvector position_in_bone_space;
		position_in_bone_space.set(0.f, 0.f, 0.f);

		CEntityAlive* EA = smart_cast<CEntityAlive*>(pGameObject);
//		CActor* A = smart_cast<CActor*>(EA);
//		CActor* A = smart_cast<CActor*>(pGameObject);
		if (EA)
		{
//			Msg("applying impulse to [%s], with power [%.2f]", A->Name(), impulse);

			// copy vector
			Fvector hit_dir_alive;
			hit_dir_alive.set(hit_dir);

			hit_dir.normalize();
			// кость 2 (bip01_pelvis) для моделей из папки actors/
			CreateHit(EA->ID(), ID(), hit_dir, power, power_critical, 2, position_in_bone_space, impulse, m_eHitTypeBlowout);

			if (EA->g_Alive())
			{
				impulse = m_fThrowImpulseAlive*m_fHitImpulseScale*power*EA->GetMass();
				hit_dir_alive.mul(impulse);

				EA->character_physics_support()->movement()->SetVelocity(hit_dir_alive);
			}
		}
		else
		{
			// кость 0 (link) для остальных моделей
			CreateHit(pGameObject->ID(), ID(), hit_dir, power, power_critical, 0, position_in_bone_space, impulse, m_eHitTypeBlowout);
		}

		PlayHitParticles(pGameObject);
	}
}
