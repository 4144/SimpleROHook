// dllmain.cpp : DLL �A�v���P�[�V�����̃G���g�� �|�C���g���`���܂��B
#include "stdafx.h"

#include "tinyconsole.h"

#include "RoCodeBind.h"

#include "ProxyDirectDraw.h"
#include "ProxyDirectInput.h"

template <typename T>
BOOL InstallProxyFunction(LPCTSTR dllname,LPCTSTR exportname,T ProxyFunction,T *pOriginalFunction)
{
	BOOL result = FALSE;
	std::stringstream fullpath;

	TCHAR systemdir[MAX_PATH];
	HINSTANCE hDll;
	::GetSystemDirectory( systemdir, MAX_PATH);

	//fullpath.Format(_T("%s\\%s"), systemdir, dllname);
	fullpath << systemdir << "\\" << dllname;
	hDll = ::LoadLibrary(fullpath.str().c_str() );

	if( !hDll )
		return result;

	BYTE *p = (BYTE*)::GetProcAddress( hDll, exportname);

	if( p )
	{
		if(     p[ 0] == 0x8b && p[ 1] == 0xff &&
			( ( p[-5] == 0x90 && p[-4] == 0x90 && p[-3] == 0x90 && p[-2] == 0x90 && p[-1] == 0x90 ) || 
			  ( p[-5] == 0xcc && p[-4] == 0xcc && p[-3] == 0xcc && p[-2] == 0xcc && p[-1] == 0xcc ) )  )
		{
			// find hotpatch structure.
			//
			// 9090909090 nop  x 5
			// 8bff       mov  edi,edi
			//       or
			// cccccccccc int 3 x 5
			// 8bff       mov edi,edi
			DWORD flOldProtect, flDontCare;
			if ( ::VirtualProtect(   (LPVOID)&p[-5], 7 , PAGE_READWRITE, &flOldProtect) )
			{
				p[-5] = 0xe9;              // jmp
				p[ 0] = 0xeb; p[ 1] = 0xf9;// jmp short [pc-7]

				*pOriginalFunction = (T)&p[2];
				*((DWORD*)&p[-4])  = (DWORD)ProxyFunction - (DWORD)&p[-5] -5;

				::VirtualProtect( (LPVOID)&p[-5], 7 , flOldProtect, &flDontCare);
				result = TRUE;
			}
		}else
		if( p[-5] == 0xe9 &&
			p[ 0] == 0xeb && p[ 1] == 0xf9 )
		{
			// find hotpached function.
			// jmp **** 
			// jmp short [pc -7]
			DWORD flOldProtect, flDontCare;
			if ( ::VirtualProtect(   (LPVOID)&p[-5], 7 , PAGE_READWRITE, &flOldProtect) )
			{
				*pOriginalFunction = (T)(*((DWORD*)&p[-4]) + (DWORD)&p[-5] +5);
				*((DWORD*)&p[-4])  = (DWORD)ProxyFunction - (DWORD)&p[-5] -5;

				::VirtualProtect( (LPVOID)&p[-5], 7 , flOldProtect, &flDontCare);
				result = TRUE;
			}
		}
	}
	::FreeLibrary(hDll);

	return result;
}

void *pResumeAIL_open_digital_driverFunction;
void __declspec(naked) ProxyAIL_open_digital_driver(void)
{
	__asm mov   eax,[esp+0x04] // eax = soundrate
	__asm add   [esp+0x04],eax // soundrate + soundrate ( soundrate x 2 )
	__asm sub   esp,0x010
	__asm jmp   pResumeAIL_open_digital_driverFunction
}

