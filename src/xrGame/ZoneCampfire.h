#pragma once
#include "..\Include\xrRender\animation_motion.h"
#include "MosquitoBald.h"
#include "script_export_space.h"

class CZoneCampfire : public CMosquitoBald
{
	typedef CMosquitoBald	inherited;

protected:
	CParticlesObject*		m_pEnablingParticles;
	CParticlesObject*		m_pDisabledParticles;
	ref_sound				m_disabled_sound;
	bool					m_turned_on;
	u32						m_turn_time;
	u16						m_light_bone;
	LPCSTR					m_visual_str;

		virtual	void		PlayIdleParticles			(bool bIdleLight=true);
		virtual	void		StopIdleParticles			(bool bIdleLight=true);
		virtual BOOL		AlwaysTheCrow				();
		virtual BOOL		net_Spawn					(CSE_Abstract* DC);
		virtual	void		UpdateWorkload				(u32 dt);
		virtual void		UpdateIdleLight				();

public:
							CZoneCampfire				();
	virtual					~CZoneCampfire				();
	virtual		void		Load						(LPCSTR section);
	virtual		void		GoEnabledState				();
	virtual		void		GoDisabledState				();

				void		Switch						(bool enabled);
				bool		Enabled						() const;
	IRender_Light*			GetLight					(int target) const;

	virtual		void		shedule_Update				(u32 dt);
	DECLARE_SCRIPT_REGISTER_FUNCTION
};
add_to_type_list(CZoneCampfire)
#undef script_type_list
#define script_type_list save_type_list(CZoneCampfire)