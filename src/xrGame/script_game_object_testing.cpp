// script_game_object_testing.cpp
// Функции для тестирования объектов по их классу

#include "StdAfx.h"
#include "pch_script.h"
#include "ai_space.h"
#include "alife_simulator.h"
#include "script_game_object.h"
#include "script_game_object_impl.h"

#include "Actor.h"
#include "Antirad.h"
#include "Artefact.h"
#include "BottleItem.h"
#include "Car.h"
#include "CustomOutfit.h"
#include "CustomZone.h"
#include "Grenade.h"
#include "GrenadeLauncher.h"
#include "HangingLamp.h" 
#include "Helicopter.h"
#include "InventoryBox.h"
#include "Medkit.h"
#include "Scope.h"
#include "Searchlight.h"
#include "Silencer.h"
#include "Torch.h"
#include "WeaponAmmo.h"
#include "WeaponMagazinedWGrenade.h"
#include "ai\monsters\basemonster\base_monster.h"
#include "ai\trader\ai_trader.h"
#include "ai\stalker\ai_stalker.h"
#include "script_zone.h"

#define TEST_OBJECT_CLASS(A,B)								\
bool A () const												\
{															\
	B				*l_tpEntity = smart_cast<B*>(&object());\
	if (!l_tpEntity)										\
		return false;										\
															\
	return true;											\
};															\

TEST_OBJECT_CLASS(CScriptGameObject::IsActor,				CActor)
TEST_OBJECT_CLASS(CScriptGameObject::IsAmmo,				CWeaponAmmo)
TEST_OBJECT_CLASS(CScriptGameObject::IsAnomaly,				CCustomZone)
TEST_OBJECT_CLASS(CScriptGameObject::IsAntirad,				CAntirad)
TEST_OBJECT_CLASS(CScriptGameObject::IsArtefact,			CArtefact)
TEST_OBJECT_CLASS(CScriptGameObject::IsBottleItem,			CBottleItem)
TEST_OBJECT_CLASS(CScriptGameObject::IsCar,					CCar)
TEST_OBJECT_CLASS(CScriptGameObject::IsCustomMonster,		CCustomMonster)
TEST_OBJECT_CLASS(CScriptGameObject::IsCustomOutfit,		CCustomOutfit)
TEST_OBJECT_CLASS(CScriptGameObject::IsEatableItem,			CEatableItem)
TEST_OBJECT_CLASS(CScriptGameObject::IsEntityAlive,			CEntityAlive)
TEST_OBJECT_CLASS(CScriptGameObject::IsExplosive,			CExplosive)
TEST_OBJECT_CLASS(CScriptGameObject::IsFoodItem,			CFoodItem)
TEST_OBJECT_CLASS(CScriptGameObject::IsGameObject,			CGameObject)
TEST_OBJECT_CLASS(CScriptGameObject::IsGrenade,				CGrenade)
TEST_OBJECT_CLASS(CScriptGameObject::IsGrenadeLauncher,		CGrenadeLauncher)
TEST_OBJECT_CLASS(CScriptGameObject::IsHeli,				CHelicopter)
TEST_OBJECT_CLASS(CScriptGameObject::IsHolderCustom,		CHolderCustom)
TEST_OBJECT_CLASS(CScriptGameObject::IsHangingLamp,			CHangingLamp)
TEST_OBJECT_CLASS(CScriptGameObject::IsHudItem,				CHudItem)
TEST_OBJECT_CLASS(CScriptGameObject::IsInventoryBox,		CInventoryBox)
TEST_OBJECT_CLASS(CScriptGameObject::IsInventoryItem,		CInventoryItem)
TEST_OBJECT_CLASS(CScriptGameObject::IsInventoryOwner,		CInventoryOwner)
TEST_OBJECT_CLASS(CScriptGameObject::IsMedkit,				CMedkit)
TEST_OBJECT_CLASS(CScriptGameObject::IsMissile,				CMissile)
TEST_OBJECT_CLASS(CScriptGameObject::IsMonster,				CBaseMonster)
TEST_OBJECT_CLASS(CScriptGameObject::IsPhysicsShellHolder,	CPhysicsShellHolder)
TEST_OBJECT_CLASS(CScriptGameObject::IsProjector,			CProjector)
TEST_OBJECT_CLASS(CScriptGameObject::IsScope,				CScope)
TEST_OBJECT_CLASS(CScriptGameObject::IsScriptZone,			CScriptZone)
TEST_OBJECT_CLASS(CScriptGameObject::IsSilencer,			CSilencer)
TEST_OBJECT_CLASS(CScriptGameObject::IsSpaceRestrictor,		CSpaceRestrictor)
TEST_OBJECT_CLASS(CScriptGameObject::IsStalker,				CAI_Stalker)
TEST_OBJECT_CLASS(CScriptGameObject::IsTorch,				CTorch)
TEST_OBJECT_CLASS(CScriptGameObject::IsTrader,				CAI_Trader)
TEST_OBJECT_CLASS(CScriptGameObject::IsWeapon,				CWeapon)
TEST_OBJECT_CLASS(CScriptGameObject::IsWeaponGL,			CWeaponMagazinedWGrenade)
TEST_OBJECT_CLASS(CScriptGameObject::IsWeaponMagazined,		CWeaponMagazined)

