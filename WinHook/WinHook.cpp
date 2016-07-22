// WinHook.cpp : Defines the initialization routines for the DLL.
//

#include "stdafx.h"
#include "WinHook.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#endif

//system scope variable
#pragma data_seg(".SHARED")
DWORD				g_dwMode = 0;
HHOOK				g_hCallWndHook = NULL;
HHOOK				g_hGetMsgHook = NULL;
HHOOK				g_hKeyboardHook = NULL;
HHOOK				g_hMouseHook = NULL;
HHOOK				g_hLLMouseHook = NULL;
HWND				g_hReceiver = NULL;
DWORD				g_ReceiverProcessId = 0;
BOOL				g_isPaused = FALSE;
#pragma data_seg()
#pragma comment(linker, "/section:.SHARED,RWS")

HMODULE CWinHookApp::s_hInstance = NULL;
HMODULE CWinHookApp::s_hJavaFerretInstance = NULL;
bool CWinHookApp::isStopped = false;
bool CWinHookApp::isFirstCaptureMessage = true;
bool CWinHookApp::isJavaFerretLoaded = false;

//
//TODO: If this DLL is dynamically linked against the MFC DLLs,
//		any functions exported from this DLL which call into
//		MFC must have the AFX_MANAGE_STATE macro added at the
//		very beginning of the function.
//
//		For example:
//
//		extern "C" BOOL PASCAL EXPORT ExportedFunction()
//		{
//			AFX_MANAGE_STATE(AfxGetStaticModuleState());
//			// normal function body here
//		}
//
//		It is very important that this macro appear in each
//		function, prior to any calls into MFC.  This means that
//		it must appear as the first statement within the 
//		function, even before any object variable declarations
//		as their constructors may generate calls into the MFC
//		DLL.
//
//		Please see MFC Technical Notes 33 and 58 for additional
//		details.
//

/* 
 * writes a text string to a logfile
 */
#define WIN_HOOK_LOG "WinHook.log"
FILE* logfile = NULL;
void CWinHookApp::LogString(char *msg, ...) {

	if (logfile == NULL) {
		char szWinHookLog[MAX_PATH+1];
		::GetModuleFileNameA(s_hInstance, szWinHookLog, MAX_PATH);
		for (int i = strlen(szWinHookLog) - 1; i >= 0; i--) {
			if (szWinHookLog[i] == '\\') {
				szWinHookLog[i + 1] = '\0';
				break;
			}
		}
		strcat(szWinHookLog, WIN_HOOK_LOG);
		logfile = fopen(szWinHookLog, "w"); // overwrite existing log file
		LogString("WinHook.log \n");
	}

    char tmpbuf[1024];

	if (msg != NULL) {
		va_list argprt;
		va_start(argprt, msg);
		vsprintf(tmpbuf, msg, argprt);
	}

	if (logfile != NULL) {
		fprintf(logfile, "[%d][%d]", GetCurrentProcessId(), GetCurrentThreadId());
		fprintf(logfile, tmpbuf);
		fprintf(logfile, "\n");
		fflush(logfile);
	}
}

// CWinHookApp

BEGIN_MESSAGE_MAP(CWinHookApp, CWinApp)
END_MESSAGE_MAP()


// CWinHookApp construction

CWinHookApp::CWinHookApp()
{
	// TODO: add construction code here,
	// Place all significant initialization in InitInstance
}


// The one and only CWinHookApp object

CWinHookApp theApp;


// CWinHookApp initialization

BOOL CWinHookApp::InitInstance()
{
	CWinApp::InitInstance();
	s_hInstance = this->m_hInstance;
	return TRUE;
}

