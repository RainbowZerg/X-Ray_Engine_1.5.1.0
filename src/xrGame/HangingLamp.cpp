#include "pch_script.h"
#include "HangingLamp.h"
#include "../xrEngine/LightAnimLibrary.h"
#include "../xrEngine/xr_collide_form.h"
#include "PhysicsShell.h"
#include "Physics.h"
#include "xrserver_objects_alife.h"
#include "PHElement.h"
#include "../Include/xrRender/Kinematics.h"
#include "../Include/xrRender/KinematicsAnimated.h"
#include "game_object_space.h"
#include "script_callback_ex.h"
#include "script_game_object.h"
#include "ai_sounds.h"
//////////////////////////////////////////////////////////////////////
// Construction/Destruction
//////////////////////////////////////////////////////////////////////
CHangingLamp::CHangingLamp	()
{
	Init();
}

CHangingLamp::~CHangingLamp	()
{
}

DLL_Pure *CHangingLamp::_construct()
{
	CDamageManager::_construct();
	return inherited::_construct();
}

void CHangingLamp::Init()
{
	fHealth			= 100.f;
	light_bone		= BI_NONE;
	ambient_bone	= BI_NONE;
	lanim			= 0;
	ambient_power	= 0.f;
	light_render	= 0;
	light_ambient	= 0;
	glow_render		= 0;

	enabled			= false;
	usable			= false;

	enable_tip		= "hanging_lamp_enable";
	disable_tip		= "hanging_lamp_disable";

	use_custom_bone_damage = false;
	use_custom_immunities = false;

	particles_object_idle = NULL;
}

void CHangingLamp::RespawnInit()
{
	Init();
	if (Visual())
	{
		IKinematics* K = smart_cast<IKinematics*>(Visual());
		K->LL_SetBonesVisible(u64(-1));
		K->CalculateBones_Invalidate();
		K->CalculateBones	(TRUE);
	}
}

void CHangingLamp::Center	(Fvector& C) const 
{ 
	if (renderable.visual)
		renderable.xform.transform_tiny(C,renderable.visual->getVisData().sphere.P);	
	else
		C.set(XFORM().c);
}

float CHangingLamp::Radius	() const 
{ 
	return (renderable.visual)?renderable.visual->getVisData().sphere.R:EPS;
}

void CHangingLamp::Load		(LPCSTR section)
{
	inherited::Load			(section);
//	LPCSTR imm_sect = READ_IF_EXISTS (pSettings, r_string, section, "immunities_sect", NULL);
//	if (imm_sect)
//		CHitImmunity::LoadImmunities(imm_sect, pSettings);
}

void CHangingLamp::net_Destroy()
{
	light_render.destroy	();
	light_ambient.destroy	();
	glow_render.destroy		();
	RespawnInit				();
	if (Visual())
		CPHSkeleton::RespawnInit();

	inherited::net_Destroy	();
}

