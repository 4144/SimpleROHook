#pragma once

#define SHAREDMEMORY_OBJECTNAME _T("SimpleROHook1008")

typedef struct _StSHAREDMEMORY{
	HWND	g_hROWindow;

	DWORD	executeorder;


	BOOL	write_packetlog;
	BOOL	freemouse; 
	BOOL	m2e; 
	BOOL	fix_windowmode_vsyncwait;
	BOOL	show_framerate;
	BOOL	objectinformation;
	BOOL	_44khz_audiomode;
	int		cpucoolerlevel;

	WCHAR	configfilepath[MAX_PATH];
	WCHAR	musicfilename[MAX_PATH];

}StSHAREDMEMORY;