BOOL CWinHookApp::InitHook(DWORD dwMode) 
{
	g_dwMode = dwMode;
	g_ReceiverProcessId = ::GetCurrentProcessId();
	isFirstCaptureMessage = true;
	HINSTANCE hWinHook = ::GetModuleHandle(_T("WinHook.dll"));
	if(hWinHook == NULL)
	{
		hWinHook = ::LoadLibrary(_T("WinHook.dll"));
	}
	HOOKPROC pCallWndProc = NULL;
	pCallWndProc = (HOOKPROC)::GetProcAddress(hWinHook, "CallWndProc");
	HOOKPROC pGetMsgProc = NULL;
	pGetMsgProc = (HOOKPROC)::GetProcAddress(hWinHook, "GetMsgProc");
	/*HOOKPROC pKeyboardProc = NULL;
	pKeyboardProc = (HOOKPROC)::GetProcAddress(hWinHook, "KeyboardProc");*/
	if(pCallWndProc != NULL)
	{
		g_hCallWndHook = ::SetWindowsHookEx(WH_CALLWNDPROC, pCallWndProc, hWinHook, NULL);
	}
	if(pGetMsgProc != NULL)
	{
		g_hGetMsgHook = ::SetWindowsHookEx(WH_GETMESSAGE, pGetMsgProc, hWinHook, NULL);
	}
	//if(pKeyboardProc != NULL)
	//{
	//	g_hKeyboardHook = ::SetWindowsHookEx(WH_KEYBOARD, pKeyboardProc, hWinHook, NULL);
	//}

	//if (g_hLLMouseHook == NULL)
	//{
	//	g_hLLMouseHook = ::SetWindowsHookEx(WH_MOUSE_LL, LLMouseProc, s_hInstance, NULL);
	//}

	return TRUE;
}

LRESULT CALLBACK CWinHookApp::CallWndProc(int nCode, WPARAM wParam, LPARAM lParam)
{
	AFX_MANAGE_STATE(AfxGetStaticModuleState());

	if (nCode < 0 || lParam == NULL)
	{
		return CallNextHookEx(g_hCallWndHook, nCode, wParam, lParam);
	}
    
	LPCWPSTRUCT lpCwp = (LPCWPSTRUCT) lParam;
	if(lpCwp == NULL)
	{
		return CallNextHookEx(g_hCallWndHook, nCode, wParam, lParam);
	}

	MSG msg = {0};
	msg.lParam = lpCwp->lParam;
	msg.wParam = lpCwp->wParam;
	msg.message = lpCwp->message;
	msg.hwnd = lpCwp->hwnd;
	msg.time = GetTickCount();
	HookProc(msg);
	return CallNextHookEx(g_hCallWndHook, nCode, wParam, lParam);
}

LRESULT CALLBACK CWinHookApp::GetMsgProc(int nCode, WPARAM wParam, LPARAM lParam)
{
	AFX_MANAGE_STATE(AfxGetStaticModuleState());

	if (nCode < 0)
	{
		return CallNextHookEx(g_hGetMsgHook, nCode, wParam, lParam);
	}

	PMSG pMsg = (PMSG) lParam;
	if (pMsg)
	{
		HookProc(*pMsg);
	}
	return CallNextHookEx(g_hGetMsgHook, nCode, wParam, lParam);
}

#define DOWN_UP_FLAG 1<<31

LRESULT CALLBACK CWinHookApp::KeyboardProc(int nCode, WPARAM wParam, LPARAM lParam)
{
	AFX_MANAGE_STATE(AfxGetStaticModuleState());

	if (nCode < 0) 
	{
		CallNextHookEx(g_hKeyboardHook, nCode, wParam, lParam);
	}
	else if (wParam == VK_SHIFT && lParam & DOWN_UP_FLAG) 
	{
		LogString("KeyboardProc[VK_SHIFT]");
		Test(NULL, NULL, 0, 0);
	}

	return 0;
}

bool paused = true;