BOOL CHangingLamp::net_Spawn(CSE_Abstract* DC)
{
	CSE_Abstract			*e		= (CSE_Abstract*)(DC);
	CSE_ALifeObjectHangingLamp	*lamp	= smart_cast<CSE_ALifeObjectHangingLamp*>(e);
	R_ASSERT				(lamp);
	inherited::net_Spawn	(DC);
	Fcolor					clr;

	// set bone id
//	CInifile* pUserData		= K->LL_UserData(); 
//	R_ASSERT3				(pUserData,"Empty HangingLamp user data!",lamp->get_visual());
	xr_delete(collidable.model);
	if (Visual())
	{
		IKinematics* K		= smart_cast<IKinematics*>(Visual());
		R_ASSERT			(Visual()&&smart_cast<IKinematics*>(Visual()));
		light_bone			= K->LL_BoneID	(*lamp->light_main_bone);	VERIFY(light_bone!=BI_NONE);
		ambient_bone		= K->LL_BoneID	(*lamp->light_ambient_bone);VERIFY(ambient_bone!=BI_NONE);
		collidable.model	= xr_new<CCF_Skeleton>				(this);
		
		CInifile* ini = K->LL_UserData();
		if (ini)
		{
			// alpet: загрузка иммунитетов из спавн-конфига
			if (ini->section_exist("immunities"))
			{
				CHitImmunity::LoadImmunities("immunities", ini);
				use_custom_immunities = true;
			}

			// ZergO: юзабельность лампы, переопределение скрываемых костей (on/off/dead), звуки (on/off/idle), текстовые подсказки (on/off), настройки костей
			LPCSTR params_sec = "params";
			if (ini->section_exist(params_sec))
			{
				if (ini->line_exist(params_sec, "bones_hide_disabled"))
				{ 
					LPCSTR bones = ini->r_string(params_sec, "bones_hide_disabled");
					int count = _GetItemCount(bones);
					for (int i = 0; i < count; i++)
					{
						string64 bone;
						_GetItem(bones, i, bone);
						u16 bone_id = K->LL_BoneID(bone);
						R_ASSERT2(BI_NONE != bone_id, "wrong disabled-hide bone");
						bones_hide_disabled.push_back(bone_id);
					}
				}

				if (ini->line_exist(params_sec, "bones_hide_enabled"))
				{ 
					LPCSTR bones = ini->r_string(params_sec, "bones_hide_enabled");
					int count = _GetItemCount(bones);
					for (int i = 0; i < count; i++)
					{
						string64 bone;
						_GetItem(bones, i, bone);
						u16 bone_id = K->LL_BoneID(bone);
						R_ASSERT2(BI_NONE != bone_id, "wrong enabled-hide bone");
						bones_hide_enabled.push_back(bone_id);
					}
				}

				if (ini->line_exist(params_sec, "bones_hide_dead"))
				{ 
					LPCSTR bones = ini->r_string(params_sec, "bones_hide_dead");
					int count = _GetItemCount(bones);
					for (int i = 0; i < count; i++)
					{
						string64 bone;
						_GetItem(bones, i, bone);
						u16 bone_id = K->LL_BoneID(bone);
						R_ASSERT2(BI_NONE != bone_id, "wrong dead-hide bone");
						bones_hide_dead.push_back(bone_id);
					}
				}

				if (ini->line_exist(params_sec, "usable"))
					usable = !!ini->r_bool(params_sec, "usable");

				if (ini->line_exist(params_sec, "sound_disable"))
					sound_disable.create(ini->r_string(params_sec, "sound_disable"), st_Effect, SOUND_TYPE_USING);

				if (ini->line_exist(params_sec, "sound_enable"))
					sound_enable.create(ini->r_string(params_sec, "sound_enable"), st_Effect, SOUND_TYPE_USING);

				if (ini->line_exist(params_sec, "sound_idle"))
					sound_idle.create(ini->r_string(params_sec, "sound_idle"), st_Effect, SOUND_TYPE_USING);

				if (ini->line_exist(params_sec, "enable_tip"))
					enable_tip = ini->r_string(params_sec, "enable_tip");

				if (ini->line_exist(params_sec, "disable_tip"))
					disable_tip = ini->r_string(params_sec, "disable_tip");

				if (ini->line_exist(params_sec, "damage"))
				{
					CDamageManager::reload(params_sec, "damage", ini);
					use_custom_bone_damage = true;
				}

				if (ini->line_exist(params_sec, "particles_bone"))
				{
					particles_bone = K->LL_BoneID(ini->r_string(params_sec, "particles_bone"));
					LPCSTR particles_idle = ini->r_string(params_sec, "particles_idle");
					particles_object_idle = CParticlesObject::Create(particles_idle, FALSE);
				}
			}
		}
	}
	fBrightness				= lamp->brightness;
	clr.set					(lamp->color);						clr.a = 1.f;
	clr.mul_rgb				(fBrightness);

	light_render			= ::Render->light_create();
	light_render->set_shadow(!!lamp->flags.is(CSE_ALifeObjectHangingLamp::flCastShadow));
	light_render->set_volumetric(!!lamp->flags.is(CSE_ALifeObjectHangingLamp::flVolumetric));
	light_render->set_type	(lamp->flags.is(CSE_ALifeObjectHangingLamp::flTypeSpot)?IRender_Light::SPOT:IRender_Light::POINT);
	light_render->set_range	(lamp->range);
	light_render->set_color	(clr);
	light_render->set_cone	(lamp->spot_cone_angle);
	light_render->set_texture(*lamp->light_texture);
	light_render->set_virtual_size(lamp->m_virtual_size);

	light_render->set_volumetric_quality(lamp->m_volumetric_quality);
	light_render->set_volumetric_intensity(lamp->m_volumetric_intensity);
	light_render->set_volumetric_distance(lamp->m_volumetric_distance);	

	if (lamp->glow_texture.size())	
	{
		glow_render				= ::Render->glow_create();
		glow_render->set_texture(*lamp->glow_texture);
		glow_render->set_color	(clr);
		glow_render->set_radius	(lamp->glow_radius);
	}

	if (lamp->flags.is(CSE_ALifeObjectHangingLamp::flPointAmbient))
	{
		ambient_power			= lamp->m_ambient_power;
		light_ambient			= ::Render->light_create();
		light_ambient->set_type	(IRender_Light::POINT);
		light_ambient->set_shadow(false);
		clr.mul_rgb				(ambient_power);
		light_ambient->set_range(lamp->m_ambient_radius);
		light_ambient->set_color(clr);
		light_ambient->set_texture(*lamp->m_ambient_texture);
	}

	fHealth					= lamp->m_health;

	lanim					= LALib.FindItem(*lamp->color_animator);

	CPHSkeleton::Spawn(e);

	IKinematicsAnimated* pKA = smart_cast<IKinematicsAnimated*>(Visual());
	if (pKA)
		pKA->PlayCycle("idle");

	IKinematics* pK = smart_cast<IKinematics*>(Visual());
	if (pK)
	{
		pK->CalculateBones_Invalidate();
		pK->CalculateBones(TRUE);
		//.intepolate_pos
	}

	if (lamp->flags.is(CSE_ALifeObjectHangingLamp::flPhysic) && !Visual())
		Msg("! WARNING: lamp, obj name [%s],flag physics set, but has no visual",*cName());
//.	if (lamp->flags.is(CSE_ALifeObjectHangingLamp::flPhysic)&&Visual()&&!guid_physic_bone)	fHealth=0.f;

	if (Alive())			
		TurnOn	();
	else
	{
		processing_activate		();	// temporal enable
		TurnOff					();	// -> and here is disable :)
	}
	
	setVisible					((BOOL)!!Visual());
	setEnabled					((BOOL)!!collidable.model);

	return						(TRUE);
}


