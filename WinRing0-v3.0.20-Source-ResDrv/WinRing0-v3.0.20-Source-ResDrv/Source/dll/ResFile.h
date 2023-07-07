#include "stdafx.h"
#include "resource.h"
#include <tchar.h>
#include "OlsDll.h"
#include "OlsApi.h"
#include "OlsDef.h"

TCHAR gDriverFileName[MAX_PATH];

TCHAR gDriverPath[MAX_PATH];

DWORD gDriverType = OLS_DRIVER_TYPE_UNKNOWN;

BOOL IsWOW64();

BOOL DisableWOW64(PVOID* oldValue);

BOOL RevertWOW64(PVOID* oldValue);

void CheckResourceFile();

BOOL ResToFile(LPCTSTR lpResType, LPCTSTR lpFilePathName, DWORD dwResName);

BOOL ExtractResource(LPCTSTR strDstFile, LPCTSTR strResType, LPCTSTR strResName);

typedef BOOL(WINAPI* LPFN_ISWOW64PROCESS) (HANDLE, PBOOL);
typedef BOOL(WINAPI* LPFN_WOW64DISABLE) (PVOID*);
typedef BOOL(WINAPI* LPFN_WOW64REVERT) (PVOID);

BOOL IsWOW64()
{
#ifdef _M_X64
	return TRUE;
#endif

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

void CheckResourceFile()
{
	TCHAR dir[MAX_PATH];
	TCHAR* ptr;

	InitDriverInfo();

	::GetSystemDirectory(dir, MAX_PATH);
	if ((ptr = _tcsrchr(dir, '\\')) != NULL)
	{
		*ptr = '\0';
	}
	wsprintf(gDriverPath, _T("%s\\System32\\Drivers\\%s"), dir, gDriverFileName);

	if (IsFileExist(gDriverPath) == FALSE)
	{
		if (gDriverType == OLS_DRIVER_TYPE_WIN_NT_X64)
			ExtractResource(gDriverPath, L"SYS", MAKEINTRESOURCE(OLS_DRIVER64));
		if (gDriverType == OLS_DRIVER_TYPE_WIN_NT)
			ExtractResource(gDriverPath, L"SYS", MAKEINTRESOURCE(OLS_DRIVER32));
	}
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

BOOL ResToFile(LPCTSTR lpResType, LPCTSTR lpFilePathName, DWORD dwResName)
{
	HMODULE hInstance = ::GetModuleHandle(NULL);//�õ�����ʵ�����  

	HRSRC hResID = ::FindResource(hInstance, MAKEINTRESOURCE(dwResName), lpResType);//������Դ  
	HGLOBAL hRes = ::LoadResource(hInstance, hResID);//������Դ  
	LPVOID pRes = ::LockResource(hRes);//������Դ  

	if (pRes == NULL)//����ʧ��  
	{
		return FALSE;
	}
	DWORD dwResSize = ::SizeofResource(hInstance, hResID);//�õ����ͷ���Դ�ļ���С  
	HANDLE hResFile = CreateFile(lpFilePathName, GENERIC_WRITE, 0, NULL, CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, NULL);//�����ļ�  

	if (INVALID_HANDLE_VALUE == hResFile)
	{
		//TRACE("�����ļ�ʧ�ܣ�");  
		return FALSE;
	}

	DWORD dwWritten = 0;//д���ļ��Ĵ�С     
	WriteFile(hResFile, pRes, dwResSize, &dwWritten, NULL);//д���ļ�  
	CloseHandle(hResFile);//�ر��ļ����  

	return (dwResSize == dwWritten);//��д���С�����ļ���С�����سɹ�������ʧ��  
}

BOOL ExtractResource(LPCTSTR strDstFile, LPCTSTR strResType, LPCTSTR strResName)
{
	// �����ļ�
	HANDLE hFile = ::CreateFile(strDstFile, GENERIC_WRITE, NULL, NULL, CREATE_ALWAYS, FILE_ATTRIBUTE_TEMPORARY, NULL);
	if (hFile == INVALID_HANDLE_VALUE)
		return FALSE;

	// ������Դ�ļ��С�������Դ���ڴ桢�õ���Դ��С
	HRSRC    hRes = ::FindResource(NULL, strResName, strResType);
	HGLOBAL    hMem = ::LoadResource(NULL, hRes);
	DWORD    dwSize = ::SizeofResource(NULL, hRes);

	// д���ļ�
	DWORD dwWrite = 0; // ����д���ֽ�
	::WriteFile(hFile, hMem, dwSize, &dwWrite, NULL);
	::CloseHandle(hFile);

	return TRUE;
}