LRESULT CALLBACK CWinHookApp::MouseProc(int nCode, WPARAM wParam, LPARAM lParam)
{
	AFX_MANAGE_STATE(AfxGetStaticModuleState());

	if (nCode < 0) 
	{
		CallNextHookEx(g_hMouseHook, nCode, wParam, lParam);
	}
	else
	{
		LPMOUSEHOOKSTRUCTEX lpMouse = (LPMOUSEHOOKSTRUCTEX) lParam;
		if (wParam == WM_LBUTTONDOWN) 
		{
			LogString("MouseProc[WM_LBUTTONDOWN]");
			//HWND hWndJavaFerret;
			//FPTR_GetJavaFerretWindow pfnGetJavaFerretWindow = (FPTR_GetJavaFerretWindow)::GetProcAddress(s_hJavaFerretInstance, "GetJavaFerretWindow");
			//if (pfnGetJavaFerretWindow) 
			//{
			//	hWndJavaFerret = pfnGetJavaFerretWindow();
			//}
			//paused = true;
			////PostMessage(hWndJavaFerret, WM_USER + 1, NULL, NULL);
			//KillTimer(NULL, 1234);
			//SetTimer(NULL, 1234, 100, Test);
			//MSG msg;
			//while (paused)
			//{
			//	while (paused && ::PeekMessage(&msg, NULL, NULL, NULL, PM_REMOVE))
			//	{
			//		::TranslateMessage(&msg);
			//		::DispatchMessage(&msg);
			//	}
			//}
		}
	}

	return 0;
}



LRESULT CWinHookApp::LLMouseProc(int nCode, WPARAM wParam, LPARAM lParam)
{
	AFX_MANAGE_STATE(AfxGetStaticModuleState());

	if (nCode < 0) 
	{
		return CallNextHookEx(NULL, nCode, wParam, lParam);
	}

	if (wParam == WM_LBUTTONDOWN) 
	{
		if (!isJavaFerretLoaded) {
			isJavaFerretLoaded = true;

			s_hJavaFerretInstance = LoadLibrary(GetAppLocation() + _T("\\JavaFerret.dll"));

			FPTR_Init pfnInit = (FPTR_Init)::GetProcAddress(s_hJavaFerretInstance, "Init");
			if (pfnInit) 
			{
				if (g_dwMode == 0) /* Use JavaFerret UI */
				{
					LogString("Init JavaFerret in-process with UI.");
					pfnInit(TRUE);
				}
				else if (g_dwMode == 1) /* Dont use JavaFerret UI */
				{
					LogString("Init JavaFerret in-process without UI.");
					pfnInit(FALSE);
				}
			}
		}


		LogString("LLMouseProc[WM_LBUTTONDOWN]");
		HWND hWndJavaFerret;
		FPTR_GetJavaFerretWindow pfnGetJavaFerretWindow = (FPTR_GetJavaFerretWindow)::GetProcAddress(s_hJavaFerretInstance, "GetJavaFerretWindow");
		if (pfnGetJavaFerretWindow) 
		{
			hWndJavaFerret = pfnGetJavaFerretWindow();
		}

		FPTR_echoMouseObject pfnEchoMouseObject = (FPTR_echoMouseObject)::GetProcAddress(s_hJavaFerretInstance, "echoMouseObject");
		if (pfnEchoMouseObject) 
		{
			pfnEchoMouseObject();
		}



		//PostMessage(hWndJavaFerret, WM_USER + 1, NULL, NULL);
		//KillTimer(NULL, 1234);
		//SetTimer(NULL, 1234, 100, Test);
		
	}

	return CallNextHookEx(NULL, nCode, wParam, lParam);
}

void CALLBACK CWinHookApp::Test(HWND hwnd, UINT uMsg, UINT timerId, DWORD dwTime)
{
	HWND hWndJavaFerret;
	FPTR_GetJavaFerretWindow pfnGetJavaFerretWindow = (FPTR_GetJavaFerretWindow)::GetProcAddress(s_hJavaFerretInstance, "GetJavaFerretWindow");
	if (pfnGetJavaFerretWindow) 
	{
		hWndJavaFerret = pfnGetJavaFerretWindow();
	}
	FPTR_echoMouseObject pfnEchoMouseObject = (FPTR_echoMouseObject)::GetProcAddress(s_hJavaFerretInstance, "echoMouseObject");
	if (pfnEchoMouseObject) 
	{
		pfnEchoMouseObject();
	}
	paused = false;
	KillTimer(hwnd, timerId);
}

