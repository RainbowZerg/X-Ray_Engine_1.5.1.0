// DummyObject.h: interface for the CHangingLamp class.
//
//////////////////////////////////////////////////////////////////////

#ifndef HangingLampH
#define HangingLampH
#pragma once

#include "gameobject.h"
#include "physicsshellholder.h"
#include "PHSkeleton.h"
#include "script_export_space.h"
#include "hit_immunity.h"
#include "damage_manager.h"

// refs
class CLAItem;
class CPhysicsElement;
class CSE_ALifeObjectHangingLamp;
class CPHElement;

class CHangingLamp : public CPhysicsShellHolder, public CPHSkeleton, public CHitImmunity, public CDamageManager
{
	//need m_pPhysicShell
	typedef	CPhysicsShellHolder		inherited;
private:
	u16				light_bone;
	u16				ambient_bone;

	ref_light		light_render;
	ref_light		light_ambient;
	CLAItem*		lanim;
	float			ambient_power;
	
	ref_glow		glow_render;
	
	float			fHealth;
	float			fBrightness;

	// ZergO: added
	bool			enabled;
	bool			usable;
	bool			use_custom_bone_damage;
	bool			use_custom_immunities;
	std::vector<u16> bones_hide_enabled;
	std::vector<u16> bones_hide_disabled;
	std::vector<u16> bones_hide_dead;
	ref_sound		sound_enable;
	ref_sound		sound_disable;
	ref_sound		sound_idle;
	u16				particles_bone;
	CParticlesObject* particles_object_idle;
	//

	void			CreateBody		(CSE_ALifeObjectHangingLamp	*lamp);
	void			Init();
	void			RespawnInit		();
public:
	shared_str		enable_tip;
	shared_str		disable_tip;

					CHangingLamp	();
	virtual			~CHangingLamp	();
	virtual DLL_Pure* _construct	();

	bool			Alive			() const { return fHealth>0.f; }
	bool			Enabled			() const { return enabled; }
	bool			Usable			() const { return usable; }
	void			TurnOn			(bool sound = false);
	void			TurnOff			(bool sound = false);
	void			Switch			(bool enabled, bool sound = false);

	IRender_Light  *GetLight(int target = 0);

	void			SetAngle		(float angle, int target = 0);
	void			SetAnimation	(LPCSTR name);
	void			SetBrightness	(float brightness);
	void			SetDirection	(const Fvector &v, float bank);
	void			SetColor		(const Fcolor &color, int target = 0);
	void			SetColor		(float r, float g, float b, int target = 0);
		
	void			SetPosition		(const Fvector &v);
	void			SetRange	    (float range, int target = 0);
	void			SetTexture		(LPCSTR texture, int target = 0);
	void			SetVirtualSize	(float size, int target = 0);
	
	void			SetFlare		(bool b, int target = 0);

	void			SetVolumetric				(bool b,  int target = 0);
	void			SetVolumetricIntensity		(float f, int target = 0);
	void			SetVolumetricQuality		(float f, int target = 0);
	void			SetVolumetricDistance		(float f, int target = 0);
	void			Synchronize		(); // alpet: сохранение данных в серверный объект


	virtual void	Load			( LPCSTR section);
	virtual BOOL	net_Spawn		( CSE_Abstract* DC);
	virtual void	net_Destroy		();
	virtual void	shedule_Update	( u32 dt);							// Called by sheduler
	virtual void	UpdateCL		( );								// Called each frame, so no need for dt


	virtual void	SpawnInitPhysics	(CSE_Abstract	*D)																;
	virtual CPhysicsShellHolder*	PPhysicsShellHolder	()	{return PhysicsShellHolder();}								;
	virtual	void	CopySpawnInit		()																				;
	virtual void	net_Save			(NET_Packet& P)																	;
	virtual	BOOL	net_SaveRelevant	()																				;

	virtual BOOL	renderable_ShadowGenerate	( ) { return TRUE;	}
	virtual BOOL	renderable_ShadowReceive	( ) { return TRUE;	}
	
	virtual	void	Hit				(SHit* pHDS);
	virtual void	net_Export		(NET_Packet& P);
	virtual void	net_Import		(NET_Packet& P);
	virtual BOOL	UsedAI_Locations();

	virtual void	Center			(Fvector& C)	const;
	virtual float	Radius			()				const;
	DECLARE_SCRIPT_REGISTER_FUNCTION
};
add_to_type_list(CHangingLamp)
#undef script_type_list
#define script_type_list save_type_list(CHangingLamp)

#endif //HangingLampH
