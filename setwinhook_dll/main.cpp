

#pragma comment(lib, "d3d9.lib")
#pragma comment(lib, "d3dx9.lib")
#pragma comment(lib, "ws2_32.lib")
//#pragma comment(lib, "mswsock.lib")

#include <winsock2.h> //should be the first included header!
#include <d3d9.h>
#include <d3dx9.h>

#include <stdio.h>
#include <stdlib.h>
#include <time.h>

#include <windows.h>
#include <tchar.h>


#pragma data_seg("shared")
#pragma comment(linker, "/section:shared,rws")
//HWND gTarget = 0;    // it is required to initialize shared variables
TCHAR filename_dir[]=_T("c:\\the length of this string is very long since is has to contain a directory name with arbitrary length in windows which is 128");
TCHAR filename_exe[]=_T("c:\\the length of this string is very long since is has to contain a directory name with arbitrary length in windows which is 128");

static bool is_syn=false;
#pragma data_seg()



void setdir(const TCHAR* newname)
{	
	lstrcpy(filename_dir,newname);
/*	FILE * f_dir;
	f_dir = fopen("d:\\log\\dir.txt","a");
	_ftprintf(f_dir,_T("%s\n"), filename_dir);
	fclose(f_dir);
*/
}

// sem edit for gamename
//TCHAR wow_image_name[200];

void setExe(const TCHAR* newname)
{	
	lstrcpy(filename_exe,newname);
/*	FILE * f_exe;
	f_exe = fopen("d:\\log\\exe.txt","a");
	_ftprintf(f_exe,_T("%s\n"), filename_exe);

	if(_tcsstr(filename_exe,_T("Wow.exe"))!=NULL )
	{
		_stprintf(wow_image_name,_T("Wow.exe"));
		_ftprintf(f_exe,_T("%s\n"), wow_image_name);
	}else if(_tcsstr(filename_exe,_T("OnLive.exe"))!=NULL )
	{
		_stprintf(wow_image_name,_T("OnLive.exe"));
		_ftprintf(f_exe,_T("%s\n"), wow_image_name);
	}else if(_tcsstr(filename_exe,_T("streamer_player.exe"))!=NULL )
	{
		_stprintf(wow_image_name,_T("streamer_player.exe"));
		//_ftprintf(f_exe,_T("%s\n"), wow_image_name);
	}

	fclose(f_exe);
*/	
}

//************************ PCAP Time  -  Start ************************
BOOL StickToCore()
{
	//SetThreadPriority(GetCurrentThread(), THREAD_PRIORITY_NORMAL);
	DWORD ThreadAffinityMask = 1L;
    DWORD ret = SetThreadAffinityMask(GetCurrentThread(), ThreadAffinityMask);
	//return SetProcessAffinityMask(GetCurrentProcess(), 1L);
	//if (!ret) 
	{
		//ShowError();
		return ret;
	}
 //SetThreadPriorityBoost(GetCurrentThread(), FALSE);
}
typedef LONG  (WINAPI *QuerySystemTime)  (PLARGE_INTEGER SystemTime);

struct time_conv
{
	ULONGLONG reference;
	struct timeval start[32];
};

HMODULE            hNtdll;
QuerySystemTime    NtQuerySystemTime;
struct time_conv   time_conv={0,{0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0}};


__inline void GetTimeKQPC(struct timeval *dst, struct time_conv *data)
{
	LARGE_INTEGER PTime, TimeFreq;
	LONG tmp;
	static struct timeval old_ts={0,0};
	
	QueryPerformanceFrequency(&TimeFreq);
	QueryPerformanceCounter(&PTime);
	tmp = (LONG)(PTime.QuadPart/TimeFreq.QuadPart);

	dst->tv_sec = data->start[0].tv_sec + tmp;
	dst->tv_usec = data->start[0].tv_usec + (LONG)((PTime.QuadPart%TimeFreq.QuadPart)*1000000/TimeFreq.QuadPart);

	if (dst->tv_usec >= 1000000)
	{
		dst->tv_sec ++;
		dst->tv_usec -= 1000000;
	}
}
         void SynchronizeOnCpu(struct timeval *start)
{
	LARGE_INTEGER SystemTime;
	LARGE_INTEGER TimeFreq,PTime;
	// get the absolute value of the system boot time.   	
    QueryPerformanceFrequency(&TimeFreq);
	QueryPerformanceCounter(&PTime);
	NtQuerySystemTime(&SystemTime);
	
	start->tv_sec = (LONG)(SystemTime.QuadPart/10000000-11644473600);
	start->tv_usec = (LONG)((SystemTime.QuadPart%10000000)/10);
	start->tv_sec -= (ULONG)(PTime.QuadPart/TimeFreq.QuadPart);
	start->tv_usec -= (LONG)((PTime.QuadPart%TimeFreq.QuadPart)*1000000/TimeFreq.QuadPart);
	if (start->tv_usec < 0)
	{
		start->tv_sec --;
		start->tv_usec += 1000000;
	}
}
//************************ PCAP Time  -  End ************************


HRESULT __stdcall PresentHook(IDirect3DDevice9* device, const RECT* pSourceRect, const RECT* pDestRect, HWND hDestWindowOverride, const RGNDATA* pDirtyRegion);
HRESULT __stdcall ClearHook(IDirect3DDevice9* device, DWORD Count, const D3DRECT* pRects, DWORD Flags, D3DCOLOR Color, float Z, DWORD Stencil);
HRESULT __stdcall DrawPrimitiveHook(IDirect3DDevice9* device, D3DPRIMITIVETYPE PrimitiveType,UINT StartVertex,UINT PrimitiveCount);

ULONG __stdcall AddRefHook(IDirect3DDevice9* device);