DWORD WINAPI CWinHookApp::Test2(LPVOID lpParam)
{
	HWND hWndJavaFerret;
	FPTR_GetJavaFerretWindow pfnGetJavaFerretWindow = (FPTR_GetJavaFerretWindow)::GetProcAddress(s_hJavaFerretInstance, "GetJavaFerretWindow");
	if (pfnGetJavaFerretWindow) 
	{
		hWndJavaFerret = pfnGetJavaFerretWindow();
	}
	PostMessage(hWndJavaFerret, WM_USER + 1, NULL, NULL);
	paused = false;
	return TRUE;
}


CWnd* asdf = NULL;


void CWinHookApp::HookProc(MSG &msg)
{
	if(isStopped
		|| g_isPaused
		|| !::IsWindow(msg.hwnd))
	{
		//Press key state when the watcher form is active,
		// send the key to the window at point
		DWORD currentProcessId = ::GetCurrentProcessId();
		if(currentProcessId == g_ReceiverProcessId)
		{
			if (msg.message == WM_KEYDOWN && ::GetKeyState(VK_LSHIFT))
			{
				POINT pt;
				pt.x = pt.y = 0;
				::GetCursorPos(&pt);
				HWND hWindowFromPoint = ::WindowFromPoint(pt);
				if (hWindowFromPoint != g_hReceiver)
				{
					::SendMessage(hWindowFromPoint, msg.message, msg.wParam, msg.lParam);
				}
			}
		}
		return;
	}

	if ((isFirstCaptureMessage == true) && 
		(msg.message == WM_LBUTTONDOWN
		||  msg.message == WM_NCLBUTTONDOWN
		||  msg.message == WM_MOUSEMOVE
		||  msg.message == WM_NCMOUSEMOVE
		||  msg.message == WM_RBUTTONDOWN
		||  msg.message == WM_NCRBUTTONDOWN
		|| (msg.message == WM_KEYDOWN)
		|| (msg.message == WM_KEYDOWN && msg.wParam == VK_TAB)
		||  msg.message == WM_LBUTTONUP
		||  msg.message == WM_NCLBUTTONUP
		||  msg.message == WM_RBUTTONUP
		||  msg.message == WM_NCRBUTTONUP
		|| (msg.message == WM_KEYUP && msg.wParam == VK_TAB)))
	{
		::CoInitialize(NULL);
		isFirstCaptureMessage = false;
		DWORD currentProcessId = ::GetCurrentProcessId();
		if(currentProcessId == g_ReceiverProcessId)
		{
			isStopped = true;
			return;
		}
		CString szModule = GetModuleFileName(currentProcessId).MakeLower();
		//TraceVerbose(TraceUnDefine, szModule);
		if(szModule.Find(_T("devenv.exe")) >= 0
			|| szModule.Find(_T("taskmgr.exe")) >= 0)
		{
			isStopped = true;
			return;
		}

		HWND hWnd = msg.hwnd;

		if (szModule.Find(_T("jp2launcher.exe")) >= 0
			|| szModule.Find(_T("java.exe")) >= 0
			|| szModule.Find(_T("javaw.exe")) >= 0)
		{
			// Load JavaFerret
			s_hJavaFerretInstance = LoadLibrary(GetAppLocation() + _T("\\JavaFerret.dll"));
			if (s_hJavaFerretInstance != NULL)
			{
				isJavaFerretLoaded = true;
				FPTR_Init pfnInit = (FPTR_Init)::GetProcAddress(s_hJavaFerretInstance, "Init");
				if (pfnInit) 
				{
					if (g_dwMode == 0) /* Use JavaFerret UI */
					{
						LogString("Init JavaFerret in-process with UI.");
						pfnInit(TRUE);
					}
					else if (g_dwMode == 1) /* Dont use JavaFerret UI */
					{
						LogString("Init JavaFerret in-process without UI.");
						pfnInit(FALSE);
					}
				}
			}
		}
		
	}

	if(msg.message == WM_LBUTTONDOWN || msg.message == WM_LBUTTONDBLCLK
		/*|| msg.message == WM_NCLBUTTONDOWN || msg.message == WM_NCLBUTTONDBLCLK
		|| msg.message == WM_LBUTTONUP || msg.message == WM_NCLBUTTONUP*/)
	{
		if (isJavaFerretLoaded)
		{
			HWND hWndJavaFerret = NULL;
			FPTR_GetJavaFerretWindow pfnGetJavaFerretWindow = (FPTR_GetJavaFerretWindow)::GetProcAddress(s_hJavaFerretInstance, "GetJavaFerretWindow");
			if (pfnGetJavaFerretWindow) 
			{
				hWndJavaFerret = pfnGetJavaFerretWindow();
			}
			bool bIsOnJavaFerret = false;
			HWND hWndTmp = msg.hwnd;
			while (hWndTmp != NULL && hWndTmp != hWndJavaFerret && hWndTmp != GetDesktopWindow()) 
			{
				hWndTmp = GetParent(hWndTmp);
			}
			bool bProceed = (g_dwMode==0) ? (hWndJavaFerret != NULL && hWndTmp != hWndJavaFerret) : true;
			if (bProceed)
			{
				LogString("WM_LBUTTONDOWN");
				FPTR_echoMouseObject pfnEchoMouseObject = (FPTR_echoMouseObject)::GetProcAddress(s_hJavaFerretInstance, "echoMouseObject");
				if (pfnEchoMouseObject) 
				{
					pfnEchoMouseObject();
				}
			}
		}
	}
}

