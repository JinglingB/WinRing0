//-----------------------------------------------------------------------------
//     Author : hiyohiyo
//       Mail : hiyohiyo@crystalmark.info
//        Web : http://openlibsys.org/
//    License : The modified BSD license
//
//                     Copyright 2007-2020 OpenLibSys.org. All rights reserved.
//-----------------------------------------------------------------------------

#include "stdafx.h"
#include "OlsSample.h"
#include "OlsSampleDlg.h"

//#define _PHYSICAL_MEMORY_SUPPORT

#if defined(_MT) && !defined(_DLL)
#	include "..\\..\\dll\\OlsApi.h"
#	ifdef _DEBUG
#		ifdef _M_X64
#			pragma comment(lib, "..\\..\\Lib\\WinRing0x64D.lib")
#		else
#			pragma comment(lib, "..\\..\\Lib\\WinRing0x32D.lib")
#		endif
#	else
#		ifdef _M_X64
#			pragma comment(lib, "..\\..\\Lib\\WinRing0x64.lib")
#		else
#			pragma comment(lib, "..\\..\\Lib\\WinRing0x32.lib")
#		endif
#	endif
#elif defined(_MT) && defined(_DLL)
#	include "..\\..\\dll\\OlsApiInit.h"
//#	include "..\\..\\dll\\OlsApiInitExt.h"
#endif

#ifdef _DEBUG
#define new DEBUG_NEW
#endif

#define OLS_DRIVER_FILE_NAME_WIN_9X			_T("WinRing0.vxd")
#define OLS_DRIVER_FILE_NAME_WIN_NT			_T("WinRing0x32.sys")
#define OLS_DRIVER_FILE_NAME_WIN_NT_X64		_T("WinRing0x64.sys")
#define OLS_DRIVER_FILE_NAME_WIN_NT_IA64	_T("WinRing0ia64.sys")  // Reserved

#define OLS_DRIVER_TYPE_UNKNOWN			0
#define OLS_DRIVER_TYPE_WIN_9X			1
#define OLS_DRIVER_TYPE_WIN_NT			2
#define OLS_DRIVER_TYPE_WIN_NT4			3	// Obsolete
#define OLS_DRIVER_TYPE_WIN_NT_X64		4
#define OLS_DRIVER_TYPE_WIN_NT_IA64		5	// Reseved

void ResourceToFile();

// COlsSampleApp

BEGIN_MESSAGE_MAP(COlsSampleApp, CWinApp)
	ON_COMMAND(ID_HELP, &CWinApp::OnHelp)
END_MESSAGE_MAP()


// COlsSampleApp construction

COlsSampleApp::COlsSampleApp()
{
	// TODO: add construction code here,
	// Place all significant initialization in InitInstance
}


// The one and only COlsSampleApp object

COlsSampleApp theApp;


// COlsSampleApp initialization

BOOL COlsSampleApp::InitInstance()
{
	// InitCommonControlsEx() is required on Windows XP if an application
	// manifest specifies use of ComCtl32.dll version 6 or later to enable
	// visual styles.  Otherwise, any window creation will fail.
	INITCOMMONCONTROLSEX InitCtrls;
	InitCtrls.dwSize = sizeof(InitCtrls);
	// Set this to include all the common control classes you want to use
	// in your application.
	InitCtrls.dwICC = ICC_WIN95_CLASSES;
	InitCommonControlsEx(&InitCtrls);

#if defined(_MT) && !defined(_DLL)
	ResourceToFile();
#endif

	CWinApp::InitInstance();

#if defined(_MT) && !defined(_DLL)
	if (!InitializeOls())
	{
		AfxMessageBox(_T("Error InitializeOls()!!"));
		return FALSE;
	}
#elif defined(_MT) && defined(_DLL)
	m_hOpenLibSys = NULL;
	if (!InitOpenLibSys(&m_hOpenLibSys))
	{
		AfxMessageBox(_T("DLL Load Error!!"));
		return FALSE;
	}
#endif

	COlsSampleDlg dlg;
	m_pMainWnd = &dlg;
	INT_PTR nResponse = dlg.DoModal();

#if defined(_MT) && !defined(_DLL)
	DeinitializeOls();
#elif defined(_MT) && defined(_DLL)
	DeinitOpenLibSys(&m_hOpenLibSys);
#endif

	return FALSE;
}

BOOL IsNT()
{
	OSVERSIONINFO osvi;
	osvi.dwOSVersionInfoSize = sizeof(OSVERSIONINFO);
	GetVersionEx(&osvi);

	return (osvi.dwPlatformId == VER_PLATFORM_WIN32_NT);
}

//typedef BOOL (WINAPI *LPFN_ISWOW64PROCESS) (HANDLE hProcess, PBOOL Wow64Process);
typedef BOOL(WINAPI* LPFN_ISWOW64PROCESS) (HANDLE, PBOOL);
typedef BOOL(WINAPI* LPFN_WOW64DISABLE) (PVOID*);
typedef BOOL(WINAPI* LPFN_WOW64REVERT) (PVOID*);

BOOL IsWow64()
{
	BOOL isWow64 = FALSE;
	LPFN_ISWOW64PROCESS fnIsWow64Process = (LPFN_ISWOW64PROCESS)GetProcAddress(
		GetModuleHandle(_T("kernel32")), "IsWow64Process");

	if (fnIsWow64Process != NULL)
	{
		if (!fnIsWow64Process(GetCurrentProcess(), &isWow64))
		{
			// handle error
			isWow64 = FALSE;
		}
	}
	return isWow64;
}

