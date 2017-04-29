#include "stdafx.h"
#include "poltergeist.h"
#include "../../../PhysicsShell.h"
#include "../../../level.h"
#include "../../../material_manager.h"
#include "../../../level_debug.h"
#include "sound_player.h"

CPolterSpecialAbility::CPolterSpecialAbility(CPoltergeist *polter)
{
	m_object					= polter;

	m_particles_object			= 0;
	m_particles_object_electro	= 0;
}


CPolterSpecialAbility::~CPolterSpecialAbility()
{
	CParticlesObject::Destroy	(m_particles_object);
	CParticlesObject::Destroy	(m_particles_object_electro);
	m_sound_base.destroy		();
}

void CPolterSpecialAbility::load(LPCSTR section)
{
	m_particles_show			= pSettings->r_string(section,"Particles_Show");
	m_particles_hidden			= pSettings->r_string(section,"Particles_Hidden");
	m_particles_damage			= pSettings->r_string(section,"Particles_Damage");
	m_particles_death			= pSettings->r_string(section,"Particles_Death");
	m_particles_idle			= pSettings->r_string(section,"Particles_Idle");

	m_sound_base.create			(pSettings->r_string(section,"Sound_Hidden_Idle"), st_Effect, SOUND_TYPE_MONSTER_TALKING);

	m_last_hit_frame			= 0;
}

void CPolterSpecialAbility::update_schedule()
{
	if (m_object->g_Alive()) 
	{
		if (!m_sound_base._feedback()) 
			m_sound_base.play_at_pos(m_object, m_object->Position());
		else 
			m_sound_base.set_position(m_object->Position());
	}
}

void CPolterSpecialAbility::on_hide()
{
	VERIFY(m_particles_object == 0);
	if (!m_object->g_Alive())
		return;

	m_particles_object			= m_object->PlayParticles	(m_particles_hidden, m_object->Position(),Fvector().set(0.0f,0.1f,0.0f), false);
	m_particles_object_electro	= m_object->PlayParticles	(m_particles_idle, m_object->Position(),Fvector().set(0.0f,0.1f,0.0f), false);
}

void CPolterSpecialAbility::on_show()
{
	m_object->PlayParticles(m_particles_show, m_object->Position(), Fvector().set(0.0f, 1.0f, 0.0f), TRUE, FALSE);

	if (m_particles_object)			CParticlesObject::Destroy(m_particles_object);
	if (m_particles_object_electro) CParticlesObject::Destroy(m_particles_object_electro);
}

void CPolterSpecialAbility::update_frame()
{
	if (m_particles_object)			m_particles_object->SetXFORM		(m_object->XFORM());
	if (m_particles_object_electro)	m_particles_object_electro->SetXFORM(m_object->XFORM());
}

void CPolterSpecialAbility::on_die()
{
	Fvector particles_position		= m_object->m_current_position;
	particles_position.y			+= m_object->target_height;

	m_object->m_sound_player->play	(EPolterSounds::eSndDeathHidden);
	m_object->PlayParticles			(m_particles_death, particles_position, Fvector().set(0.0f,1.0f,0.0f), TRUE, FALSE);

	CParticlesObject::Destroy		(m_particles_object_electro);
	CParticlesObject::Destroy		(m_particles_object);
	m_sound_base.destroy			();
}

void CPolterSpecialAbility::on_hit(SHit* pHDS)
{
	if (m_object->g_Alive() && (pHDS->hit_type == ALife::eHitTypeFireWound) && (Device.dwFrame != m_last_hit_frame)) 
	{
		if (BI_NONE != pHDS->bone()) 
		{
			//вычислить координаты попадания
			IKinematics* V = smart_cast<IKinematics*>(m_object->Visual());

			Fvector start_pos = pHDS->bone_space_position();
			Fmatrix& m_bone = V->LL_GetBoneInstance(pHDS->bone()).mTransform;
			m_bone.transform_tiny	(start_pos);
			m_object->XFORM().transform_tiny	(start_pos);

			m_object->m_sound_player->play(EPolterSounds::eSndHitHidden);
			m_object->PlayParticles(m_particles_damage, start_pos, Fvector().set(0.f,1.f,0.f));
		}
	} 
	m_last_hit_frame = Device.dwFrame;
}


//////////////////////////////////////////////////////////////////////////
// Other
//////////////////////////////////////////////////////////////////////////

void CPoltergeist::PhysicalImpulse	(const Fvector &position)
{
	m_nearest.clear_not_free		();
	Level().ObjectSpace.GetNearest	(m_nearest,position, m_ability_impulse_radius, NULL); 
	//xr_vector<CObject*> &m_nearest = Level().ObjectSpace.q_nearest;
	if (m_nearest.empty())			return;
	
	u32 index = Random.randI		(m_nearest.size());
	
	CPhysicsShellHolder  *obj = smart_cast<CPhysicsShellHolder *>(m_nearest[index]);
	if (!obj || !obj->m_pPhysicsShell) return;

	Fvector dir;
	dir.sub(obj->Position(), position);
	dir.normalize();
	
	CPhysicsElement* E=obj->m_pPhysicsShell->get_ElementByStoreOrder(u16(Random.randI(obj->m_pPhysicsShell->get_ElementsNumber())));
	//E->applyImpulse(dir,IMPULSE * obj->m_pPhysicsShell->getMass());
	E->applyImpulse(dir, m_ability_impulse * E->getMass());
}

void CPoltergeist::StrangeSounds(const Fvector &position)
{
	if (m_strange_sound._feedback()) return;
	
	for (u32 i = 0; i < m_ability_trace_attempts; i++) 
	{
		Fvector dir;
		dir.random_dir();

		collide::rq_result	l_rq;
		if (Level().ObjectSpace.RayPick(position, dir, m_ability_trace_distance, collide::rqtStatic, l_rq, NULL)) 
		{
			if (l_rq.range < m_ability_trace_distance) 
			{
				// Получить пару материалов
				CDB::TRI*	pTri	= Level().ObjectSpace.GetStaticTris() + l_rq.element;
				SGameMtlPair* mtl_pair = GMLib.GetMaterialPair(material().self_material_idx(),pTri->material);
				if (!mtl_pair) continue;

				// Играть звук
				if (!mtl_pair->CollideSounds.empty()) 
				{
					CLONE_MTL_SOUND(m_strange_sound, mtl_pair, CollideSounds);
					Fvector pos;
					pos.mad(position, dir, ((l_rq.range - 0.1f > 0) ? l_rq.range - 0.1f  : l_rq.range));
					m_strange_sound.play_at_pos(this,pos);
					return;
				}			
			}
		}
	}
}
