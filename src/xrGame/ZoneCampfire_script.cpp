#include "StdAfx.h"
#include "pch_script.h"
#include "ZoneCampfire.h"

using namespace luabind;

#pragma optimize("s",on)
void CZoneCampfire::script_register(lua_State *L)
{
	module(L)
		[
			class_<CZoneCampfire, CGameObject>("CZoneCampfire")
			.def(constructor<>())
			.def("switch",		&CZoneCampfire::Switch)
			.def("on",			&CZoneCampfire::Enabled)
			.def("get_light",	&CZoneCampfire::GetLight)
		];
}
