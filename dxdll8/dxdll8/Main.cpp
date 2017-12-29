#pragma comment(lib, "d3d9.lib")
#pragma comment(lib, "d3dx9.lib")
#pragma warning(disable:4996 4244)
#pragma warning(disable:4700)
#include <windows.h>
#include <mmsystem.h>
#include <stdio.h>
#include <fstream>
#include <d3d9.h>
#include <d3dx9.h>
#include <time.h>
bool hooked = false;
//--------------------------------define HOOK & UNHOOK--------------------------------------------------------------------------------------------------
#define HOOK(func,addy) o##func = (t##func)CreateDetour((DWORD)hk##func,(DWORD)addy,Detour_Type_0xB8,7)
#define UNHOOK(func,addy) o##func = (t##func)CreateDetour((DWORD)o##func,(DWORD)addy,Detour_Type_0xB8,7)
#define ES 0
#define DIP 1
#define RES 2
#define E8 2
#define ZE 3
#define DrawIndex 0x002B
#define EndS 0x0023
#define PI 3.14159265
using namespace std;
ofstream infile;
ofstream myfile;
ID3DXLine*g_pLine;
LPD3DXFONT g_pFont = NULL;
LPD3DXFONT g_pFont2 = NULL;
D3DVIEWPORT9 g_ViewPort;
DWORD dwProcessID;
HANDLE hProcess;
LPDIRECT3DDEVICE9 npDevice;
D3DXVECTOR3 pHeader;
HMODULE dwD3D9 = GetModuleHandle("d3d9.dll");
DWORD BaseD3D = NULL;
//---------------------------------------------------------------------------------------------------------------------------------- 
int Mpos = 0; //current highlighted menuitem	
int Mmax = 0; //number of menu items
int	Mxofs = 100; //offset for option text
int	Mysize = 15; //heigh of a menuline
int	Mvisible = 0; //0,1 = 0 off / 1 = show
//-----------------------------------Colors-----------------------------------------------------------------------------------------------
bool Generate = true;
#define GREY	D3DCOLOR_ARGB(255, 128, 128, 128)
#define WHITE	D3DCOLOR_ARGB(255, 255, 255, 255)
#define RED		D3DCOLOR_ARGB(255, 255, 000, 000)
#define GREEN	D3DCOLOR_ARGB(255, 000, 255, 000)
#define YELLOW	D3DCOLOR_ARGB(255, 255, 255, 000)
LPDIRECT3DTEXTURE9 White, Red, Green, Yellow;
//----------------------------------------------------------------------------------------------------------------------------------
LPDIRECT3DVERTEXBUFFER9 Stream_Data;
UINT Offset = 0;
UINT Stride = 0;
UINT texnum = 0;
//----------------------------------------------------------------------------------------------------------------------------------
RECT rect;
RECT rect2;
RECT rect3;
//-----------------------------------------------CreateDetour-----------------------------------------------------------------------------------
#define Detour_Type_0xE9 1
#define Detour_Type_0xB8 2
#define Detour_Type_0x68 3
DWORD CreateDetour(DWORD dwThread,DWORD dwAdress,DWORD dwType,DWORD dwSize)
{
	DWORD dwDetour,dwProtect,i;
	if (dwAdress&&dwThread&&dwSize>= dwSize)
	{
		dwDetour = (DWORD)VirtualAlloc(0,dwSize+dwSize,0x1000,0x40);
		if (dwDetour&&VirtualProtect((VOID*)dwAdress,dwSize,0x40,&dwProtect))
		{
			for (i=0;i<dwSize;i++) 
			{
				*(BYTE*)(dwDetour+i)=*(BYTE*)(dwAdress+i);
			  }
			       switch (dwType)
			     {
			         case Detour_Type_0xE9:
				     {
				        *(BYTE*)(dwDetour+dwSize+0)=0xE9;
				           *(DWORD*)(dwDetour+dwSize+1)=(dwAdress-dwDetour-dwSize);
	    		             *(BYTE*)(dwAdress+0)=0xE9;
				                *(DWORD*)(dwAdress+1)=(dwThread-dwAdress-dwSize);
				                     }
				break;
				   case Detour_Type_0xB8:
				      {
				        *(BYTE*)(dwDetour+dwSize+0)=0xB8;
				          *(DWORD*)(dwDetour+dwSize+1)=(dwAdress+dwSize);
				           *(WORD*)(dwDetour+dwSize+5)=0xE0FF; 
				             *(BYTE*)(dwAdress+0)=0xB8;            
				               *(DWORD*)(dwAdress+1)=(dwThread);
				                 *(WORD*)(dwAdress+5)=0xE0FF; 
				                  }
				break;
				   case Detour_Type_0x68:
				     {
				       *(BYTE*)(dwDetour+dwSize+0)=0x68;
				         *(DWORD*)(dwDetour+dwSize+1)=(dwAdress+dwSize);
				           *(WORD*)(dwDetour+dwSize+5)=0xC3; 
				             *(BYTE*)(dwAdress+0)=0x68;            
				               *(DWORD*)(dwAdress+1)=(dwThread);
				                 *(WORD*)(dwAdress+5)=0xC3; 
				                   }
				break;
			        }
			         VirtualProtect((VOID*)dwAdress,dwSize,dwProtect,&dwProtect);
			          VirtualProtect((VOID*)dwDetour,dwSize+dwSize,0x20,&dwProtect);
	    	           return dwDetour;
		                }
	                     }
	                      Sleep(10);
return (0);
}
//-----------------------------------------HRESULT-----------------------------------------------------------------------------------------
typedef HRESULT(WINAPI*tEndScene)(LPDIRECT3DDEVICE9 pDevice);
tEndScene oEndScene = NULL;