void CHangingLamp::SpawnInitPhysics	(CSE_Abstract	*D)	
{
	CSE_ALifeObjectHangingLamp *lamp = smart_cast<CSE_ALifeObjectHangingLamp*>(D);	
	if (lamp->flags.is(CSE_ALifeObjectHangingLamp::flPhysic))		
		CreateBody(lamp);

	IKinematics* pK = smart_cast<IKinematics*>(Visual());
	if (pK)
	{
		pK->CalculateBones_Invalidate();
		pK->CalculateBones(TRUE);
		//.intepolate_pos
	}
}

void CHangingLamp::CopySpawnInit		()
{
	CPHSkeleton::CopySpawnInit();
	IKinematics* K = smart_cast<IKinematics*>(Visual());
	if (!K->LL_GetBoneVisible(light_bone))
		TurnOff();
}
void	CHangingLamp::net_Save			(NET_Packet& P)	
{
	inherited::net_Save(P);
	CPHSkeleton::SaveNetState(P);
}

BOOL	CHangingLamp::net_SaveRelevant	()
{
	return (inherited::net_SaveRelevant() || BOOL(PPhysicsShell()!=NULL));
}

void CHangingLamp::shedule_Update	(u32 dt)
{
	CPHSkeleton::Update(dt);


	inherited::shedule_Update		(dt);
}

