
#include <stdio.h>
#include <windows.h>
#include <tchar.h>
#include <psapi.h>
#pragma comment(lib, "Psapi.lib")

//function pointers defined for hook use
typedef BOOL (*FFF) ();
typedef void (*FF)(TCHAR*);
typedef void (*F)(TCHAR*);

int _tmain(int argc, TCHAR** argv) //this_program.exe working_dir wow_exe
{
	static HINSTANCE hinstDLL; 
	static HHOOK hhookSysMsg; 
	FILE *log;
	TCHAR working_dir[200];
	TCHAR wow_exe[200];
	TCHAR log_filename[200];
	int a = 0;

	
	if(argc>=3)
	{
		lstrcpy(working_dir,argv[1]);
		//strcpy(wow_exe,test);
		lstrcpy(wow_exe,_T("\""));
		lstrcat(wow_exe,argv[2]);
		lstrcat(wow_exe,_T("\""));
		lstrcpy(log_filename,working_dir);
		lstrcat(log_filename,_T("log.txt"));
		log=_tfopen(log_filename,_T("w,ccs=UTF-8"));
		//a = strcmp(wow_exe,wow_exe2);
	}
	else
	{
		MessageBox(NULL,_T("錯誤：未指定工作目錄或魔獸世界執行檔位置!"),_T("錯誤"),MB_OK);
		return -1;
	}
	
	Sleep(3000);
	hinstDLL = LoadLibrary(TEXT("setwinhook_dll.dll")); 
	FFF set = (FFF)GetProcAddress(hinstDLL,"SetHook");
	FFF un = (FFF)GetProcAddress(hinstDLL,"UnHook");
	FF sd = (FF)GetProcAddress(hinstDLL,"setdir");
	F se = (F)GetProcAddress(hinstDLL,"setExe");

	set();
	sd(working_dir);
	se(wow_exe);
	//sd("d:\\wow_test\\");
	//system("pause");
	if(_tcsstr(wow_exe,_T("streamer_player.exe"))!=NULL )
	{
		lstrcat(wow_exe,_T(" smgp://c5a599bcabc0c63b87b9ec0373086510_lan/"));	// Game: Call of Duty - World at War  ( _lan or _wan)
//		lstrcat(wow_exe,_T(" smgp://2aceec859e92cca2aec90d1de8a605c1_wan/"));	// Game: Call of Duty - Wow  ( _lan or _wan)
	}

	_tsystem(wow_exe);
	un();
	FreeLibrary(hinstDLL);
	fclose(log);
	return 0;
}