int     __stdcall send_hook(SOCKET s, const char* buf, int len, int flags);
int     WINAPI    recv_hook(SOCKET s, char *buf, int len, int flags );
int     WINAPI    WSASend_hook(SOCKET s,LPWSABUF lpBuffers,DWORD dwBufferCount,LPDWORD lpNumberOfBytesSent,DWORD dwFlags,LPWSAOVERLAPPED lpOverlapped,LPWSAOVERLAPPED_COMPLETION_ROUTINE lpCompletionRoutine);
int     WINAPI    ioctlsocket_hook( SOCKET s,long cmd,u_long *argp);
int     __stdcall sendto_hook(SOCKET s, const char* buf, int len, int flags, const struct sockaddr* to, int tolen);
int     WINAPI    recvfrom_hook(SOCKET s, char *buf, int len, int flags, sockaddr* to, int* fromlen);

HMODULE         module_self                        = NULL;
HHOOK           g_hhook; 
void *          address_present                    = NULL;
unsigned char * address_winsock2_send              = NULL;
uintptr_t**     g_deviceFunctionAddresses          = NULL;
HMODULE         d3d_mod                            = NULL;
bool            isHooked;
unsigned char   EXPECTED_OPCODES_DEVICE_PRESENT[5] = {0x8B, 0xFF, 0x55, 0x8B, 0xEC};
unsigned char   EXPECTED_OPCODES_DEVICE_CLEAR[5] = {0x8B, 0xFF, 0x55, 0x8B, 0xEC};
unsigned char   EXPECTED_OPCODES_DEVICE_DRAW[5] = {0x8B, 0xFF, 0x55, 0x8B, 0xEC};

unsigned char   EXPECTED_OPCODES_DEVICE_ADDREF[5] = {0x8B, 0xFF, 0x55, 0x8B, 0xEC};


unsigned char   EXPECTED_OPCODES_WINSOCK2_SEND[5];
unsigned char   JMP_OPCODE                         = 0xE9;
DWORD           address_present_hook               = reinterpret_cast<DWORD>(&PresentHook);
DWORD           address_clear_hook               = reinterpret_cast<DWORD>(&ClearHook);
DWORD           address_drawPrimitive_hook          = reinterpret_cast<DWORD>(&DrawPrimitiveHook);

DWORD			address_addRef_hook          = reinterpret_cast<DWORD>(&AddRefHook);

DWORD           address_winsock2_send_hook         = reinterpret_cast<DWORD>(&send_hook);
DWORD           address_winsock2_hook[]            = {reinterpret_cast<DWORD>(&send_hook)};

//winsock2 working section  -  start
#define         NUM_WINSOCK2_FUNCTIONS             5
#define         WINSOCK2_send                      0
#define         WINSOCK2_recv                      1
#define         WINSOCK2_ioctlsocket               2
#define         WINSOCK2_sendto                    3
#define         WINSOCK2_recvfrom                  4
char            winsock2_function_names          [NUM_WINSOCK2_FUNCTIONS] [12] = {"send","recv","ioctlsocket","sendto","recvfrom"};
DWORD           winsock2_function_addresses      [NUM_WINSOCK2_FUNCTIONS]     = {NULL,NULL,NULL};
DWORD           winsock2_function_hook_addresses [NUM_WINSOCK2_FUNCTIONS]     = {reinterpret_cast<DWORD>(&send_hook),reinterpret_cast<DWORD>(&recv_hook),reinterpret_cast<DWORD>(&ioctlsocket_hook),
                                                                                 reinterpret_cast<DWORD>(&sendto_hook),reinterpret_cast<DWORD>(&recvfrom_hook)};
unsigned char   winsock2_function_headers        [NUM_WINSOCK2_FUNCTIONS] [5];
//winsock2 working section  -  end

// sem add for keyboard
//char ch;
//FILE * fkey;
bool            isShiftEnable;
bool			isCtrlEnable;
bool			isEscEnable;

void GenerateKey ( int vk , BOOL bExtended)
{
  KEYBDINPUT  kb={0};
  INPUT    Input={0};
  // generate down 
  if ( bExtended )
    kb.dwFlags  = KEYEVENTF_EXTENDEDKEY;
  kb.wVk  = vk;  
  Input.type  = INPUT_KEYBOARD;

  Input.ki  = kb;
  ::SendInput(1,&Input,sizeof(Input));

  // generate up 
  ::ZeroMemory(&kb,sizeof(KEYBDINPUT));
  ::ZeroMemory(&Input,sizeof(INPUT));
  kb.dwFlags  =  KEYEVENTF_KEYUP;
  if ( bExtended )
    kb.dwFlags  |= KEYEVENTF_EXTENDEDKEY;

  kb.wVk    =  vk;
  Input.type  =  INPUT_KEYBOARD;
  Input.ki  =  kb;
  ::SendInput(1,&Input,sizeof(Input));
}


void winsock2_remove_hook(unsigned int function)
{
	unsigned char * p = reinterpret_cast<unsigned char*>(winsock2_function_addresses[function]);
	*(p)=winsock2_function_headers[function][0];
	*(p+1)=winsock2_function_headers[function][1];
	*(p+2)=winsock2_function_headers[function][2];
	*(p+3)=winsock2_function_headers[function][3];
	*(p+4)=winsock2_function_headers[function][4];
}

void winsock2_install_hook(unsigned int function)
{
	*(reinterpret_cast<unsigned char*>(winsock2_function_addresses[function]))=JMP_OPCODE;
	*(reinterpret_cast<unsigned int *>(winsock2_function_addresses[function]+1))=winsock2_function_hook_addresses[function]-winsock2_function_addresses[function]-5;
}

void winsock2_backup_function_header(unsigned int function)
{
	unsigned char * p = reinterpret_cast<unsigned char*>(winsock2_function_addresses[function]);
	winsock2_function_headers[function][0]=*(p+0);
	winsock2_function_headers[function][1]=*(p+1);
	winsock2_function_headers[function][2]=*(p+2);
	winsock2_function_headers[function][3]=*(p+3);
	winsock2_function_headers[function][4]=*(p+4);
}

time_t          time_start                         = 0;
time_t          time_current                       = 0;
unsigned long   frames                             = 0;
const double    fps_duration                       = 5.0;