void CHangingLamp::UpdateCL	()
{
	inherited::UpdateCL		();

	if (m_pPhysicsShell)
		m_pPhysicsShell->InterpolateGlobalTransform(&XFORM());

	if (!enabled) return;

	if (Alive())
	{
		if (Visual())	
			PKinematics(Visual())->CalculateBones();

		// update T&R from light (main) bone
		Fmatrix xf;
		if (light_bone != BI_NONE)
		{
			Fmatrix& M	= smart_cast<IKinematics*>(Visual())->LL_GetTransform(light_bone);
			xf.mul		(XFORM(),M);
			VERIFY		(!fis_zero(DET(xf)));
		}
		else
			xf.set		(XFORM());

		light_render->set_rotation	(xf.k, xf.i);
		light_render->set_position	(xf.c);
		if (glow_render)
			glow_render->set_position(xf.c);

		// update T&R from ambient bone
		if (light_ambient)
		{	
			if (ambient_bone != light_bone)
			{
				if (ambient_bone != BI_NONE)
				{
					Fmatrix& M	= smart_cast<IKinematics*>(Visual())->LL_GetTransform(ambient_bone);
					xf.mul		(XFORM(), M);
					VERIFY		(!fis_zero(DET(xf)));
				}
				else
				{
					xf.set		(XFORM());
				}
			}
			light_ambient->set_rotation	(xf.k,xf.i);
			light_ambient->set_position	(xf.c);
		}
		
		if (lanim)
		{
			int frame;
			u32 clr					= lanim->CalculateBGR(Device.fTimeGlobal,frame); // возвращает в формате BGR
			Fcolor					fclr;
			fclr.set				((float)color_get_B(clr),(float)color_get_G(clr),(float)color_get_R(clr),1.f);
			fclr.mul_rgb			(fBrightness/255.f);
			light_render->set_color	(fclr);
			if (glow_render)		
				glow_render->set_color(fclr);

			if (light_ambient) 
			{
				fclr.mul_rgb		(ambient_power);
				light_ambient->set_color(fclr);
			}
		}

		if (sound_idle._p != NULL)
		{
			if (!sound_idle._feedback())
				sound_idle.play_at_pos(this, this->Position());
			else
				sound_idle.set_position(this->Position());
		}
	}
}

void CHangingLamp::Switch(bool enabled, bool sound)
{
	enabled ? TurnOn(sound) : TurnOff(sound);
}

void CHangingLamp::TurnOn (bool sound)
{
	if (enabled || !Alive()) return;

	light_render->set_active						(true);
	if (glow_render)	glow_render->set_active		(true);
	if (light_ambient)	light_ambient->set_active	(true);
	if (Visual())
	{
		IKinematics* K				= smart_cast<IKinematics*>(Visual());
		if (bones_hide_disabled.empty() && bones_hide_enabled.empty())
			K->LL_SetBoneVisible		(light_bone, TRUE, TRUE);
		else
		{
			if (!bones_hide_disabled.empty())
			{
				for (u32 i = 0; i < bones_hide_disabled.size(); ++i)
					K->LL_SetBoneVisible(bones_hide_disabled[i], TRUE, FALSE);
			}

			if (!bones_hide_enabled.empty())
			{
				for (u32 i = 0; i < bones_hide_enabled.size(); ++i)
					K->LL_SetBoneVisible(bones_hide_enabled[i], FALSE, FALSE);
			}
		}

		K->CalculateBones_Invalidate();
		K->CalculateBones			(TRUE);
//		K->LL_SetBoneVisible		(light_bone, TRUE, TRUE); //hack

		if (sound && (sound_enable._p != NULL))
			sound_enable.play_at_pos(this, this->Position());

		if (particles_object_idle && !particles_object_idle->IsPlaying())
		{
			Fmatrix bone_xform = K->LL_GetTransform(particles_bone);
			bone_xform.mulA_43(XFORM());
			particles_object_idle->SetXFORM(bone_xform);
			particles_object_idle->Play(false);
		}
	}
	processing_activate		();

	enabled = true;
}