BOOL RagexeSoundRateFixer(void)
{
	BOOL result = FALSE;
	std::stringstream fullpath;

	TCHAR currentdir[MAX_PATH];
	HINSTANCE hDll;
	::GetCurrentDirectoryA( MAX_PATH,currentdir );

	//fullpath.Format(_T("%s\\Mss32.dll"), currentdir);
	fullpath << currentdir << "\\Mss32.dll";
	hDll = ::LoadLibrary(fullpath.str().c_str() );

	if( !hDll ){
		return result;
	}

	BYTE *p = (BYTE*)::GetProcAddress( hDll, "_AIL_open_digital_driver@16");

	if( p )
	{
		if( p[ 0] == 0x83 && p[ 1] == 0xec && p[ 2] == 0x10 )
		{
			// find hotpatch structure.
			DWORD flOldProtect, flDontCare;
			if ( ::VirtualProtect(   (LPVOID)&p[-5], 7 , PAGE_READWRITE, &flOldProtect) )
			{
				p[-5] = 0xe9;              // jmp
				p[ 0] = 0xeb; p[ 1] = 0xf9;// jmp short [pc-7]
				p[ 2] = 0x90; // nop
				pResumeAIL_open_digital_driverFunction = &p[3];
				*((DWORD*)&p[-4])  = (DWORD)ProxyAIL_open_digital_driver - (DWORD)&p[-5] -5;

				::VirtualProtect( (LPVOID)&p[-5], 7 , flOldProtect, &flDontCare);
				result = TRUE;
			}
		}
	}
	::FreeLibrary(hDll);

	return result;
}



extern HINSTANCE g_hDLL;

typedef HRESULT (WINAPI *tDirectDrawCreateEx)( GUID FAR *lpGUID, LPVOID *lplpDD, REFIID iid, IUnknown FAR *pUnkOuter );
typedef HRESULT (WINAPI *tDirectInputCreateA)( HINSTANCE hinst, DWORD dwVersion, LPDIRECTINPUTA *ppDI, LPUNKNOWN punkOuter);

typedef int (WSAAPI *tWS32_recv)( SOCKET s, char *buf,int len,int flags );

typedef BOOL (WINAPI *tPeekMessageA)(LPMSG lpMsg, HWND hWnd, UINT wMsgFilterMin, UINT wMsgFilterMax, UINT wRemoveMsg);

tDirectDrawCreateEx OrigDirectDrawCreateEx = NULL;
tDirectInputCreateA OrigDirectInputCreateA = NULL;
tWS32_recv OrigWS32_recv = NULL;

//tPeekMessageA OrigPeekMessageA = NULL;
//
//BOOL WINAPI ProxyPeekMessageA(
//	LPMSG lpMsg, HWND hWnd, UINT wMsgFilterMin, UINT wMsgFilterMax, UINT wRemoveMsg)
//{
//	return OrigPeekMessageA(lpMsg,hWnd,wMsgFilterMin,wMsgFilterMax,wRemoveMsg);
//}

int WSAAPI ProxyWS32_recv( SOCKET s, char *buf,int len,int flags )
{
	int result;

	result = OrigWS32_recv(s,buf,len,flags);
	if( result >= 2 ){
		kDD_LOGGING2( ("recv [0]=%02X [1]=%02X\n",(unsigned char)buf[0],(unsigned char)buf[1]) );
	}
	return result;
}

void hook_ws2_32_recv(void)
{
	if( InstallProxyFunction( _T("ws2_32.dll"), _T("recv"), ProxyWS32_recv, &OrigWS32_recv ) )
	{
		MessageBox(NULL,"ws2_32 hook enable","debug",MB_OK);
	}else{
		MessageBox(NULL,"ws2_32 hook failed","debug",MB_OK);
	}
}



HRESULT WINAPI ProxyDirectDrawCreateEx(
	GUID FAR     *lpGuid,
	LPVOID       *lplpDD,
	REFIID        iid,
	IUnknown FAR *pUnkOuter )
{
	kDD_LOGGING( ("DirectDrawCreateEx hookfunc\n") );

	HRESULT Result = OrigDirectDrawCreateEx( lpGuid, lplpDD, iid, pUnkOuter );
	if(FAILED(Result))
		return Result;

	CProxy_IDirectDraw7 *lpcDD;
	*lplpDD = lpcDD = new CProxy_IDirectDraw7((IDirectDraw7*)*lplpDD);
	lpcDD->setThis(lpcDD);

	kDD_LOGGING( ("DirectDrawCreateEx Hook hookfunc") );

    return Result;
}