int __stdcall send_hook(SOCKET s, const char* buf, int len, int flags)
{
/*	//print out data
	//char * aa = (char*)malloc(len+1);
	//strncpy(aa,buf,len);
	//aa[len]='\0';
	FILE * f_send1;
	f_send1 = fopen("d:\\log\\send_hook.txt","a");
	fprintf(f_send1,"send function hooked successfully!\n");
	//fprintf(f_send1,"%s\n",aa);
	fclose(f_send1);
	//free(aa);
*/
	struct timeval now;
	LARGE_INTEGER t,t2;
	QueryPerformanceFrequency(&t);
	QueryPerformanceCounter(&t2);
	//SynchronizeOnCpu(&time_conv.start[0]);
	GetTimeKQPC(&now,&time_conv);
	TCHAR filename[100];
	TCHAR local_port[10];
	TCHAR remote_port[10];
	sockaddr tmp;
	bool file_exist=false;
	int namelen=sizeof(tmp);
	
	winsock2_remove_hook(WINSOCK2_send);
	int ret = send(s,buf,len,flags);
	winsock2_install_hook(WINSOCK2_send);


	if(getsockname(s,&tmp,&namelen) == 0)
	{
		_stprintf(local_port,_T("%d"),ntohs(((struct sockaddr_in*)&tmp)->sin_port));
	}
	else//==SOCKET_ERROR
	{
		_stprintf(local_port,_T("%s"),_T("00000"));
	}
	if(getpeername(s,&tmp,&namelen) == 0)
	{
		_stprintf(remote_port,_T("%d"),ntohs(((struct sockaddr_in*)&tmp)->sin_port));
	}
	else//==SOCKET_ERROR
	{
		_stprintf(remote_port,_T("%s"),_T("00000"));
	}
/*	if(ntohs(((struct sockaddr_in*)&tmp)->sin_port) < 100 || ntohs(((struct sockaddr_in*)&tmp)->sin_port) > 10000 || ntohs(((struct sockaddr_in*)&tmp)->sin_port)==3724)
	{
		return ret;
	}
*/
	_stprintf(filename,_T("%s%s.send.txt"),filename_dir,remote_port);
    //test if file exists
    FILE *f_send = _tfopen(filename,_T("r"));
    
	if(f_send != NULL)
	{
		file_exist=true;
		fclose(f_send);
	}
	f_send=_tfopen(filename,_T("a"));
	if(!file_exist)
	{
		_ftprintf(f_send,_T("time len ret flags\n"));
	}
	_ftprintf(f_send,_T("%d.%06d %d %d %d\n"),now.tv_sec,now.tv_usec,len,ret,flags);
	fclose(f_send);

	return ret;
}

int __stdcall sendto_hook(SOCKET s, const char* buf, int len, int flags, const struct sockaddr* to, int tolen)
{
/*	//char * aa = (char*)malloc(len+1);
	//strncpy(aa,buf,len);
	//aa[len]='\0';
	FILE * f_send;
	f_send = fopen("d:\\log\\sendto_hook.txt","a");
	fprintf(f_send,"sendto function hooked successfully!\n");
	//fprintf(f_send,"%s\n",aa);
	fclose(f_send);
	//free(aa);
*/

	struct timeval now;
	LARGE_INTEGER t,t2;
	QueryPerformanceFrequency(&t);
	QueryPerformanceCounter(&t2);
	//SynchronizeOnCpu(&time_conv.start[0]);
	GetTimeKQPC(&now,&time_conv);
	TCHAR filename[100];
	TCHAR local_port[10];
	TCHAR remote_port[10];
	sockaddr tmp;
	bool file_exist=false;
	int namelen=sizeof(tmp);
	

	winsock2_remove_hook(WINSOCK2_sendto);
	int ret = sendto(s,buf,len,flags,to,tolen);
	winsock2_install_hook(WINSOCK2_sendto);


	if(getsockname(s,&tmp,&namelen) == 0)
	{
		_stprintf(local_port,_T("%d"),ntohs(((struct sockaddr_in*)&tmp)->sin_port));
	}
	else//==SOCKET_ERROR
	{
		_stprintf(local_port,_T("%s"),_T("00000"));
	}
	if(getpeername(s,&tmp,&namelen) == 0)
	{
		_stprintf(remote_port,_T("%d"),ntohs(((struct sockaddr_in*)&tmp)->sin_port));
	}
	else//==SOCKET_ERROR
	{
		_stprintf(remote_port,_T("%s"),_T("00000"));
	}

	_stprintf(filename,_T("%s%s.sendto.txt"),filename_dir,remote_port);
    //test if file exists
    FILE *f_send = _tfopen(filename,_T("r"));
    
	if(f_send != NULL)
	{
		file_exist=true;
		fclose(f_send);
	}
	f_send=_tfopen(filename,_T("a"));
	if(!file_exist)
	{
		_ftprintf(f_send,_T("time len ret flags\n"));
	}
	_ftprintf(f_send,_T("%d.%06d %d %d %d\n"),now.tv_sec,now.tv_usec,len,ret,flags);
	fclose(f_send);
	
	
	return ret;
}


