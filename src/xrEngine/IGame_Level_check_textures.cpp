#include "stdafx.h"
//#include "resourcemanager.h"
#include "igame_level.h"

void IGame_Level::LL_CheckTextures()
{
	u32	m_base,c_base,m_lmaps,c_lmaps;
	//Device.Resources->_GetMemoryUsage		(m_base,c_base,m_lmaps,c_lmaps);
	Device.m_pRender->ResourcesGetMemoryUsage(m_base,c_base,m_lmaps,c_lmaps);

	Msg	("* t-report - base: %d, %d K",	c_base,		m_base/1024);
	Msg	("* t-report - lmap: %d, %d K",	c_lmaps,	m_lmaps/1024);
}
