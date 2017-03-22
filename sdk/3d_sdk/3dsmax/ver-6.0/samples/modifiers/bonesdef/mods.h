/**********************************************************************
 *<
	FILE: mods.h

	DESCRIPTION:

	CREATED BY: Rolf Berteig (based on prim.h)

	HISTORY:

 *>	Copyright (c) 1994, All Rights Reserved.
 **********************************************************************/

#ifndef __MODS__H
#define __MODS__H

#include "Max.h"
//#include "reslib.h"
#include "modsres.h"


TCHAR *GetString(int id);

extern ClassDesc* GetBonesDefModDesc();
extern ClassDesc* GetGizmoJointDesc();
// in mods.cpp
extern HINSTANCE hInstance;

// For 'Supports Object of Type' rollups
extern INT_PTR CALLBACK DefaultSOTProc(HWND hWnd,UINT msg,WPARAM wParam,LPARAM lParam);

#define BIGFLOAT	float( 1.0e+37F )

#define NEWSWMCAT	_T("Modifiers")

#endif

