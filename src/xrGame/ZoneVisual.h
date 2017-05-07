#pragma once
#include "..\Include\xrRender\animation_motion.h"
#include "MosquitoBald.h"

class CVisualZone : public CMosquitoBald
{
	typedef	CMosquitoBald inherited;
	MotionID			m_idle_animation;
	MotionID			m_attack_animation;
	bool				m_use_attack_animation;
	u32					m_dwAttackAnimaionStart;
	u32					m_dwAttackAnimaionEnd;

public:
	CVisualZone				()						;
	virtual			~CVisualZone					();
	virtual BOOL	net_Spawn						(CSE_Abstract* DC);
	virtual void	SwitchZoneState					(EZoneState new_state);
	virtual void	Load							(LPCSTR section);
	virtual void	UpdateBlowout					();
};