int WINAPI recv_hook( SOCKET s, char *buf, int len, int flags )
{
/*	//char * aa = (char*)malloc(len+1);
	//strncpy(aa,buf,len);
	//aa[len]='\0';
	FILE * f_send;
	f_send = fopen("d:\\log\\recv_hook.txt","a");
	fprintf(f_send,"recv function hooked successfully!\n");
	//fprintf(f_send,"%s\n",aa);
	fclose(f_send);
	//free(aa);
*/
	
	FILE * f_recv;
	struct timeval now;
	TCHAR filename[100];
	TCHAR local_port[10];
	TCHAR remote_port[10];
	sockaddr tmp;
	bool file_exist=false;
	bool toreturn=false;
	int namelen=sizeof(tmp);
	if(getsockname(s,&tmp,&namelen) == 0)
	{
		_stprintf(local_port,_T("%d"),ntohs(((struct sockaddr_in*)&tmp)->sin_port));
	}
	else//==SOCKET_ERROR
	{
		_stprintf(local_port,_T("%d"),00000);
	}
	if(getpeername(s,&tmp,&namelen) == 0)
	{
		_stprintf(remote_port,_T("%d"),ntohs(((struct sockaddr_in*)&tmp)->sin_port));
	}
	else//==SOCKET_ERROR
	{
		_stprintf(remote_port,_T("%d"),00000);
	}
/*	if(ntohs(((struct sockaddr_in*)&tmp)->sin_port) < 100 || ntohs(((struct sockaddr_in*)&tmp)->sin_port) > 10000 || ntohs(((struct sockaddr_in*)&tmp)->sin_port)==3724)
	{
		toreturn=true;
	}
	*/
	_stprintf(filename,_T("%s%s.recv.txt"),filename_dir,remote_port);
	if( (f_recv=_tfopen(filename,_T("r"))) != NULL)
	{
		file_exist=true;
		fclose(f_recv);
	}
	if(!toreturn)
	{
		f_recv=_tfopen(filename,_T("a"));
	}


	winsock2_remove_hook(WINSOCK2_recv);
	int ret = recv(s,buf,len,flags);
	winsock2_install_hook(WINSOCK2_recv);

	if(toreturn)
	{
		return ret;
	}
	GetTimeKQPC(&now, &time_conv);
	if(!file_exist)
	{
		_ftprintf(f_recv,_T("time len ret flags\n"));
	}
	//notice! ret may be SOCKET_ERROR(-1)
	_ftprintf(f_recv,_T("%d.%06d %d %d %d\n"),now.tv_sec,now.tv_usec,len,ret,flags);
	fclose(f_recv);
	
	return ret;
}

int WINAPI recvfrom_hook( SOCKET s, char *buf, int len, int flags, sockaddr* to, int* fromlen )
{
/*	//char * aa = (char*)malloc(len+1);
	//strncpy(aa,buf,len);
	//aa[len]='\0';
	FILE * f_send;
	f_send = fopen("d:\\log\\recvfrom_hook.txt","a");
	fprintf(f_send,"sendto function hooked successfully!\n");
	//fprintf(f_send,"%s\n",aa);
	fclose(f_send);
	//free(aa);
*/
	char buf_null[512];

	FILE * f_recv;
	struct timeval now;
	TCHAR filename[100];
	TCHAR local_port[10];
	TCHAR remote_port[10];
	sockaddr tmp;
	bool file_exist=false;
	bool toreturn=false;
	int namelen=sizeof(tmp);
	if(getsockname(s,&tmp,&namelen) == 0)
	{
		_stprintf(local_port,_T("%d"),ntohs(((struct sockaddr_in*)&tmp)->sin_port));
	}
	else//==SOCKET_ERROR
	{
		_stprintf(local_port,_T("%d"),00000);
	}
	if(getpeername(s,&tmp,&namelen) == 0)
	{
		_stprintf(remote_port,_T("%d"),ntohs(((struct sockaddr_in*)&tmp)->sin_port));
	}
	else//==SOCKET_ERROR
	{
		_stprintf(remote_port,_T("%d"),00000);
	}

	_stprintf(filename,_T("%s%s.recvfrom.txt"),filename_dir,remote_port);
	if( (f_recv=_tfopen(filename,_T("r"))) != NULL)
	{
		file_exist=true;
		fclose(f_recv);
	}
	if(!toreturn)
	{
		f_recv=_tfopen(filename,_T("a"));
	}

/*	if(isShiftEnable)
	{
		_ftprintf(f_recv,_T("Shift Eanble\n"));

		GetTimeKQPC(&now, &time_conv);
		_ftprintf(f_recv,_T("%d.%06d\n"),now.tv_sec,now.tv_usec);
		fclose(f_recv);
		Sleep(5000);
	}
*/
	if(isShiftEnable)
	{
		fclose(f_recv);
		FILE * f_p;
		f_p = fopen("d:\\log\\break.recvform.txt","a");
		_ftprintf(f_p,_T("Shift Eanble\n"));
		winsock2_remove_hook(WINSOCK2_recvfrom);
		int ret = recvfrom(s,buf_null,0,flags,to,0);
		winsock2_install_hook(WINSOCK2_recvfrom);
		GetTimeKQPC(&now, &time_conv);
		_ftprintf(f_p,_T("%d.%06d %d %d %d\n"),now.tv_sec,now.tv_usec,len,ret,flags);
		fclose(f_p);
		return ret;
	}
	

	winsock2_remove_hook(WINSOCK2_recvfrom);
	int ret = recvfrom(s,buf,len,flags,to,fromlen);
	winsock2_install_hook(WINSOCK2_recvfrom);
	
	if(isShiftEnable)
	{
		return ret;
	}

	if(toreturn)
	{
		return ret;
	}

	GetTimeKQPC(&now, &time_conv);
	if(!file_exist)
	{
		_ftprintf(f_recv,_T("time len ret flags\n"));
	}
	//notice! ret may be SOCKET_ERROR(-1)
	_ftprintf(f_recv,_T("%d.%06d %d %d %d\n"),now.tv_sec,now.tv_usec,len,ret,flags);
	fclose(f_recv);

	return ret;
}

int     WINAPI    ioctlsocket_hook( SOCKET s,long cmd,u_long *argp)
{
	struct timeval now;
	TCHAR filename[200];
	//SetThreadAffinityMask(GetCurrentThread(), 1L);
	
	//GetProcessAffinityMask( GetCurrentProcess(), &dwProcessAffinityMask, &dwSystemAffinityMask );
	GetTimeKQPC(&now,&time_conv);
	winsock2_remove_hook(WINSOCK2_ioctlsocket);
	int ret = ioctlsocket(s,cmd,argp);
	winsock2_install_hook(WINSOCK2_ioctlsocket);
	_stprintf(filename,_T("%sioctlsocket.txt"),filename_dir);
	
	FILE *f = _tfopen (filename,_T("a"));
	switch(cmd)
	{
	case FIONBIO:    _ftprintf(f,_T("cmd=%s, argp=%d, ret=%d @ %d.%06d\n"),_T("FIONBIO"),   *argp,ret,now.tv_sec,now.tv_usec);break;
	case FIONREAD:   _ftprintf(f,_T("cmd=%s, argp=%d, ret=%d @ %d.%06d\n"),_T("FIONREAD"),  *argp,ret,now.tv_sec,now.tv_usec);break;
	case SIOCATMARK: _ftprintf(f,_T("cmd=%s, argp=%d, ret=%d @ %d.%06d\n"),_T("SIOCATMARK"),*argp,ret,now.tv_sec,now.tv_usec);break;
	default:         _ftprintf(f,_T("cmd=%s, argp=%d, ret=%d @ %d.%06d\n"),_T("UNKOWN"),    *argp,ret,now.tv_sec,now.tv_usec);break;
	}
	fclose(f);
	
	return ret;
}