void CHangingLamp::TurnOff	(bool sound)
{
	if (!enabled && Alive()) return;

	light_render->set_active						(false);
	if (glow_render)	glow_render->set_active		(false);
	if (light_ambient)	light_ambient->set_active	(false);
	if (Visual())		
	{
		IKinematics *K = smart_cast<IKinematics*>(Visual());
		VERIFY(K);

		if (bones_hide_enabled.empty() && bones_hide_disabled.empty() && bones_hide_dead.empty())
			K->LL_SetBoneVisible(light_bone, FALSE, TRUE);
		else
		{
			if (Alive())
			{
				if (!bones_hide_enabled.empty())
				{
					for (u32 i = 0; i < bones_hide_enabled.size(); ++i)
						K->LL_SetBoneVisible(bones_hide_enabled[i], TRUE, FALSE);
				}

				if (!bones_hide_disabled.empty())
				{
					for (u32 i = 0; i < bones_hide_disabled.size(); ++i)
						K->LL_SetBoneVisible(bones_hide_disabled[i], FALSE, FALSE);
				}
			}
			else
			{
				if (!bones_hide_dead.empty())
				{
					for (u32 i = 0; i < bones_hide_dead.size(); ++i)
						K->LL_SetBoneVisible(bones_hide_dead[i], FALSE, TRUE);
				}
			}
		}

		VERIFY2( K->LL_GetBonesVisible() != 0, make_string("can not Turn Off lamp: %s, visual %s - because all bones become invisible", cNameVisual().c_str(), cName().c_str() ));

		if (sound_idle._p != NULL)
			sound_idle.stop();

		if (sound && (sound_disable._p != NULL))
			sound_disable.play_at_pos(this, this->Position());

		if (particles_object_idle)
			particles_object_idle->Stop(FALSE);
	}

	processing_deactivate();

	enabled = false;
}

//void CHangingLamp::Hit(float P,Fvector &dir, CObject* who,s16 element,
//					   Fvector p_in_object_space, float impulse, ALife::EHitType hit_type)
void CHangingLamp::Hit (SHit* pHDS)
{
	SHit HDS = *pHDS;

	if (use_custom_immunities)
		HDS.power = CHitImmunity::AffectHit(HDS.power,HDS.hit_type);

	if (use_custom_bone_damage)
	{
		float hitScale = 1.f, woundScale = 1.f;
		CDamageManager::HitScale(HDS.bone(), hitScale, woundScale);
		HDS.power *= hitScale;
	}

	inherited::Hit(&HDS); // physics shell only
	callback(GameObject::eHit)(lua_game_object(), HDS.power, HDS.dir, smart_cast<const CGameObject*>(HDS.who)->lua_game_object(), HDS.bone());

//	if (m_pPhysicsShell) 
//		m_pPhysicsShell->applyHit(pHDS->p_in_bone_space,pHDS->dir,pHDS->impulse,pHDS->boneID,pHDS->hit_type);

	bool bWasAlive = Alive();

	if (use_custom_bone_damage || use_custom_immunities)
	{ 
		fHealth -= HDS.damage()*100.f; // у ламп макс. здоровье - 100.f, а не 1.f как у всего остального
	}
	else
	{
		// пуля гасит только при попадании в bone_light, а взрыв работает как раньше
		if (pHDS->boneID == light_bone)
			fHealth = 0.f;
		else if (pHDS->hit_type == ALife::eHitTypeExplosion)
			fHealth -= pHDS->damage()*100.f;
	}

	if (bWasAlive && !Alive())
		TurnOff();
}

