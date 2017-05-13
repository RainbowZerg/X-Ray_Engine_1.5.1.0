// script_game_object_testing.cpp
// Функции для тестирования объектов по их классу

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

#define TEST_OBJECT_CLASS(A,B)					\
bool A () const									\
{												\
	B* l_tpEntity = smart_cast<B*>(&object());	\
	if (!l_tpEntity)							\
		return false;							\
												\
	return true;								\
};												\

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