HRESULT __stdcall PresentHook(IDirect3DDevice9* device, const RECT* pSourceRect, const RECT* pDestRect, HWND hDestWindowOverride, const RGNDATA* pDirtyRegion) {
	// The hook function for IDirect3DDevice9::Present
	struct timeval now;
	GetTimeKQPC(&now,&time_conv);
	TCHAR filename_out[200];
	_stprintf(filename_out,_T("%swow_d3d.present.txt"),filename_dir);
	FILE * f_present = _tfopen(filename_out,_T("a"));
	_ftprintf(f_present,_T("%d.%06d\n"),now.tv_sec,now.tv_usec);
	fclose(f_present);

	if(isShiftEnable)
	{
		FILE * f_p;
		f_p = fopen("d:\\log\\break.present.txt","a");
		_ftprintf(f_p,_T("%d.%06d\n"),now.tv_sec,now.tv_usec);
		_ftprintf(f_p,_T("Shift Eanble\n"));
		fclose(f_p);
		
//		Sleep(2000);
	}

	//	ºI¹Ï¨ìbmp
	if(isEscEnable || isCtrlEnable)
	{
		FILE * f_p;
		f_p = fopen("d:\\log\\break.capture.txt","a");
		_ftprintf(f_p,_T("%d.%06d\n"),now.tv_sec,now.tv_usec);
		_ftprintf(f_p,_T("capture Eanble\n"));
		// save bmp
		HRESULT hr = 0;
		IDirect3DSurface9 *pBackBuffer;
		hr = device->GetBackBuffer(0,0,D3DBACKBUFFER_TYPE_MONO,&pBackBuffer);
		if (FAILED(hr)) 
			_ftprintf(f_p,_T("Fail GetBackBuffer\n"));

		D3DSURFACE_DESC surface_Desc;
		hr = pBackBuffer->GetDesc(&surface_Desc);
		if (FAILED(hr))
			_ftprintf(f_p,_T("Fail GetDesc\n"));


		IDirect3DTexture9 *pTex = NULL;
		IDirect3DSurface9 *pTexSurface = NULL;
		device->CreateTexture(surface_Desc.Width,
                                surface_Desc.Height,
                                1,
                                0,
                                surface_Desc.Format,
                                D3DPOOL_SYSTEMMEM, 
                                &pTex, NULL);
		if (pTex)
			hr = pTex->GetSurfaceLevel(0, &pTexSurface);
		if (pTexSurface)
			hr = device->GetRenderTargetData(pBackBuffer, pTexSurface);

		D3DLOCKED_RECT lockedRect;
		pTex->LockRect(0, &lockedRect, NULL, D3DLOCK_READONLY);


		TCHAR filename_bmp[200];
		_stprintf(filename_bmp,_T("%s%d.%06d.bmp"),filename_dir,now.tv_sec,now.tv_usec);

		pTex->UnlockRect(0);
		D3DXSaveTextureToFile(filename_bmp, D3DXIFF_BMP, pTex, NULL);

		fclose(f_p);
		pBackBuffer->Release();
		pTex->Release();
		pTexSurface->Release();

	}

	//remove Present Hook
	unsigned char * present  = (unsigned char*)g_deviceFunctionAddresses[17];
	for(unsigned int i = 0 ; i < 5 ; ++i)
	{
		*(present+i)=EXPECTED_OPCODES_DEVICE_PRESENT[i];
	}

	//orignial Present
	HRESULT ret = device->Present(pSourceRect,pDestRect,hDestWindowOverride,pDirtyRegion);
	//install Present Hook
	*(present+0)=JMP_OPCODE;
	*((DWORD *)(present+1)) = address_present_hook - reinterpret_cast<DWORD>(g_deviceFunctionAddresses[17]) - 5;
	return ret;
}

HRESULT __stdcall ClearHook(IDirect3DDevice9* device, DWORD Count, const D3DRECT* pRects, DWORD Flags, D3DCOLOR Color, float Z, DWORD Stencil) {
	// The hook function for IDirect3DDevice9::Clear
	struct timeval now;
	GetTimeKQPC(&now,&time_conv);
	TCHAR filename_out[200];
	_stprintf(filename_out,_T("%swow_d3d.clear.txt"),filename_dir);
	FILE * f_present = _tfopen(filename_out,_T("a"));
	_ftprintf(f_present,_T("%d.%06d\n"),now.tv_sec,now.tv_usec);
	fclose(f_present);
	//remove Clear Hook
	unsigned char * clear  = (unsigned char*)g_deviceFunctionAddresses[43];
	for(unsigned int i = 0 ; i < 5 ; ++i)
	{
		*(clear+i)=EXPECTED_OPCODES_DEVICE_CLEAR[i];
	}

	//orignial Clear
	HRESULT ret = device->Clear(Count, pRects, Flags,  Color, Z, Stencil);
	
	//install Clear Hook
	*(clear+0)=JMP_OPCODE;
	*((DWORD *)(clear+1)) = address_clear_hook - reinterpret_cast<DWORD>(g_deviceFunctionAddresses[43]) - 5;
	return ret;
}