static BONE_P_MAP bone_map = BONE_P_MAP();
void CHangingLamp::CreateBody(CSE_ALifeObjectHangingLamp *lamp)
{
	if (!Visual())			return;
	if (m_pPhysicsShell)	return;
	
	IKinematics* pKinematics = smart_cast<IKinematics*>(Visual());

	m_pPhysicsShell			= P_create_Shell();

	bone_map.clear();
	LPCSTR fixed_bones = *lamp->fixed_bones;
	if (fixed_bones)
	{
		int count =	_GetItemCount(fixed_bones);
		for (int i = 0; i < count; ++i)
		{
			string64 fixed_bone;
			_GetItem				(fixed_bones, i, fixed_bone);
			u16 fixed_bone_id		= pKinematics->LL_BoneID(fixed_bone);
			R_ASSERT2				(BI_NONE != fixed_bone_id, "wrong fixed bone");
			bone_map.insert			(mk_pair(fixed_bone_id, physicsBone()));
		}
	}
	else
	{
		bone_map.insert(mk_pair(pKinematics->LL_GetBoneRoot(),physicsBone()));
	}

	phys_shell_verify_object_model			(*this);
	
	m_pPhysicsShell->build_FromKinematics	(pKinematics,&bone_map);
	m_pPhysicsShell->set_PhysicsRefObject	(this);
	m_pPhysicsShell->mXFORM.set				(XFORM());
	m_pPhysicsShell->Activate				(true);//,
	//m_pPhysicsShell->SmoothElementsInertia(0.3f);
	m_pPhysicsShell->SetAirResistance		();//0.0014f,1.5f

/////////////////////////////////////////////////////////////////////////////
	BONE_P_PAIR_IT i = bone_map.begin(), e = bone_map.end();
	for(; i != e; i++)
	{
		CPhysicsElement* fixed_element = i->second.element;
		///R_ASSERT2(fixed_element,"fixed bone has no physics");
		if (fixed_element)
			fixed_element->Fix();
	}

	m_pPhysicsShell->mXFORM.set			(XFORM());
	m_pPhysicsShell->SetAirResistance	(0.001f, 0.02f);
	SAllDDOParams disable_params;
	disable_params.Load					(smart_cast<IKinematics*>(Visual())->LL_UserData());
	m_pPhysicsShell->set_DisableParams	(disable_params);
	ApplySpawnIniToPhysicShell			(&lamp->spawn_ini(),m_pPhysicsShell,fixed_bones[0]!='\0');
}

void CHangingLamp::net_Export(NET_Packet& P)
{
	VERIFY					(Local());
}

void CHangingLamp::net_Import(NET_Packet& P)
{
	VERIFY					(Remote());
}

BOOL CHangingLamp::UsedAI_Locations()
{
	return					(FALSE);
}

CSE_ALifeObjectHangingLamp *get_se_lamp(CHangingLamp *lobj)
{
	CSE_ALifeDynamicObject *e = lobj->lua_game_object()->alife_object();
	if (!e) return NULL;
	return smart_cast<CSE_ALifeObjectHangingLamp*>(e);
}

void CHangingLamp::SetDirection(const Fvector &v, float bank)
{
	lua_game_object()->SetDirection(v, bank);		
}
void CHangingLamp::SetPosition(const Fvector &v) 
{
	lua_game_object()->SetPosition(v);	
}

void CHangingLamp::Synchronize() // alpet: сохранение данных в серверный объект
{
	CSE_ALifeObjectHangingLamp *lamp = get_se_lamp(this);
	if (!lamp) return;
	lamp->position()		= XFORM().c;	
	XFORM().getXYZ			(lamp->angle());	 
	lamp->brightness		= fBrightness;
	lamp->spot_cone_angle	= light_render->get_cone();
	lamp->range				= light_render->get_range();
	Fcolor clr				= light_render->get_color();
	if (fBrightness > 0)
		clr.mul_rgb			(255.f / fBrightness);

	lamp->color				= clr.get();
	lamp->m_virtual_size	= light_render->get_virtual_size();
	lamp->flare				= light_render->get_flare();
	lamp->m_volumetric_distance		= light_render->get_volumetric_distance();
	lamp->m_volumetric_intensity	= light_render->get_volumetric_intensity();
	lamp->m_volumetric_quality		= light_render->get_volumetric_quality();

	if (light_ambient)
	{
		//lamp->m_ambient_power
		//lamp->m_ambient_texture
		lamp->m_ambient_radius = light_ambient->get_range();
	}

	if (lanim)
		lamp->color_animator = lanim->cName;
}

