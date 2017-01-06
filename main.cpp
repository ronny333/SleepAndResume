#include <windows.h>
#include <stdio.h>
#include <PowrProf.h>
#include <tchar.h>
#pragma comment(lib, "PowrProf.lib")
#define OPERATION_SLEEP TRUE
#define OPERATION_HIBERNATE FALSE
BOOL
WINAPI
_EnableShutDownPriv()
{
	BOOL ret = FALSE;
	HANDLE process = GetCurrentProcess();
	HANDLE token;

	if (!OpenProcessToken(process
		, TOKEN_ADJUST_PRIVILEGES | TOKEN_QUERY
		, &token))
		return FALSE;

	LUID luid;
	LookupPrivilegeValue(NULL, _T("SeShutdownPrivilege"), &luid);

	TOKEN_PRIVILEGES priv;
	priv.PrivilegeCount = 1;
	priv.Privileges[0].Attributes = SE_PRIVILEGE_ENABLED;
	priv.Privileges[0].Luid = luid;
	ret = AdjustTokenPrivileges(token, FALSE, &priv, 0, NULL, NULL);
	CloseHandle(token);

	return TRUE;
}


DWORD
CALLBACK
ShutDownProc(LPVOID p)
{
	PHANDLE timer = (PHANDLE)p;

	WaitForSingleObject(*timer, INFINITE);
	CloseHandle(*timer);
	SetThreadExecutionState(ES_DISPLAY_REQUIRED);

	return 0;
}
BOOL
WINAPI
HibernateAndReboot(int secs, int Operation)
{
	BOOL bState = FALSE;
	if (!_EnableShutDownPriv())
		return FALSE;

	HANDLE timer = CreateWaitableTimer(NULL, TRUE, _T("MyWaitableTimer"));
	if (timer == NULL)
		return FALSE;

	__int64 nanoSecs;
	LARGE_INTEGER due;

	nanoSecs = -secs * 1000 * 1000 * 10;
	due.LowPart = (DWORD)(nanoSecs & 0xFFFFFFFF);
	due.HighPart = (LONG)(nanoSecs >> 32);

	if (!SetWaitableTimer(timer, &due, 0, 0, 0, TRUE))
		return FALSE;

	if (GetLastError() == ERROR_NOT_SUPPORTED)
		return FALSE;

	HANDLE thread = CreateThread(NULL, 0, ShutDownProc, &timer, 0, NULL);
	if (!thread)
	{
		CloseHandle(timer);
		return FALSE;
	}

	CloseHandle(thread);
	if (OPERATION_SLEEP == Operation)
	{
		bState = TRUE;
	}
	else
	{
		bState = FALSE;
	}
	if (0==SetSystemPowerState(bState, FALSE))
	{
		return FALSE;
	}
	return TRUE;
}
int main(int argc,TCHAR **argv)
{
	if (argc < 2)
	{
		_tprintf(_T("Invalid argument\nHelp:\n\tFor system SLEEP    : SleepResume.exe -s\n\tFor system HIBERNATE: SleepResume.exe -h\n"));
		return 0;
	}
	if (_tcscmp(argv[1],"-s") == 0)
	{
		HibernateAndReboot(1, OPERATION_SLEEP);
	}
	else if (_tcscmp(argv[1], "-h") == 0)
	{
		HibernateAndReboot(10,OPERATION_HIBERNATE);
	}
	else
	{
		printf("Invalid input\n");
	}
	return 0;
}