HRESULT __stdcall DrawPrimitiveHook(IDirect3DDevice9* device, D3DPRIMITIVETYPE PrimitiveType,UINT StartVertex,UINT PrimitiveCount) {
	// The hook function for IDirect3DDevice9::DrawPrimitive
	struct timeval now;
	GetTimeKQPC(&now,&time_conv);
	TCHAR filename_out[200];
	_stprintf(filename_out,_T("%swow_d3d.draw.txt"),filename_dir);
	FILE * f_present = _tfopen(filename_out,_T("a"));
	_ftprintf(f_present,_T("%d.%06d\n"),now.tv_sec,now.tv_usec);
	fclose(f_present);
	//remove UpdateSurface Hook
	unsigned char * draw  = (unsigned char*)g_deviceFunctionAddresses[81];
	for(unsigned int i = 0 ; i < 5 ; ++i)
	{
		*(draw+i)=EXPECTED_OPCODES_DEVICE_DRAW[i];
	}

	//orignial UpdateSurface
	HRESULT ret = device->DrawPrimitive( PrimitiveType, StartVertex, PrimitiveCount);
	
	//install UpdateSurface Hook
	*(draw+0)=JMP_OPCODE;
	*((DWORD *)(draw+1)) = address_drawPrimitive_hook - reinterpret_cast<DWORD>(g_deviceFunctionAddresses[81]) - 5;
	return ret;
}


ULONG __stdcall AddRefHook(IDirect3DDevice9* device) {
	// The hook function for  AddRef
	struct timeval now;
	GetTimeKQPC(&now,&time_conv);
	TCHAR filename_out[200];
	_stprintf(filename_out,_T("%swow_d3d.AddRef.txt"),filename_dir);
	FILE * f_present = _tfopen(filename_out,_T("a"));
	_ftprintf(f_present,_T("%d.%06d\n"),now.tv_sec,now.tv_usec);
	fclose(f_present);
	//remove AddRef Hook
	unsigned char * addRef  = (unsigned char*)g_deviceFunctionAddresses[1];
	for(unsigned int i = 0 ; i < 5 ; ++i)
	{
		*(addRef+i)=EXPECTED_OPCODES_DEVICE_ADDREF[i];
	}

	//orignial AddRef
	ULONG ret = device->AddRef();
	
	//install AddRef Hook
	*(addRef+0)=JMP_OPCODE;
	*((DWORD *)(addRef+1)) = address_addRef_hook - reinterpret_cast<DWORD>(g_deviceFunctionAddresses[1]) - 5;
	return ret;
}


uintptr_t* APIENTRY GetD3D9DeviceFunctionAddress(short methodIndex)
{
	// There are 119 functions defined in the IDirect3DDevice9 interface 
	const int interfaceMethodCount = 119;

	if (!g_deviceFunctionAddresses) {
		// Ensure d3d9.dll is loaded
		//HMODULE hMod = LoadLibraryA("d3d9.dll");
		HMODULE hMod = GetModuleHandle(_T("d3d9"));
		LPDIRECT3D9       pD3D       = NULL;
		LPDIRECT3DDEVICE9 pd3dDevice = NULL;
		d3d_mod=hMod;

		HWND hWnd = CreateWindowA("BUTTON", "Temporary Window", WS_SYSMENU | WS_MINIMIZEBOX, CW_USEDEFAULT, CW_USEDEFAULT, 300, 300, NULL, NULL, NULL /* dllHandle */, NULL);
		__try 
		{
			pD3D = Direct3DCreate9( D3D_SDK_VERSION );
			if(pD3D == NULL )
			{
				// Respond to failure of Direct3DCreate9
				return 0;
			}
			__try 
			{
				D3DDISPLAYMODE d3ddm;

				if( FAILED( pD3D->GetAdapterDisplayMode( D3DADAPTER_DEFAULT, &d3ddm ) ) )
				{
					//  Respond to failure of GetAdapterDisplayMode
					return 0;
				}

				D3DCAPS9 d3dCaps;
				if( FAILED(pD3D->GetDeviceCaps( D3DADAPTER_DEFAULT, 
												   D3DDEVTYPE_HAL, &d3dCaps ) ) )
				{
					// Respond to failure of GetDeviceCaps
					return 0;
				}

				DWORD dwBehaviorFlags = 0;

				if( d3dCaps.VertexProcessingCaps != 0 )
					dwBehaviorFlags |= D3DCREATE_HARDWARE_VERTEXPROCESSING;
				else
					dwBehaviorFlags |= D3DCREATE_SOFTWARE_VERTEXPROCESSING;

				D3DPRESENT_PARAMETERS d3dpp;
				memset(&d3dpp, 0, sizeof(d3dpp));

				d3dpp.BackBufferFormat       = d3ddm.Format;
				d3dpp.SwapEffect             = D3DSWAPEFFECT_DISCARD;
				d3dpp.Windowed               = TRUE;
				d3dpp.EnableAutoDepthStencil = TRUE;
				d3dpp.AutoDepthStencilFormat = D3DFMT_D16;
				d3dpp.PresentationInterval   = D3DPRESENT_INTERVAL_IMMEDIATE;

				if( FAILED( pD3D->CreateDevice( D3DADAPTER_DEFAULT, D3DDEVTYPE_HAL, hWnd,
												  dwBehaviorFlags, &d3dpp, &pd3dDevice ) ) )
				{
					// Respond to failure of CreateDevice
					return 0;
				}

				//
				// Device has been created - we can now check for the method addresses
				//

				__try 
				{
					// retrieve a pointer to the VTable
					uintptr_t* pInterfaceVTable = (uintptr_t*)*(uintptr_t*)pd3dDevice;
					g_deviceFunctionAddresses = new uintptr_t*[interfaceMethodCount];

					//add_log("d3d9.dll base address: 0x%x", hMod);

	
					for (int i=0; i<interfaceMethodCount; i++) {
						g_deviceFunctionAddresses[i] = (uintptr_t*)pInterfaceVTable[i];
						
						// Log the address offset
						//add_log("Method [%i] offset: 0x%x", i, pInterfaceVTable[i] - (uintptr_t)hMod);
					}
				}
				__finally {
					pd3dDevice->Release();
				}
			}
			__finally
			{
				pD3D->Release();
			}
		}
		__finally {
			DestroyWindow(hWnd);
		}
	}

	// Return the address of the method requested
	if (g_deviceFunctionAddresses) {
		return g_deviceFunctionAddresses[methodIndex];
	} else {
		return 0;
	}
}