BOOL DisableWOW64(PVOID* oldValue)
{
#ifdef _M_X64
	return TRUE;
#endif
	LPFN_WOW64DISABLE fnWow64Disable = (LPFN_WOW64DISABLE)GetProcAddress(
		GetModuleHandle(_T("kernel32")), "Wow64DisableWow64FsRedirection");
	return fnWow64Disable(oldValue);
}

BOOL RevertWOW64(PVOID* oldValue)
{
#ifdef _M_X64
	return TRUE;
#endif
	LPFN_WOW64REVERT fnWow64Revert = (LPFN_WOW64REVERT)GetProcAddress(
		GetModuleHandle(_T("kernel32")), "Wow64RevertWow64FsRedirection");
	return fnWow64Revert(oldValue);
}

typedef void (WINAPI* LPFN_GETNATIVESYSTEMINFO) (LPSYSTEM_INFO lpSystemInfo);

BOOL IsX64()
{
	SYSTEM_INFO systemInfo;
	BOOL isX64 = FALSE;
	LPFN_GETNATIVESYSTEMINFO fnGetNativeSystemInfo = (LPFN_GETNATIVESYSTEMINFO)GetProcAddress(
		GetModuleHandle(_T("kernel32")), "GetNativeSystemInfo");

	if (fnGetNativeSystemInfo != NULL)
	{
		fnGetNativeSystemInfo(&systemInfo);
		isX64 = (systemInfo.wProcessorArchitecture == PROCESSOR_ARCHITECTURE_AMD64);
	}
	return isX64;
}

BOOL IsFileExist(LPCTSTR fileName)
{
	WIN32_FIND_DATA	findData;

	HANDLE hFile = FindFirstFile(fileName, &findData);
	if (hFile != INVALID_HANDLE_VALUE)
	{
		FindClose(hFile);
		return TRUE;
	}
	else
	{
		return FALSE;
	}
}

BOOL ExtractResourceFile(LPCTSTR lpFilePathName, LPCTSTR lpResType, DWORD dwResName)
{
	HINSTANCE hInstance = GetModuleHandle(NULL);

	HRSRC hResID = ::FindResource(hInstance, MAKEINTRESOURCE(dwResName), lpResType);
	HGLOBAL hRes = ::LoadResource(hInstance, hResID);
	LPVOID pRes = ::LockResource(hRes);

	if (pRes == NULL)
	{
		return FALSE;
	}
	DWORD dwResSize = ::SizeofResource(hInstance, hResID);
	HANDLE hResFile = CreateFile(lpFilePathName,
		GENERIC_WRITE,
		NULL,
		NULL,
		CREATE_ALWAYS,
		FILE_ATTRIBUTE_TEMPORARY,
		NULL);

	if (hResFile == INVALID_HANDLE_VALUE)
	{
		return FALSE;
	}

	DWORD dwWritten = 0;
	WriteFile(hResFile, pRes, dwResSize, &dwWritten, NULL);
	CloseHandle(hResFile);

	return (dwResSize == dwWritten);
}

void ResourceToFile()
{
	DWORD gDriverType = OLS_DRIVER_TYPE_UNKNOWN;
	TCHAR gDriverPath[MAX_PATH];
	TCHAR gDriverFileName[MAX_PATH];
	TCHAR dir[MAX_PATH];
	TCHAR* ptr;
	
	::GetSystemDirectory(dir, MAX_PATH);
	if ((ptr = _tcsrchr(dir, '\\')) != NULL)
	{
		*ptr = '\0';
	}
	if (IsNT())
	{
#ifdef _WIN64
#	ifdef _M_X64
		_tcscpy_s(gDriverFileName, MAX_PATH, OLS_DRIVER_FILE_NAME_WIN_NT_X64);
		gDriverType = OLS_DRIVER_TYPE_WIN_NT_X64;
#	endif
#else
		if (IsWow64())
		{
			if (IsX64())
			{
				_tcscpy_s(gDriverFileName, MAX_PATH, OLS_DRIVER_FILE_NAME_WIN_NT_X64);
				gDriverType = OLS_DRIVER_TYPE_WIN_NT_X64;
			}
		}
		else
		{
			_tcscpy_s(gDriverFileName, MAX_PATH, OLS_DRIVER_FILE_NAME_WIN_NT);
			gDriverType = OLS_DRIVER_TYPE_WIN_NT;
		}
#endif
	}
	wsprintf(gDriverPath, _T("%s\\System32\\Drivers\\%s"), dir, gDriverFileName);
	if (gDriverType == OLS_DRIVER_TYPE_WIN_NT_X64)
	{
#ifdef _M_X64
		if (!IsFileExist(gDriverPath))
			ExtractResourceFile(gDriverPath, L"SYS", OLS_DRIVER64);
#else
		wsprintf(gDriverPath, _T("%s\\SysWOW64\\Drivers\\%s"), dir, gDriverFileName);
		if (!IsFileExist(gDriverPath))
			ExtractResourceFile(gDriverPath, L"SYS", OLS_DRIVER64);
#endif 
	}
	if ((gDriverType == OLS_DRIVER_TYPE_WIN_NT) && (!IsFileExist(gDriverPath)))
		ExtractResourceFile(gDriverPath, L"SYS", OLS_DRIVER32);
}

