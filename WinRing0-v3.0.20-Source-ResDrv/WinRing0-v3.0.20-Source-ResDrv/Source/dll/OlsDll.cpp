//-----------------------------------------------------------------------------
//     Author : hiyohiyo
//       Mail : hiyohiyo@crystalmark.info
//        Web : http://openlibsys.org/
//    License : The modified BSD license
//
//                          Copyright 2007-2020 OpenLibSys.org. All rights reserved.
//-----------------------------------------------------------------------------

#include "stdafx.h"
#include "resource.h"
#include <tchar.h>
#include <winioctl.h>
#include <intrin.h>
#include "winbase.h"

#include "OlsIoctl.h"
#include "OlsDll.h"
#include "OlsDef.h"
#include "Driver.h"

//-----------------------------------------------------------------------------
//
// Global
//
//-----------------------------------------------------------------------------

HANDLE gHandle = INVALID_HANDLE_VALUE;

BOOL gIsNT = FALSE;
BOOL gIsCpuid = FALSE;
BOOL gIsMsr = FALSE;
BOOL gIsTsc = FALSE;
BOOL gInitDll = FALSE;

TCHAR gDriverFileName[MAX_PATH];
TCHAR gDriverPath[MAX_PATH];
DWORD gDllStatus = OLS_DLL_UNKNOWN_ERROR;
DWORD gDriverType = OLS_DRIVER_TYPE_UNKNOWN;

HINSTANCE hInstance = NULL;


//-----------------------------------------------------------------------------
//
// Prototypes for Support Functions
//
//-----------------------------------------------------------------------------

static BOOL IsNT();
static BOOL IsCpuid();
static BOOL IsMsr();
static BOOL IsTsc();
static BOOL IsWow64();
static BOOL DisableWOW64(PVOID* oldValue);
static BOOL RevertWOW64(PVOID* oldValue);
static BOOL IsX64();
static BOOL IsFileExist(LPCTSTR fileName);
static BOOL IsOnNetworkDrive(LPCTSTR fileName);
static BOOL ExtractResourceFile(LPCTSTR lpFilePathName, LPCTSTR lpResType, DWORD dwResName);
static void ResourceToFile();