#define  CLASS_IMPL		CHangingLamp
#define  target_0		light_render
#define  target_1		light_ambient
#define	 target_2		glow_render		
#include "script_light_ext.h"
#undef   target_0
#undef	 target_1
#undef	 target_2 
#undef CLASS_IMPL

#pragma optimize("s",on)
void CHangingLamp::script_register(lua_State *L)
{
	luabind::module(L)
	[
		luabind::class_<CHangingLamp,CGameObject>("hanging_lamp")
			.def(luabind::constructor<>())
			.def("on",				&CHangingLamp::Enabled)
			.def("switch",			&CHangingLamp::Switch)	
			.def("get_light",		&CHangingLamp::GetLight) 
			.def("synchronize",		&CHangingLamp::Synchronize)

			.def("set_animation",	&CHangingLamp::SetAnimation)
			.def("set_brightness",	&CHangingLamp::SetBrightness)
			.def("set_direction",   &CHangingLamp::SetDirection)   
			.def("set_position",    &CHangingLamp::SetPosition)  
			.def("set_angle",		&CHangingLamp::SetAngle)
			.def("set_color",		(void (CHangingLamp::*)(const Fcolor&, int)) &CHangingLamp::SetColor)
			.def("set_color",		(void (CHangingLamp::*)(float, float, float, int)) &CHangingLamp::SetColor)
			.def("set_range",		&CHangingLamp::SetRange)
			.def("set_texture",		&CHangingLamp::SetTexture)
			.def("set_virtual_size",&CHangingLamp::SetVirtualSize)

			.def("set_flare",		&CHangingLamp::SetFlare)
			.def("set_volumetric",			&CHangingLamp::SetVolumetric)
			.def("set_volumetric_intensity",&CHangingLamp::SetVolumetricIntensity)
			.def("set_volumetric_quality",	&CHangingLamp::SetVolumetricQuality)
			.def("set_volumetric_distance", &CHangingLamp::SetVolumetricDistance)
			,
		luabind::class_<IRender_Light>("IRender_Light")
			.def("get_active",				&IRender_Light::get_active)
			.def("get_angle",				&IRender_Light::get_cone)			
			.def("get_color",				&IRender_Light::get_color)
			.def("get_range",				&IRender_Light::get_range)
			.def("get_virtual_size",		&IRender_Light::get_virtual_size)
			.def("get_flare",				&IRender_Light::get_flare)
			.def("get_volumetric",			&IRender_Light::get_volumetric)
			.def("get_volumetric_intensity",&IRender_Light::get_volumetric_intensity)
			.def("get_volumetric_quality",	&IRender_Light::get_volumetric_quality)
			.def("get_volumetric_distance", &IRender_Light::get_volumetric_distance)

			.def("set_active",				&IRender_Light::set_active)
			.def("set_angle",				&IRender_Light::set_cone)
			.def("set_color",				(void (IRender_Light::*)(const Fcolor&)) (&IRender_Light::set_color))
			.def("set_color",				(void (IRender_Light::*)(float, float, float)) (&IRender_Light::set_color))
			.def("set_range",				&IRender_Light::set_range)
			.def("set_texture",				&IRender_Light::set_texture)
			.def("set_virtual_size",		&IRender_Light::set_virtual_size)

			.def("set_flare",				&IRender_Light::set_flare)

			.def("set_volumetric",			&IRender_Light::set_volumetric)
			.def("set_volumetric_intensity",&IRender_Light::set_volumetric_intensity)
			.def("set_volumetric_quality",	&IRender_Light::set_volumetric_quality)
			.def("set_volumetric_distance", &IRender_Light::set_volumetric_distance)
	];
}