LRESULT CALLBACK Hook_Proc( int nCode, WPARAM wParam, LPARAM lParam)
{
	// sem add for keyboard
	 if(GetAsyncKeyState(VK_LSHIFT))
	 {
		isShiftEnable=true;
	 }

	 if(GetAsyncKeyState(VK_RSHIFT))
	 {
		isShiftEnable=false;
	 }
	 
	 if(GetAsyncKeyState(VK_LCONTROL))
	 {
		isCtrlEnable=true;
		/*KEYBDINPUT key;
		key.wVk =VK_ESCAPE;

		INPUT inputKey;
		inputKey.type=INPUT_KEYBOARD;
		inputKey.ki=key;

		INPUT event[1] = {inputKey};
		SendInput(1,event,sizeof(INPUT));*/
		//GenerateKey (VK_ESCAPE, FALSE);
	 }

	 if(GetAsyncKeyState(VK_RCONTROL))
	 {
		isCtrlEnable=false;
		isEscEnable=false;
	 }

	 if(GetAsyncKeyState(VK_ESCAPE))
	 {
		isEscEnable=true;
		FILE * f_key;
		f_key = fopen("d:\\log\\key_enable.txt","a");
		struct timeval now;
		GetTimeKQPC(&now,&time_conv);
		_ftprintf(f_key,_T("%d.%06d\n"),now.tv_sec,now.tv_usec);
		fclose(f_key);
	 }

	//initialize time before we can use it
	hNtdll            = GetModuleHandleA("ntdll.dll");
	NtQuerySystemTime = (QuerySystemTime)GetProcAddress(hNtdll, "NtQuerySystemTime");
	DWORD affinity = StickToCore();
	TCHAR timage_name[200];
	GetModuleFileName(NULL,timage_name,200);

	if(GetModuleHandle(_T("quartz")) != NULL)
	{
		FILE *f;
		TCHAR image_name[200];
		TCHAR filename_d3d[200];
		_stprintf(filename_d3d,_T("%sdirectshow.txt"),filename_dir);
		f=_tfopen(filename_d3d,_T("a"));
		if(!GetModuleFileName(NULL,image_name,200))
		{
			_ftprintf(f,_T("can't get image name\r\n"));
		}
		else
		{			
			TCHAR *wow_image_name;
			if(_tcsstr(filename_exe,_T("streamer_player.exe"))!=NULL )
			{
				wow_image_name=_T("c:\\Program Files\\StreamMyGame\\streamer_player.exe");
			}
			TCHAR buf[255];
			
			if(_tcsstr(image_name,wow_image_name)!=NULL && g_deviceFunctionAddresses==NULL)
			{
				_ftprintf(f,_T("%s\r\n"),image_name);
				_ftprintf(f,_T("%s\r\n"),wow_image_name);
				_ftprintf(f,_T("main loop\r\n"));
			}
			else
			{
				_ftprintf(f,_T("quartz fail. image:%s, game:%s, device_fun:%x\r\n"),image_name,wow_image_name,g_deviceFunctionAddresses);
			}
		}
		fclose(f);
	}

	if(GetModuleHandle(_T("d3d9")) != NULL)
	{
		FILE *f;
		TCHAR image_name[200];
		TCHAR filename_d3d[200];
		_stprintf(filename_d3d,_T("%sd3d9dection.txt"),filename_dir);
		f=_tfopen(filename_d3d,_T("a"));
		if(!GetModuleFileName(NULL,image_name,200))
		{
			_ftprintf(f,_T("can't get image name\r\n"));
		}
		else
		{			
			TCHAR *wow_image_name;
			if(_tcsstr(filename_exe,_T("Wow.exe"))!=NULL )
			{
				wow_image_name=_T("Wow.exe");
			}else if(_tcsstr(filename_exe,_T("OnLive.exe"))!=NULL )
			{
				wow_image_name=_T("OnLive.exe");
			}else if(_tcsstr(filename_exe,_T("streamer_player.exe"))!=NULL )
			{
				wow_image_name=_T("c:\\Program Files\\StreamMyGame\\streamer_player.exe");
			}

			TCHAR buf[255];

			
			if(_tcsstr(image_name,wow_image_name)!=NULL  && g_deviceFunctionAddresses==NULL)
			{
				_ftprintf(f,_T("%s\r\n"),image_name);
				_ftprintf(f,_T("%s\r\n"),wow_image_name);
				
				//	hook present 17
				if(GetD3D9DeviceFunctionAddress(17)!=0)
				{
					DWORD offset = reinterpret_cast<DWORD>(g_deviceFunctionAddresses[17])-(uintptr_t)d3d_mod;
					char *       p      = reinterpret_cast<char *>(g_deviceFunctionAddresses[17]);
					
					
					_ftprintf(f,_T("PresentHook address: 0x%X\n"),address_present_hook);
					_ftprintf(f,_T("D3D base address: 0x%X(unsigned int : 0x%X)\r\n"),d3d_mod,(uintptr_t)d3d_mod);
					_ftprintf(f,_T("D3D(IDirect3DDevice9::Present<17>)  address: 0x%X (offset 0x%X)\r\n"),g_deviceFunctionAddresses[17],(unsigned int)g_deviceFunctionAddresses[17] - (uintptr_t)d3d_mod);
					_ftprintf(f,_T("D3D(IDirect3DDevice9::GetBackBuffer<18>)  address: 0x%X (offset 0x%X)\r\n"),g_deviceFunctionAddresses[18],(unsigned int)g_deviceFunctionAddresses[18] - (uintptr_t)d3d_mod);
					_ftprintf(f,_T("first 5 bytes of Present are [0]:0x%X, [1]:0x%X, [2]:0x%X, [3]:0x%X, [4]:0x%X\n"),(unsigned int)(*p) & 0x000000FF,(unsigned int)(*(p+1)) & 0x000000FF,(unsigned int)(*(p+2)) & 0x000000FF,(unsigned int)(*(p+3)) & 0x000000FF,(unsigned int)(*(p+4)) & 0x000000FF);
					_ftprintf(f,_T("PresentHook - Present - 5 = 0x%X\n"),address_present_hook - reinterpret_cast<DWORD>(g_deviceFunctionAddresses[17]) - 5);
					
					//set read/write permission of virtual memory
					unsigned char * present  = (unsigned char*)g_deviceFunctionAddresses[17];
					DWORD old_protect = 0;
					if (VirtualProtect((void*)present, 5, PAGE_EXECUTE_READWRITE, &old_protect) == FALSE) 
					{
						_ftprintf(f,_T("virtualprotect failed!\n"));
					}
					if (IsBadWritePtr((void*)present, 5) != FALSE) 
					{
						_ftprintf(f,_T("badwriteptr\n"));
					}
					//install Present Hook
					//memcpy(address, reinterpret_cast<void*> (patch), 5);
					*(present+0)=JMP_OPCODE;
					*((unsigned int *)(present+1)) = address_present_hook - (unsigned int)g_deviceFunctionAddresses[17] - 5;
					_ftprintf(f,_T("first 5 bytes of Present(after hook) are [0]:0x%X, [1]:0x%X, [2]:0x%X, [3]:0x%X, [4]:0x%X\n"),(unsigned int)(*p) & 0x000000FF,(unsigned int)(*(p+1)) & 0x000000FF,(unsigned int)(*(p+2)) & 0x000000FF,(unsigned int)(*(p+3)) & 0x000000FF,(unsigned int)(*(p+4)) & 0x000000FF);
					_ftprintf(f,_T("address_present_hook: 0x%X\n"),(unsigned int)g_deviceFunctionAddresses[17]+( (unsigned int)(address_present_hook - (unsigned int)g_deviceFunctionAddresses[17])));
					//fprintf(f,"hi\n");
				}
				else
				{
					_ftprintf(f,_T("can't get present addresses\r\n"));
				}
			}
			else
			{
				_ftprintf(f,_T("d3d9 _tcsstr fail. image:%s, game:%s, device_fun:%x\r\n"),image_name,wow_image_name,g_deviceFunctionAddresses);
			}
		}
		fclose(f);
	}
	//*/
	

	HMODULE mod_ws2_32;
	if((mod_ws2_32=GetModuleHandle(_T("ws2_32"))) != NULL) //also wsock32 and mswsock
	{
		FILE *f;
		TCHAR image_name[200];
		TCHAR filename_ws2[200];
		_stprintf(filename_ws2,_T("%sws2_32dection.txt"),filename_dir);
		f=_tfopen(filename_ws2,_T("a"));
		_ftprintf(f,_T("thread affinity: %x\n"),affinity);
		if(!GetModuleFileName(NULL,image_name,200))
		{			
			_ftprintf(f,_T("can't get image name\r\n"));
		}
		else
		{			
//			TCHAR wow_image_name[]=_T("OnLive.exe");
			TCHAR *wow_image_name;
			if(_tcsstr(filename_exe,_T("Wow.exe"))!=NULL )
			{
				wow_image_name=_T("Wow.exe");
			}else if(_tcsstr(filename_exe,_T("OnLive.exe"))!=NULL )
			{
				wow_image_name=_T("OnLive.exe");
			}else if(_tcsstr(filename_exe,_T("streamer_player.exe"))!=NULL )
			{
				wow_image_name=_T("streamer_player.exe");
			}


			if(_tcsstr(image_name,wow_image_name)!=NULL  && g_deviceFunctionAddresses==NULL)
			{
				_ftprintf(f,_T("%s\r\n"),image_name);
				_ftprintf(f,_T("%s\r\n"),wow_image_name);
				time_t current;
				TCHAR   t[40];
				time(&current);
				_tcsftime(t,40,_T("%Y-%m-%d %H:%M:%S"),localtime(&current));
				_ftprintf(f,_T("%s @ %s\r\n"),image_name,t);

				g_deviceFunctionAddresses=(uintptr_t**)1;
				//psend ori_send = (psend)GetProcAddress(mod_ws2_32,"send");
				//address_winsock2_send_ori = *ori_send;
				
				for(unsigned int i = 0 ; i < NUM_WINSOCK2_FUNCTIONS ; i++)
				{
					winsock2_function_addresses[i] = (DWORD)GetProcAddress(mod_ws2_32,winsock2_function_names[i]);
					//fprintf(f,"about to hook %s\n",winsock2_function_names[i]);
					if(winsock2_function_addresses[i]!=NULL)
					{
						winsock2_backup_function_header(i);
						DWORD old_protect = 0;
						if (VirtualProtect(reinterpret_cast<void*>(winsock2_function_addresses[i]), 5, PAGE_EXECUTE_READWRITE, &old_protect) == FALSE) 
						{
							_ftprintf(f,_T("virtualprotect failed!\n"));
						}
						if (IsBadWritePtr(reinterpret_cast<void*>(winsock2_function_addresses[i]), 5) != FALSE) 
						{
							_ftprintf(f,_T("badwriteptr\n"));
						}
						winsock2_install_hook(i);
						fprintf(f,"%s hooked\n",(winsock2_function_names[i]));
					}
					else
					{
						_ftprintf(f,_T("%s not hooked(can't find %s)\n"),winsock2_function_names[i],winsock2_function_names[i]);
					}
				}
				
			}
			else
			{
				_ftprintf(f,_T("ws32 _tcsstr fail\r\n"));
			}
		}
		fclose(f);
	}

	return CallNextHookEx(g_hhook,nCode,wParam,lParam);
}


BOOL SetHook()
{

	g_hhook = SetWindowsHookEx(WH_CBT, Hook_Proc, module_self, 0);
	//filename_dir = (char*)malloc(300);
    //strcpy(log_directory,filename);
	if(g_hhook==NULL){
		return FALSE;
	}
	return TRUE;
}

BOOL UnHook()
{	
	return UnhookWindowsHookEx(g_hhook);
}

BOOL APIENTRY DllMain(HINSTANCE hModule, DWORD dwReason, PVOID lpReserved)
{	
	switch (dwReason) 
	{
    case DLL_PROCESS_ATTACH:
		module_self = hModule;
		break;
	case DLL_PROCESS_DETACH:
		break;		
	}
	return TRUE;
}