CString CWinHookApp::GetModuleFileName(DWORD dwPID)
{
	//Base on module name
	HMODULE hMods[1024];
	HANDLE hProcess;
	DWORD cbNeeded;    
	unsigned int i;	
	CString szModuleFileName;

	szModuleFileName.Empty();

	if (dwPID != 0)
	{		
		hProcess = OpenProcess(MAXIMUM_ALLOWED, FALSE, dwPID);

		if(!hProcess)
			return szModuleFileName;

		TCHAR szExeFileName[MAX_PATH + 1] = {0};

		if(GetProcessImageFileName(hProcess,szExeFileName,MAX_PATH)!=0)
		{
			szModuleFileName = szExeFileName;
			goto Exit;
		}

		if( EnumProcessModules(hProcess, hMods, sizeof(hMods), &cbNeeded))
		{
			for ( i = 0; i < (cbNeeded / sizeof(HMODULE)); i++ )
			{
				TCHAR szModName[MAX_PATH+1]={0};
				// Get the full path to the module's file.
				if ( GetModuleFileNameEx( hProcess, hMods[i], (LPTSTR)szModName, MAX_PATH))
				{	
					szModuleFileName = szModName;
					goto Exit;
				}
			}
		}
Exit:
		CloseHandle( hProcess );
	}

	szModuleFileName.MakeLower();
	return szModuleFileName;
}

CString CWinHookApp::GetAppLocation()
{
	TCHAR szPathName[MAX_PATH];
	CString strPath;
	::GetModuleFileName(s_hInstance, &szPathName[0], MAX_PATH);
	strPath = szPathName;
	int slashPos = strPath.ReverseFind('\\');
	if (slashPos == strPath.GetLength())
	{
		return _T("");//means can not get exe location => return ""
	}
	strPath = strPath.Left(slashPos);
	return strPath;
}