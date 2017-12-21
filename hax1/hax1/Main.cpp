#include <Windows.h>
#include <conio.h>
#include <stdio.h>
#include <iostream>
using namespace std;

HANDLE phandle;
unsigned int isingame, currentobjtype;
unsigned long long baseaddr, firstobj, nextobj, localguid, targetguid, currentguid, test;
float myx, myy, myz;

int LoadPrivilege()
{
	HANDLE hToken;
	LUID Value;
	TOKEN_PRIVILEGES tp;
	if(!OpenProcessToken(GetCurrentProcess(),TOKEN_ADJUST_PRIVILEGES|TOKEN_QUERY, &hToken))return(GetLastError());
	if(!LookupPrivilegeValue(NULL, SE_DEBUG_NAME, &Value))return(GetLastError());
	tp.PrivilegeCount = 1;
	tp.Privileges[0].Luid = Value;
	tp.Privileges[0].Attributes = SE_PRIVILEGE_ENABLED;
	if(!AdjustTokenPrivileges(hToken, FALSE,&tp, sizeof(tp), NULL, NULL))return(GetLastError());
	CloseHandle(hToken);
	return 1;
}

int main()
{
	LoadPrivilege();
	DWORD pid;
	HWND hwnd;
	hwnd = FindWindow(NULL,"World of Warcraft");
	if(!hwnd)printf("Window not found!\n");
	GetWindowThreadProcessId(hwnd, &pid);
	phandle = OpenProcess(PROCESS_VM_READ, 0, pid);
	cout << pid << endl;
	if(phandle)
	{
		ReadProcessMemory(phandle, (void*)0xB4B424, &isingame, sizeof(isingame), 0);
		if(isingame == 1)printf("You are logged in!\n");
	}
	ReadProcessMemory(phandle, (void*)(0xB41414), &baseaddr, sizeof(baseaddr), 0);
	ReadProcessMemory(phandle, (void*)(baseaddr + 0xAC), &firstobj, sizeof(firstobj), 0);
	//printf("firstobj: %llu\n", firstobj);
	ReadProcessMemory(phandle, (void*)(baseaddr + 0xC0), &localguid, sizeof(localguid), 0);
	//printf("LOCALGUID: %llu\n", localguid);
	ReadProcessMemory(phandle, (void*)(0xB4E2D8), &targetguid, sizeof(targetguid), 0);
	printf("TARGETGUID: %llu\n", targetguid);
	if(targetguid == localguid)
	{
		printf("I AM TARGETING MYSELF LOL!\n");
	}
	nextobj = firstobj;
	while(firstobj != 0 && (firstobj & 1) == 0)
	{
		ReadProcessMemory(phandle, (void*)(firstobj + 0x14), &currentobjtype, sizeof(currentobjtype), 0);
		printf("currentobjtypenumber: %u\n", currentobjtype);
		ReadProcessMemory(phandle, (void*)(firstobj + 0x30), &currentguid, sizeof(currentguid), 0);
		if(currentguid == localguid)
		{
			//printf("HELLO WORLD!\n");
			printf("MY GUID: %llu\n", currentguid);
			ReadProcessMemory(phandle, (void*)(firstobj + 0x9B8), &myx, sizeof(myx), 0);
			ReadProcessMemory(phandle, (void*)(firstobj + 0x9BC), &myy, sizeof(myy), 0);
			ReadProcessMemory(phandle, (void*)(firstobj + 0x9C0), &myz, sizeof(myz), 0);
			printf("MY LOCATION: X: %f, Y: %f, Z: %f\n", myx, myy, myz);
		}
		ReadProcessMemory(phandle, (void*)(firstobj + 0x3C), &nextobj, sizeof(nextobj), 0);
		if(nextobj == firstobj)
		{
			printf("BREAK!\n");
			break;
		}
		else
		{
			firstobj = nextobj;
		}
	}
	_getch();
	return 0;
}

/*
private enum ObjTypes : byte
{
	OT_NONE = 0,
	OT_ITEM = 1,
	OT_CONTAINER = 2,
	OT_UNIT = 3,
	OT_PLAYER = 4,
	OT_GAMEOBJ = 5,
	OT_DYNOBJ = 6,
	OT_CORPSE = 7,
}
*/