using namespace luabind;

class_<CScriptGameObject> &script_register_game_object_testing(class_<CScriptGameObject> &instance)
{
	instance
		.def("is_actor",					&CScriptGameObject::IsActor)
		.def("is_ammo",						&CScriptGameObject::IsAmmo)
		.def("is_anomaly",					&CScriptGameObject::IsAnomaly)
		.def("is_antirad",					&CScriptGameObject::IsAntirad)
		.def("is_artefact",					&CScriptGameObject::IsArtefact)
		.def("is_bottle_item",				&CScriptGameObject::IsBottleItem)
		.def("is_car",						&CScriptGameObject::IsCar)
		.def("is_custom_monster",			&CScriptGameObject::IsCustomMonster)
		.def("is_eatable_item",				&CScriptGameObject::IsEatableItem)
		.def("is_entity_alive",				&CScriptGameObject::IsEntityAlive)
		.def("is_explosive",				&CScriptGameObject::IsExplosive)
		.def("is_food_item",				&CScriptGameObject::IsFoodItem)
		.def("is_game_object",				&CScriptGameObject::IsGameObject)
		.def("is_grenade",					&CScriptGameObject::IsGrenade)
		.def("is_grenade_launcher",			&CScriptGameObject::IsGrenadeLauncher)
		.def("is_helicopter",				&CScriptGameObject::IsHeli)
		.def("is_holder",					&CScriptGameObject::IsHolderCustom)
		.def("is_hud_item",					&CScriptGameObject::IsHudItem)
		.def("is_inventory_box",			&CScriptGameObject::IsInventoryBox)
		.def("is_inventory_item",			&CScriptGameObject::IsInventoryItem)
		.def("is_inventory_owner",			&CScriptGameObject::IsInventoryOwner)
		.def("is_medkit",					&CScriptGameObject::IsMedkit)
		.def("is_missile",					&CScriptGameObject::IsMissile)
		.def("is_monster",					&CScriptGameObject::IsMonster)
		.def("is_outfit",					&CScriptGameObject::IsCustomOutfit)
		.def("is_physics_shell_holder",		&CScriptGameObject::IsPhysicsShellHolder)
		.def("is_projector",				&CScriptGameObject::IsProjector)
		.def("is_scope",					&CScriptGameObject::IsScope)
		.def("is_script_zone",				&CScriptGameObject::IsScriptZone)
		.def("is_silencer",					&CScriptGameObject::IsSilencer)
		.def("is_space_restrictor",			&CScriptGameObject::IsSpaceRestrictor)
		.def("is_stalker",					&CScriptGameObject::IsStalker)
		.def("is_torch",					&CScriptGameObject::IsTorch)
		.def("is_trader",					&CScriptGameObject::IsTrader)
		.def("is_weapon",					&CScriptGameObject::IsWeapon)
		.def("is_weapon_gl",				&CScriptGameObject::IsWeaponGL)
		.def("is_weapon_magazined",			&CScriptGameObject::IsWeaponMagazined)
	; return(instance);
}