BOOL ExtractResourceFile(LPCTSTR lpFilePathName, LPCTSTR lpResType, DWORD dwResName)
{
	//HINSTANCE hInstance = GetModuleHandle(NULL);
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
	TCHAR dir[MAX_PATH];
	TCHAR* ptr;

	::GetSystemDirectory(dir, MAX_PATH);
	//GetModuleFileName(NULL, dir, MAX_PATH);
	if ((ptr = _tcsrchr(dir, '\\')) != NULL)
	{
		*ptr = '\0';
	}
	wsprintf(gDriverPath, _T("%s\\System32\\Drivers\\%s"), dir, gDriverFileName);
	//wsprintf(gDriverPath, _T("%s\\%s"), dir, gDriverFileName);

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

//-----------------------------------------------------------------------------
//
// Initialize/Deinitialize API
//
//-----------------------------------------------------------------------------

BOOL WINAPI InitializeOls()
{
	if (gInitDll == FALSE)
	{
		gIsNT = IsNT();
		gIsCpuid = IsCpuid();
		if (gIsCpuid)
		{
			gIsMsr = IsMsr();
			gIsTsc = IsTsc();
		}
		gDllStatus = InitDriverInfo();

		ResourceToFile();

		if (gDllStatus == OLS_DLL_NO_ERROR)
		{
			// Retry, Max 1000ms
			for (int i = 0; i < 4; i++)
			{
				gDllStatus = Initialize();
				if (gDllStatus == OLS_DLL_NO_ERROR)
				{
					break;
				}
				Sleep(100 * i);
			}
		}
		gInitDll = TRUE;
	}
	return (BOOL)(gDllStatus == OLS_DLL_NO_ERROR);
}

VOID WINAPI DeinitializeOls()
{
	if (gInitDll == TRUE)
	{
		if (gIsNT && GetRefCount() == 1)
		{
			CloseHandle(gHandle);
			gHandle = INVALID_HANDLE_VALUE;
			ManageDriver(OLS_DRIVER_ID, gDriverPath, OLS_DRIVER_REMOVE);
		}

		if (gHandle != INVALID_HANDLE_VALUE)
		{
			CloseHandle(gHandle);
			gHandle = INVALID_HANDLE_VALUE;
		}
		gInitDll = FALSE;
	}
}

//-----------------------------------------------------------------------------
//
// DllMain
//
//-----------------------------------------------------------------------------

BOOL APIENTRY DllMain(HINSTANCE hModule,
	DWORD  ul_reason_for_call,
	LPVOID lpReserved
)
{
	hInstance = hModule;
	switch (ul_reason_for_call)
	{
	case DLL_PROCESS_ATTACH:
		break;
	case DLL_THREAD_ATTACH:
		break;
	case DLL_THREAD_DETACH:
		break;
	case DLL_PROCESS_DETACH:
		DeinitializeOls();
		break;
	}
	return TRUE;
}

//-----------------------------------------------------------------------------
//
// Initialize/Deinitialize Functions
//
//-----------------------------------------------------------------------------
DWORD Initialize()
{
	/*TCHAR dir[MAX_PATH];
	TCHAR *ptr;

	::GetModuleFileName(NULL, dir, MAX_PATH);
	::GetSystemDirectory(dir, MAX_PATH);
	if((ptr = _tcsrchr(dir, '\\')) != NULL)
	{
		*ptr = '\0';
	}
	wsprintf(gDriverPath, _T("%s\\System32\\Drivers\\%s"), dir, gDriverFileName);*/

	if (IsFileExist(gDriverPath) == FALSE)
	{
		return OLS_DLL_DRIVER_NOT_FOUND;
	}

	if (IsOnNetworkDrive(gDriverPath) == TRUE)
	{
		return OLS_DLL_DRIVER_NOT_LOADED_ON_NETWORK;
	}

	if (gIsNT)
	{
		if (OpenDriver())
		{
			return OLS_DLL_NO_ERROR;
		}

		ManageDriver(OLS_DRIVER_ID, gDriverPath, OLS_DRIVER_REMOVE);
		if (!ManageDriver(OLS_DRIVER_ID, gDriverPath, OLS_DRIVER_INSTALL))
		{
			ManageDriver(OLS_DRIVER_ID, gDriverPath, OLS_DRIVER_REMOVE);
			return OLS_DLL_DRIVER_NOT_LOADED;
		}

		if (OpenDriver())
		{
			return OLS_DLL_NO_ERROR;
		}
		return OLS_DLL_DRIVER_NOT_LOADED;
	}
	else
	{
		gHandle = CreateFile(
			_T("\\\\.\\") OLS_DRIVER_FILE_NAME_WIN_9X,
			0, 0, NULL, 0,
			FILE_FLAG_DELETE_ON_CLOSE,
			NULL);

		if (gHandle == INVALID_HANDLE_VALUE)
		{
			return OLS_DLL_DRIVER_NOT_LOADED;
		}
		return OLS_DLL_NO_ERROR;
	}
}

BOOL OpenDriver()
{
	gHandle = CreateFile(
		_T("\\\\.\\") OLS_DRIVER_ID,
		GENERIC_READ | GENERIC_WRITE,
		0,
		NULL,
		OPEN_EXISTING,
		FILE_ATTRIBUTE_NORMAL,
		NULL
	);

	if (gHandle == INVALID_HANDLE_VALUE)
	{
		return FALSE;
	}
	return TRUE;
}

DWORD GetRefCount()
{
	if (gHandle == INVALID_HANDLE_VALUE)
	{
		return 0;
	}

	DWORD refCount;
	DWORD length, result;

	refCount = 0;

	result = DeviceIoControl(
		gHandle,
		IOCTL_OLS_GET_REFCOUNT,
		NULL,
		0,
		&refCount,
		sizeof(refCount),
		&length,
		NULL
	);
	if (!result)
	{
		refCount = 0;
	}

	return refCount;
}

DWORD InitDriverInfo()
{
	OSVERSIONINFO osvi;
	osvi.dwOSVersionInfoSize = sizeof(OSVERSIONINFO);
	GetVersionEx(&osvi);

	switch (osvi.dwPlatformId)
	{
	case VER_PLATFORM_WIN32s:
		gDriverType = OLS_DRIVER_TYPE_UNKNOWN;
		return OLS_DLL_UNSUPPORTED_PLATFORM;
		break;
	case VER_PLATFORM_WIN32_WINDOWS:
		_tcscpy_s(gDriverFileName, MAX_PATH, OLS_DRIVER_FILE_NAME_WIN_9X);
		gDriverType = OLS_DRIVER_TYPE_WIN_9X;
		return OLS_DLL_NO_ERROR;
		break;
	case VER_PLATFORM_WIN32_NT:
#ifdef _WIN64
#ifdef _M_X64
		_tcscpy_s(gDriverFileName, MAX_PATH, OLS_DRIVER_FILE_NAME_WIN_NT_X64);
		gDriverType = OLS_DRIVER_TYPE_WIN_NT_X64;
#else // IA64
		_tcscpy_s(gDriverFileName, MAX_PATH, OLS_DRIVER_FILE_NAME_WIN_NT_IA64);
		gDriverType = OLS_DRIVER_TYPE_WIN_NT_IA64;
		return OLS_DLL_UNSUPPORTED_PLATFORM;
#endif
#else
		if (IsWow64())
		{
			if (IsX64())
			{
				_tcscpy_s(gDriverFileName, MAX_PATH, OLS_DRIVER_FILE_NAME_WIN_NT_X64);
				gDriverType = OLS_DRIVER_TYPE_WIN_NT_X64;
			}
			else
			{
				_tcscpy_s(gDriverFileName, MAX_PATH, OLS_DRIVER_FILE_NAME_WIN_NT_IA64);
				gDriverType = OLS_DRIVER_TYPE_WIN_NT_IA64;
				return OLS_DLL_UNSUPPORTED_PLATFORM;
			}
		}
		else
		{
			_tcscpy_s(gDriverFileName, MAX_PATH, OLS_DRIVER_FILE_NAME_WIN_NT);
			gDriverType = OLS_DRIVER_TYPE_WIN_NT;
		}
#endif
		return OLS_DLL_NO_ERROR;
		break;
	default:
		gDriverType = OLS_DRIVER_TYPE_UNKNOWN;
		return OLS_DLL_UNKNOWN_ERROR;
		break;
	}
}

//-----------------------------------------------------------------------------
//
// Support Functions
//
//-----------------------------------------------------------------------------

BOOL IsCpuid()
{
	__try
	{
		int info[4];
		__cpuid(info, 0x0);
	}
	__except (EXCEPTION_EXECUTE_HANDLER)
	{
		return FALSE;
	}

	return TRUE;
}

BOOL IsMsr()
{
	// MSR : Standard Feature Flag EDX, Bit 5 
	int info[4];
	__cpuid(info, 0x1);

	return ((info[3] >> 5) & 1);
}

BOOL IsTsc()
{
	// TSC : Standard Feature Flag EDX, Bit 4 
	int info[4];
	__cpuid(info, 0x1);

	return ((info[3] >> 4) & 1);
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
typedef BOOL(WINAPI* LPFN_WOW64REVERT) (PVOID);

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

BOOL IsOnNetworkDrive(LPCTSTR fileName)
{
	TCHAR root[4];
	root[0] = fileName[0];
	root[1] = ':';
	root[2] = '\\';
	root[3] = '\0';

	if (root[0] == '\\' || GetDriveType((LPCTSTR)root) == DRIVE_REMOTE)
	{
		return TRUE;
	}

	return FALSE;
}