typedef HRESULT(WINAPI*tDrawIndexedPrimitive)(LPDIRECT3DDEVICE9 pDevice, D3DPRIMITIVETYPE PrimType, INT BaseVertexIndex, UINT MinVertexIndex, UINT NumVertices, UINT startIndex, UINT primCount);
tDrawIndexedPrimitive oDrawIndexedPrimitive = NULL;

typedef HRESULT(WINAPI* tReset)(LPDIRECT3DDEVICE9 pDevice, D3DPRESENT_PARAMETERS* pPresentationParameters);
tReset oReset = NULL;
//----------------------------------------PrintText---------------------------------------------------------------------------------------
void PrintText(char pString[], int x, int y, D3DCOLOR col, ID3DXFont*font)
{
	RECT FontRect = { x, y, x+500, y+30 };
	font->DrawText(NULL, pString, -1, &FontRect, DT_LEFT|DT_WORDBREAK, col);
}
//-----------------------------------------FillRGB---------------------------------------------------------------------------------------
void FillRGB( int x, int y, int w, int h, D3DCOLOR color, IDirect3DDevice9* pDevice )
{
	D3DRECT rec = { x, y, x + w, y + h };
	pDevice->Clear( 1, &rec, D3DCLEAR_TARGET, color, 0, 0 );
}
//----------------------------------------DrawCircle-----------------------------------------------------------------------------------------
void DrawCircle(int X, int Y, int radius, int numSides, DWORD Color) 
{
	D3DXVECTOR2 Line[128];
	float Step = PI * 2.0 / numSides;
	int Count = 0;
	for(float a = 0; a < PI*2.0; a += Step)
	{
		float X1 = radius * cos(a) + X;
		float Y1 = radius * sin(a) + Y;
		float X2 = radius * cos(a+Step) + X;
		float Y2 = radius * sin(a+Step) + Y;
		Line[Count].x = X1;
		Line[Count].y = Y1;
		Line[Count+1].x = X2;
		Line[Count+1].y = Y2;
		Count += 2;
	}
	g_pLine->Begin();
	g_pLine->Draw(Line,Count,Color);
	g_pLine->End(); 
}

