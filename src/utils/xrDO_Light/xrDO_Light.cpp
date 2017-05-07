#include "stdafx.h"
#include "process.h"
#include "global_options.h"

#pragma comment(lib,"comctl32.lib")
#pragma comment(lib,"winmm.LIB")
#pragma comment(lib,"xrCDB.lib")
#pragma comment(lib,"xrCore.lib")
#pragma comment(lib,"FreeImage.lib")

extern void	xrCompiler			();
extern void logThread			(void *dummy);
extern volatile BOOL bClose;

static const char* h_str = 
	"The following keys are supported / required:\n"
	"-? or -h	== this help\n"
	"-f<NAME>	== compile level in gamedata\\levels\\<NAME>\\\n"
	"-norgb		== disable common lightmap calculating\n"
	"-nosun		== disable sun-lighting\n"
	"\n"
	"NOTE: The last key is required for any functionality\n";

void Help()
{	
	MessageBox(0,h_str,"Command line options",MB_OK|MB_ICONINFORMATION); 
}

void Startup(LPSTR lpCmdLine)
{
	char cmd[512],name[256];
	strcpy_s(cmd,lpCmdLine);
	strlwr(cmd);

	if (strstr(cmd, "-?") || strstr(cmd, "-h") || strstr(cmd, "-f") == 0)
	{ 
		Help();
		return; 
	}

	if (strstr(cmd, "-norgb")) b_norgb = true;
	if (strstr(cmd, "-nosun")) b_nosun = true;

	// Give a LOG-thread a chance to startup
	InitCommonControls	();
	thread_spawn		(logThread,	"log-update", 1024*1024,0);
	Sleep				(150);
	
	// Load project
	name[0]=0; sscanf	(strstr(cmd,"-f")+2,"%s",name);

	extern  HWND logWindow;
	string256			temp;
	sprintf_s			(temp, "%s - Detail Compiler", name);
	SetWindowText		(logWindow, temp);

	//FS.update_path	(name,"$game_levels$",name);
	FS.get_path			("$level$")->_set	(name);

	CTimer				dwStartupTime; dwStartupTime.Start();
	xrCompiler			();

	// Show statistic
	char	stats[256];
	extern	std::string make_time(u32 sec);
	sprintf				(stats,"Time elapsed: %s",make_time((dwStartupTime.GetElapsed_ms())/1000).c_str());

	if (!strstr(cmd,"-silent"))
		MessageBox		(logWindow,stats,"Congratulation!",MB_OK|MB_ICONINFORMATION);

	bClose				= TRUE;
	Sleep				(500);
}

int APIENTRY WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR lpCmdLine, int nCmdShow)
{
	// Initialize debugging
	Debug._initialize	(false);
	Core._initialize	("xrDO");
	Startup				(lpCmdLine);
	Core._destroy		();
	
	return 0;
}
