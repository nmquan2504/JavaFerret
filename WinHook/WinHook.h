// WinHook.h : main header file for the WinHook DLL
//

#pragma once

#ifndef __AFXWIN_H__
	#error "include 'stdafx.h' before including this file for PCH"
#endif

#include "resource.h"		// main symbols
#include <Psapi.h>

// CWinHookApp
// See WinHook.cpp for the implementation of this class
//

typedef BOOL (*FPTR_Init)(BOOL bHasUI);
typedef HWND (*FPTR_GetJavaFerretWindow)();
typedef BOOL (*FPTR_echoMouseObject)();

class CWinHookApp : public CWinApp
{
public:
	CWinHookApp();

// Overrides
public:
	virtual BOOL InitInstance();

	static BOOL InitHook(DWORD dwMode);
	static LRESULT CALLBACK CallWndProc(int nCode, WPARAM wParam, LPARAM lParam);
	static LRESULT CALLBACK GetMsgProc(int nCode, WPARAM wParam, LPARAM lParam);
	static LRESULT CALLBACK KeyboardProc(int nCode, WPARAM wParam, LPARAM lParam);
	static LRESULT CALLBACK MouseProc(int nCode, WPARAM wParam, LPARAM lParam);
	static LRESULT WINAPI LLMouseProc(int nCode, WPARAM wParam, LPARAM lParam);
	static void HookProc(MSG &msg);

public:
	static CString GetModuleFileName(DWORD dwPID);
	static CString GetAppLocation();
	static void LogString(char *msg, ...);

	static void CALLBACK Test(HWND hwnd, UINT uMsg, UINT timerId, DWORD dwTime);
	static DWORD WINAPI CWinHookApp::Test2(LPVOID lpParam);

public:
	static HMODULE s_hInstance;
	static HMODULE s_hJavaFerretInstance;
	static bool isFirstCaptureMessage;
	static bool isStopped;
	static bool isJavaFerretLoaded;

	DECLARE_MESSAGE_MAP()
};