//-------------------------------VTableFunction---------------------------------------------------------------------------------------------------
PBYTE HookVTableFunction( PDWORD* dwVTable, PBYTE dwHook, INT Index )
{
    DWORD dwOld = 0;
    VirtualProtect((void*)((*dwVTable) + (Index) ), 0, PAGE_EXECUTE_READWRITE, &dwOld);
    PBYTE pOrig = ((PBYTE)(*dwVTable)[Index]);
    (*dwVTable)[Index] = (DWORD)dwHook;
    VirtualProtect((void*)((*dwVTable) + (Index)), 0, dwOld, &dwOld);
    return pOrig;
}
//----------------------------------ProtectHook------------------------------------------------------------------------------------------------
PBYTE ProtectHook( PDWORD* dwVTable, PBYTE dwHook, INT Index )
{
    DWORD d = 0;
     DWORD ds = 0;
      VirtualProtect((PVOID*)((*dwVTable) + (Index*4)), 4, PAGE_EXECUTE_READWRITE, &d);
       Sleep(-0);
        PBYTE pOrig = ((PBYTE)(*dwVTable)[Index]);
         (*dwVTable)[Index] = (DWORD)dwHook;
           memcpy(pOrig, dwVTable, ds);
          Sleep(-0);
         VirtualProtect((PVOID*)((*dwVTable) + (Index*4)), 4, d, &ds);
        Sleep(-0);
       VirtualProtect((void*)(dwVTable), 4, PAGE_EXECUTE_READWRITE, &d);
      memcpy(pOrig,(void*)(pOrig), 4);
    VirtualProtect((void*)(dwHook), 4, d, &ds);
return pOrig;
}
//----------------------------------DrawString------------------------------------------------------------------------------------------------
void DrawString(int x, int y, DWORD color, const char *fmt, ...)
{
	RECT FontPos = { x, y, x + 30, y + 20 };
	char buf[1024] = {'\0'};
	va_list va_alist;

	va_start(va_alist, fmt);
	vsprintf(buf, fmt, va_alist);
	va_end(va_alist);
	
	g_pFont2->DrawText(NULL, buf, -1, &FontPos, DT_NOCLIP, color);
}
//------------------------------------Show timeinfo----------------------------------------------------------------------------------------------
void Time(int x,int y,D3DCOLOR color)
{
   static float TimeElapsed = 0;
   static char FinalString[MAX_PATH];
   static time_t TimeValue;
   static tm* timeinfo;

   RECT FontPos = { x, y, x + 200, y + 16 };

   time ( &TimeValue );
   timeinfo = localtime ( &TimeValue );
   if(timeinfo->tm_hour>12)
   sprintf(FinalString, " [Time : %d:%02d:%02d PM] ",timeinfo->tm_hour - 12,timeinfo->tm_min,timeinfo->tm_sec);
   else
   sprintf(FinalString, " [Time : %d:%02d:%02d AM] ",timeinfo->tm_hour, timeinfo->tm_min,timeinfo->tm_sec);
   g_pFont->DrawTextA(NULL, FinalString, -1, &FontPos, DT_NOCLIP, color); 
}
//-------------------------------------current time---------------------------------------------------------------------------------------------
void currenttime(int x,int y,D3DCOLOR color)
{
	static char cdate[20] = "" ;
	struct tm * current_tm;

	RECT FontPos = { x, y, x + 200, y + 16 };

	time_t current_time;
	time (&current_time);
	current_tm = localtime (&current_time);
	sprintf( cdate, " [Date : %d-%d-%02d] ",current_tm->tm_mon+1,current_tm->tm_mday,current_tm->tm_year-100+2000);
	g_pFont->DrawTextA(NULL, cdate, -1, &FontPos, DT_NOCLIP, color); 
}
//-------------------------------------EndScene---------------------------------------------------------------------------------------------
HANDLE phandle;
unsigned int isingame, currentobjtype, myhp, mymana;
unsigned long long baseaddr, firstobj, nextobj, localguid, targetguid, currentguid, test;
float myx, myy, myz, myfacing, myspeed, myspeedrunmodifier;

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
void objmanager()
{
	DWORD pid;
	HWND hwnd;
	hwnd = FindWindow(NULL,"World of Warcraft");
	//if(!hwnd)printf("Window not found!\n");
	GetWindowThreadProcessId(hwnd, &pid);
	phandle = OpenProcess(PROCESS_VM_READ, 0, pid);
	//cout << pid << endl;
	if(phandle)
	{
		ReadProcessMemory(phandle, (void*)0xB4B424, &isingame, sizeof(isingame), 0);
		//if(isingame == 1)printf("You are logged in!\n");
	}
	ReadProcessMemory(phandle, (void*)(0xB41414), &baseaddr, sizeof(baseaddr), 0);
	ReadProcessMemory(phandle, (void*)(baseaddr + 0xAC), &firstobj, sizeof(firstobj), 0);
	//printf("firstobj: %llu\n", firstobj);
	ReadProcessMemory(phandle, (void*)(baseaddr + 0xC0), &localguid, sizeof(localguid), 0);
	//printf("LOCALGUID: %llu\n", localguid);
	ReadProcessMemory(phandle, (void*)(0xB4E2D8), &targetguid, sizeof(targetguid), 0);
	//printf("TARGETGUID: %llu\n", targetguid);
	/*
	if(targetguid == localguid)
	{
		printf("I AM TARGETING MYSELF LOL!\n");
	}
	*/
	nextobj = firstobj;
	while(firstobj != 0 && (firstobj & 1) == 0)
	{
		ReadProcessMemory(phandle, (void*)(firstobj + 0x14), &currentobjtype, sizeof(currentobjtype), 0);
		//printf("currentobjtypenumber: %u\n", currentobjtype);
		ReadProcessMemory(phandle, (void*)(firstobj + 0x30), &currentguid, sizeof(currentguid), 0);
		if(currentguid == localguid)
		{
			//printf("MY GUID: %llu\n", currentguid);
			ReadProcessMemory(phandle, (void*)(firstobj + 0x9B8), &myx, sizeof(myx), 0);
			ReadProcessMemory(phandle, (void*)(firstobj + 0x9BC), &myy, sizeof(myy), 0);
			ReadProcessMemory(phandle, (void*)(firstobj + 0x9C0), &myz, sizeof(myz), 0);
			//printf("MY LOCATION: X: %f, Y: %f, Z: %f\n", myx, myy, myz);

			ReadProcessMemory(phandle, (void*)(firstobj + 0x1DC8), &myhp, sizeof(myhp), 0);
			ReadProcessMemory(phandle, (void*)(firstobj + 0x1DCC), &mymana, sizeof(mymana), 0);

			ReadProcessMemory(phandle, (void*)(firstobj + 0xA2C), &myspeed, sizeof(myspeed), 0);
			ReadProcessMemory(phandle, (void*)(firstobj + 0xA34), &myspeedrunmodifier, sizeof(myspeedrunmodifier), 0);
			//printf("RUNSPEED: %f RUNSPEEDMODIFIER: %f\n", myspeed, myspeedrunmodifier);
		}
		ReadProcessMemory(phandle, (void*)(firstobj + 0x3C), &nextobj, sizeof(nextobj), 0);
		if(nextobj == firstobj)
		{
			//printf("BREAK!\n");
			break;
		}
		else
		{
			firstobj = nextobj;
		}
	}
}
void convertfloatchar(char*ch, float fl)
{
	sprintf(ch, "%f", fl);
}
HRESULT WINAPI hkEndScene(LPDIRECT3DDEVICE9 pDevice)
{
	myfile << "EndScene is Hook\n";
	__asm PUSHAD;
	while(!npDevice)
	{
		npDevice = pDevice;
	}
	//-----------------------------------------Sett your menu here-----------------------------------------------------------------------------------------
	if(g_pFont == NULL) D3DXCreateFont(pDevice, 15, 0, FW_BOLD, 1, 0, DEFAULT_CHARSET, OUT_DEFAULT_PRECIS, ANTIALIASED_QUALITY, DEFAULT_PITCH | FF_DONTCARE, "Arial", &g_pFont); //Create fonts
	if(g_pFont2 == NULL) D3DXCreateFont(pDevice, 15, 0, FW_BOLD, 1, 0, DEFAULT_CHARSET, OUT_DEFAULT_PRECIS, ANTIALIASED_QUALITY, DEFAULT_PITCH | FF_DONTCARE, "Arial", &g_pFont2); //Create fonts
	if(g_pLine == NULL) D3DXCreateLine(pDevice, &g_pLine);
	
	pDevice->GetViewport(&g_ViewPort);
	pDevice->GetViewport(&g_ViewPort);
	
	DrawString(0, 1, GREEN, "Our first D3D9 endscene hook!");
	currenttime(0,15,0xFFFF0000); // x,y = width and height
	Time(0, 29, 0xFFFF0000);      // x,y = width and height
	int test = 3;
	if(test == 24)
	{
		test = 3;
	}
	else
	{
		DrawCircle(200, 200, 200, test, WHITE); //This is a test
	}
	test++;

	objmanager();
	if(isingame == 1)
	{
		DrawString(300, 1, GREEN, "You are logged in!");
		char myxarray[64], myyarray[64], myzarray[64];
		convertfloatchar(myxarray, myx),
			convertfloatchar(myyarray, myy),
			convertfloatchar(myzarray, myz);
		DrawString(300, 15, GREEN, myxarray);
		DrawString(300, 29, GREEN, myyarray);
		DrawString(300, 43, GREEN, myzarray);
	}

	__asm POPAD;
	return oEndScene(pDevice);
}
//---------------------------------------------DrawIndexedPrimitive---------------------------------------------------------------------------------------
HRESULT WINAPI hkDrawIndexedPrimitive(LPDIRECT3DDEVICE9 pDevice, D3DPRIMITIVETYPE PrimType,INT BaseVertexIndex,UINT MinVertexIndex,UINT NumVertices,UINT startIndex,UINT primCount)
{
	myfile << "DIP is hooked\n";
	if(pDevice->GetStreamSource(0, &Stream_Data, &Offset, &Stride) == D3D_OK)Stream_Data->Release();
	return oDrawIndexedPrimitive(pDevice, PrimType, BaseVertexIndex, MinVertexIndex, NumVertices, startIndex, primCount); // NO FOG 
}
//-----------------------------------------Reset is hooked-----------------------------------------------------------------------------------------
HRESULT WINAPI hkReset(LPDIRECT3DDEVICE9 pDevice, D3DPRESENT_PARAMETERS*pPresentationParameters)
{
	myfile << "Reset is hooked\n";
	if(g_pFont)g_pFont->OnLostDevice();
    if(g_pLine)g_pLine->OnLostDevice();
	HRESULT iReturnValue = oReset(pDevice, pPresentationParameters);
	if(iReturnValue == D3D_OK)
	{
		if(g_pFont)g_pFont->OnResetDevice();
		if(g_pLine)g_pLine->OnResetDevice();
	}
	return iReturnValue;
}
//-------------------------------------------CreateWindow---------------------------------------------------------------------------------------
LRESULT CALLBACK MsgProc(HWND hwnd,UINT uMsg,WPARAM wParam,LPARAM lParam){return DefWindowProc(hwnd, uMsg, wParam, lParam);}
void DX_Init(DWORD*table)
{
    WNDCLASSEX wc = {sizeof(WNDCLASSEX),CS_CLASSDC,MsgProc,0L,0L,GetModuleHandle(NULL),NULL,NULL,NULL,NULL,"DX",NULL};
    RegisterClassEx(&wc);
    HWND hWnd = CreateWindow("DX",NULL,WS_OVERLAPPEDWINDOW,100,100,300,300,GetDesktopWindow(),NULL,wc.hInstance,NULL);
    LPDIRECT3D9 pD3D = Direct3DCreate9( D3D_SDK_VERSION );
    D3DPRESENT_PARAMETERS d3dpp;
    ZeroMemory( &d3dpp, sizeof(d3dpp) );
    d3dpp.Windowed = TRUE;
    d3dpp.SwapEffect = D3DSWAPEFFECT_DISCARD;
    d3dpp.BackBufferFormat = D3DFMT_UNKNOWN;
    LPDIRECT3DDEVICE9 pd3dDevice;
    pD3D->CreateDevice(D3DADAPTER_DEFAULT,D3DDEVTYPE_HAL,hWnd,D3DCREATE_SOFTWARE_VERTEXPROCESSING,&d3dpp,&pd3dDevice);
    DWORD* pVTable = (DWORD*)pd3dDevice;
    pVTable = (DWORD*)pVTable[0];
    table[ES]   = pVTable[42];   
    table[DIP]  = pVTable[82];
    DestroyWindow(hWnd);
}
//--------------------------------------------LoopFunction--------------------------------------------------------------------------------------
DWORD WINAPI LoopFunction(LPVOID lpParam)
{
	if(hooked == false)
	{
		DWORD VTable[3] = {0};
		while(LoadLibraryA("d3d9.dll") == NULL)
		{
			Sleep(100);
		}
		DX_Init(VTable);
		HOOK(EndScene,VTable[ES]); //Hook EndScene as a device discovery hook
		while(!npDevice)
		{
			Sleep(50);
		}
		UNHOOK(EndScene, VTable[ES]);
		void*d3DIP = (void*)(dwD3D9 + (DWORD) + (DWORD) + (DWORD) + (PDWORD)BaseD3D + 1024); //[82]
		void*d3ES = (void*)(dwD3D9 + (DWORD) + (DWORD) + (DWORD) + (PDWORD)BaseD3D + 1036);  //[42]
		{
			int i;
			for(i = 0; i <= 0; i++)
			{
				DWORD d,ds;
				VirtualProtect((void*)(d3ES), 4, PAGE_EXECUTE_READWRITE, &d);
				memcpy(d3ES,(void*)(d3ES), 4);
				VirtualProtect((void*)(d3ES), 4, d, &ds);
			}
		}
		int C;
		for(C = 0; C <= 0; C++)
		{
			{
				DWORD d,ds;
				VirtualProtect((void*)(d3DIP), 4, PAGE_EXECUTE_READWRITE, &d);
				memcpy(d3DIP,(void*)(d3DIP), 4);
				VirtualProtect((void*)(d3DIP), 4, d, &ds);
			}
		}
		CreateDetour((DWORD)hkDrawIndexedPrimitive, (DWORD)d3DIP*C,Detour_Type_0xB8,7);
		CreateDetour((DWORD)hkEndScene, (DWORD)d3ES*C,Detour_Type_0xB8,7);
		*(PDWORD)&oDrawIndexedPrimitive = VTable[DIP];
		*(PDWORD)&oEndScene = VTable[ES];
		hooked = true;
	}
	Sleep(10);
	void*d3DIP = (void*)(dwD3D9 + (DWORD) + (DWORD) + (DWORD) + (PDWORD)BaseD3D + 1024); //[82]
	void*d3ES = (void*)(dwD3D9 + (DWORD) + (DWORD) + (DWORD) + (PDWORD)BaseD3D + 1036); //[42]
	{
		DWORD Dark, ds;
		VirtualProtect((void*)(d3ES), 4, PAGE_EXECUTE_READWRITE, &Dark);
		memcpy((void*)d3ES, (const void*)d3ES, 4);
		VirtualProtect((void*)(d3ES), 4, Dark, &ds);
	}
	int i;
	for(i = 0; i <= 15; i++)
	{
		{
			DWORD d, ds;
			VirtualProtect((void*)(d3DIP), 4, PAGE_EXECUTE_READWRITE, &d);
			memcpy((void*)d3DIP, (const void*)d3DIP, 4);
			VirtualProtect((void*)(d3DIP), 4, d, &ds);
		}
	}
	for(i = 0; i <= 15; i++)if(memcmp((void*)d3DIP, (void*)d3DIP, 82) == 0)ProtectHook((LPDWORD*)npDevice, (PBYTE)d3DIP, 82); //Protect Hook Draw Indexed Primitive
	for (i = 0; i <= 15; i++)if(memcmp((void*)d3ES, (void*)d3ES, 42) == 0)ProtectHook((LPDWORD*)npDevice, (PBYTE)d3ES, 42); // Protect Hook End Scene
	Sleep(100);
	return 0;
}
BOOL WINAPI DllMain(HMODULE hModule, DWORD dwReason, LPVOID lpvReserved)
{
	LoadPrivilege();
	if(dwReason == DLL_PROCESS_ATTACH)CreateThread(0, 0,LoopFunction, 0, 0, 0);
	return TRUE;
}