HRESULT WINAPI ProxyDirectInputCreateA(
	HINSTANCE hinst,
	DWORD dwVersion,
	LPDIRECTINPUTA *ppDI,
	LPUNKNOWN punkOuter )
{
	kDD_LOGGING( ("DirectInputCreateA hookfunc instance = %08X",g_ROmouse) );

	HRESULT Result = OrigDirectInputCreateA( hinst, dwVersion, ppDI, punkOuter );

	if(FAILED(Result))
		return Result;
	if(dwVersion == 0x0700){
		*ppDI = new CProxy_IDirectInput7((IDirectInput7*)*ppDI);
	}
	kDD_LOGGING( ("DirectInputCreateA Hook hookfunc") );

    return Result;
}


BOOL IsRagnarokApp(void)
{
	TCHAR filename[MAX_PATH];

	::GetModuleFileName( ::GetModuleHandle( NULL ), filename, sizeof(filename)/sizeof(TCHAR) );
	::PathStripPath( filename );
	// check the module filename
	if( _tcsicmp( filename, _T("Ragexe.exe") ) == 0
	 || _tcsicmp( filename, _T("Sakexe.exe") ) == 0
	 || _tcsicmp( filename, _T("clragexe.exe") ) == 0
	 || _tcsicmp( filename, _T("2011-11-22_Ragexe.exe") ) == 0
	 || _tcsicmp( filename, _T("2011-12-01aRagexe.exe") ) == 0
	 || _tcsicmp( filename, _T("2012-8-21data_gm_r_Ragexe.exe") ) == 0
	 || _tcsicmp( filename, _T("2012-8-16data_gm_r_Ragexe.exe") ) == 0
	 || _tcsicmp( filename, _T("2012-8-16data_gm_r_clragexe.exe") ) == 0
	 || _tcsicmp( filename, _T("2012-11-15data_gm_Ragexe.exe") ) == 0
	 || _tcsicmp( filename, _T("F2P_Ragexe.exe") ) == 0
	 || _tcsicmp( filename, _T("RagFree.exe") ) == 0
	 || _tcsicmp( filename, _T("HighPriest.exe") ) == 0
		)
	{
		return TRUE;
	}
	return FALSE;
}

BOOL APIENTRY DllMain( HMODULE hModule,
                       DWORD  ul_reason_for_call,
                       LPVOID lpReserved
					 )
{
	switch (ul_reason_for_call)
	{
	case DLL_THREAD_ATTACH:
	case DLL_THREAD_DETACH:
		break;

	case DLL_PROCESS_ATTACH:
		g_hDLL = hModule;
		if( IsRagnarokApp() )
		{
			TCHAR temppath[MAX_PATH];
			::DisableThreadLibraryCalls( hModule );
			CreateTinyConsole();
			OpenSharedMemory();
			//::MessageBox(NULL,_T("DLL_PROCESS_ATTACH"),_T("debug"),MB_OK);
			InstallProxyFunction( _T("ddraw.dll"),  _T("DirectDrawCreateEx"), ProxyDirectDrawCreateEx, &OrigDirectDrawCreateEx );
			InstallProxyFunction( _T("dinput.dll"), _T("DirectInputCreateA"), ProxyDirectInputCreateA, &OrigDirectInputCreateA );
			//InstallProxyFunction( _T("ws2_32.dll"), _T("recv"), ProxyWS32_recv, &OrigWS32_recv );

			if( g_pSharedData ){
				::GetCurrentDirectory(MAX_PATH,temppath);
				strcat_s( temppath,"\\BGM\\");
				//
				::MultiByteToWideChar(CP_ACP,MB_PRECOMPOSED,
					temppath,strlen(temppath)+1,
					g_pSharedData->musicfilename,MAX_PATH);
				//strcpy_s( g_pSharedData->musicfilename,temppath );
				//MessageBox(NULL,g_pSharedData->musicfilename,"BGM path",MB_OK);
				g_MouseFreeSw = g_pSharedData->freemouse;
				if( g_pSharedData->_44khz_audiomode )
					RagexeSoundRateFixer();
			}

		}
		break;
	case DLL_PROCESS_DETACH:
		if( IsRagnarokApp() )
		{
			ReleaseTinyConsole();
			ReleaceSharedMemory();
		}
		break;
	}
	return TRUE;
}

