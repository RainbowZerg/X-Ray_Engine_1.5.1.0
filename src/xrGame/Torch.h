#pragma once

#include "inventory_item_object.h"
//#include "night_vision_effector.h"
#include "hudsound.h"
#include "script_export_space.h"

class CLAItem;

class CTorch : public CInventoryItemObject {
private:
    typedef	CInventoryItemObject	inherited;

protected:
	float			fBrightness;
	CLAItem*		lanim;
	CLAItem*		lanim_flickering;

	u16				guid_bone;
	shared_str		light_trace_bone;

	float			m_delta_h;
	Fvector2		m_prev_hp;
	bool			m_switched_on;
	ref_light		light_render;
	ref_light		light_omni;
	ref_glow		glow_render;
private:
	// ZergO: added
	bool			m_is_flickering;
	bool			m_is_broken;
	bool			m_light_is_volumetric;

	float			m_light_range;
	float			m_light_volumetric_intensity;

	Fvector			m_light_offset;
	Fvector			m_light_omni_offset;
	Fcolor			m_light_color;
	//

	inline	bool	can_use_dynamic_lights	();

public:
					CTorch				(void);
	virtual			~CTorch				(void);

	virtual void	Load				(LPCSTR section);
	virtual BOOL	net_Spawn			(CSE_Abstract* DC);
	virtual void	net_Destroy			();
	virtual void	net_Export			(NET_Packet& P);				// export to server
	virtual void	net_Import			(NET_Packet& P);				// import from server

	virtual void	OnH_A_Chield		();
	virtual void	OnH_B_Independent	(bool just_before_destroy);

	virtual void	UpdateCL			();

			bool	Enabled				()				const;
			bool	Broken				(bool fatal)	const;

			void	Switch				();
			void	Switch				(bool light_on);
			void	Break				(bool fatal);

	IRender_Light  *GetLight(int target = 0);

	void			SetAngle			(float angle, int target = 0);
	void			SetAnimation		(LPCSTR name);
	void			SetBrightness		(float brightness);
	void			SetDirection		(const Fvector &v, float bank);
	void			SetColor			(const Fcolor &color, int target = 0);
	void			SetRGB				(float r, float g, float b, int target = 0);
		
	void			SetPosition			(const Fvector &v);
	void			SetRange			(float range, int target = 0);
	void			SetTexture			(LPCSTR texture, int target = 0);
	void			SetVirtualSize		(float size, int target = 0);
	
	void			SetFlare			(bool b, int target = 0);

	void			SetVolumetric				(bool b, int target = 0);
	void			SetVolumetricIntensity		(float f, int target = 0);
	void			SetVolumetricQuality		(float f, int target = 0);
	void			SetVolumetricDistance		(float f, int target = 0);

	virtual bool	can_be_attached		() const;

	//CAttachableItem
	virtual	void				enable					(bool value);

protected:
	HUD_SOUND_ITEM m_FlashlightSwitchSnd;

	enum EStats
	{
		eActive		= (1<<0),
		eAttached	= (1<<1),
		eFlickering	= (1<<2), // ZergO: added
	};

public:

	virtual bool			use_parent_ai_locations	() const
	{
		return				(!H_Parent());
	}
	virtual void	create_physic_shell		();
	virtual void	activate_physic_shell	();
	virtual void	setup_physic_shell		();

	virtual void	afterDetach				();
	virtual void	renderable_Render		();

	DECLARE_SCRIPT_REGISTER_FUNCTION
};
add_to_type_list(CTorch)
#undef script_type_list
#define script_type_list save_type_list(CTorch)
