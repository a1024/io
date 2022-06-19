//io.c - A cross-platform window creation, event handling and immediate graphics library
//Copyright (C) 2022  Ayman Wagih, unless source link provided
//
//This program is free software: you can redistribute it and/or modify
//it under the terms of the GNU General Public License as published by
//the Free Software Foundation, either version 3 of the License, or
//(at your option) any later version.
//
//This program is distributed in the hope that it will be useful,
//but WITHOUT ANY WARRANTY; without even the implied warranty of
//MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
//GNU General Public License for more details.
//
//You should have received a copy of the GNU General Public License
//along with this program.  If not, see <https://www.gnu.org/licenses/>.

#define				IO_IMPLEMENTATION
#include			"io.h"
#include			<stdio.h>
#include			<string.h>
#include			<stdarg.h>
#include			<math.h>
#include			<tmmintrin.h>
#define				STB_IMAGE_IMPLEMENTATION
#include			"stb_image.h"
#ifdef _MSC_VER
#define		HUGE_VAL	_HUGE
#else
#define		sprintf_s	snprintf
#define		vsprintf_s	vsnprintf
#endif

int					w=0, h=0, mx=0, my=0;
char				timer=0, keyboard[256]={0};
static char			mouse_bypass=0;

char				g_buf[g_buf_size]={0};
#if defined _MSC_VER|| defined _WIN32 || defined _WINDOWS
wchar_t				g_wbuf[g_buf_size]={0};
#endif
char				*exe_dir=0;
int					exe_dir_len=0;

//OpenGL globals
const char			*GLversion=0;
int					rx0=0, ry0=0, rdx=0, rdy=0;//current OpenGL region
float
	SN_x0=0, SN_x1=0, SN_y0=0, SN_y1=0,//screen-NDC conversion
	NS_x0=0, NS_x1=0, NS_y0=0, NS_y1=0;
extern unsigned		vertex_buffer;

//text globals
char				sdf_available=0, sdf_active=0;
short				tab_count=8;
float				tdx=0, tdy=0;//non-tab character dimensions
float				sdf_dx=0, sdf_dy=0, sdf_txh=0;
float				font_zoom=1, font_zoom_min=1, font_zoom_max=32, sdf_dzoom=1.02f, sdf_slope=0.062023f;
typedef struct		QuadCoordsStruct
{
	float x1, x2, y1, y2;
} QuadCoords;
QuadCoords			font_coords[128-32]={0}, sdf_glyph_coords[128-32]={0};
unsigned			font_txid=0, sdf_atlas_txid=0;
typedef struct		SDFTextureHeaderStruct
{
	double slope;
	char
		grid_start_x, grid_start_y,
		cell_size_x, cell_size_y,
		csize_x, csize_y,
		reserved[2];
} SDFTextureHeader;
long long			colors_text=0xFFABABABFF000000;//0xBKBKBKBK_TXTXTXTX

//math constants
float
	_pi, _2pi, pi_2, inv_2pi,
	sqrt2,
	torad, todeg,
	infinity,
	inv255;
static void			init_math_constants()
{
	_pi=acosf(-1.f), _2pi=2*_pi, pi_2=_pi*0.5f, inv_2pi=1/_2pi;
	sqrt2=sqrtf(2.f);
	torad=_pi/180, todeg=180/_pi;
	infinity=(float)HUGE_VAL;
	inv255=1.f/255;
}

//shader declarations
#define		ATTR(NAME, LABEL)	int a_##NAME##_##LABEL=-1;
#define		SHADER(NAME)		ATTR_##NAME
SHADER_LIST
#undef		SHADER
#undef		ATTR

#define		UNIF(NAME, LABEL)	int u_##NAME##_##LABEL=-1;
#define		SHADER(NAME)		UNIF_##NAME
SHADER_LIST
#undef		SHADER
#undef		UNIF

#define		ATTR(NAME, LABEL)	{&a_##NAME##_##LABEL, "a_" #LABEL},
#define		SHADER(NAME)		ShaderVar attr_##NAME[]={ATTR_##NAME};
SHADER_LIST
#undef		SHADER
#undef		ATTR

#define		UNIF(NAME, LABEL)	{&u_##NAME##_##LABEL, "u_" #LABEL},
#define		SHADER(NAME)		ShaderVar unif_##NAME[]={UNIF_##NAME};
SHADER_LIST
#undef		SHADER
#undef		UNIF

#define		SHADER(NAME)		ShaderProgram shader_##NAME={"shader_" #NAME, src_vert_##NAME, src_frag_##NAME, attr_##NAME, unif_##NAME, SIZEOF(attr_##NAME), SIZEOF(unif_##NAME), 0};
SHADER_LIST
#undef		SHADER

int					init_gl();//returns 0 on error to exit application
void				memswap(void *p1, void *p2, size_t size)
{
	unsigned char *s1=(unsigned char*)p1, *s2=(unsigned char*)p2, *end=s1+size;
	for(;s1<end;++s1, ++s2)
	{
		const unsigned char t=*s1;
		*s1=*s2;
		*s2=t;
	}
}
void				memfill(void *dst, const void *src, size_t dstbytes, size_t srcbytes)
{
	unsigned copied;
	char *d=(char*)dst;
	const char *s=(const char*)src;
	if(dstbytes<srcbytes)
	{
		memcpy(dst, src, dstbytes);
		return;
	}
	copied=srcbytes;
	memcpy(d, s, copied);
	while(copied<<1<=dstbytes)
	{
		memcpy(d+copied, d, copied);
		copied<<=1;
	}
	if(copied<dstbytes)
		memcpy(d+copied, d, dstbytes-copied);
}

//math
int					mod(int x, int n)
{
	x%=n;
	x+=n&-(x<0);
	return x;
}
float				modulof(float x, float n){return x-floorf(x/n)*n;}
int					floor_log2(unsigned n)
{
	int logn=0;
	int sh=(((short*)&n)[1]!=0)<<4;	logn^=sh, n>>=sh;	//21.54
		sh=(((char*)&n)[1]!=0)<<3;	logn^=sh, n>>=sh;
		sh=((n&0x000000F0)!=0)<<2;	logn^=sh, n>>=sh;
		sh=((n&0x0000000C)!=0)<<1;	logn^=sh, n>>=sh;
		sh=((n&0x00000002)!=0);		logn^=sh;
	return logn;
}
int					minimum(int a, int b){return (a+b-abs(a-b))>>1;}
int					maximum(int a, int b){return (a+b+abs(a-b))>>1;}
int					clamp(int lo, int x, int hi)
{
	if(x<lo)
		x=lo;
	if(x>hi)
		x=hi;
	return x;
}
float				clampf(float lo, float x, float hi)
{
	if(x<lo)
		x=lo;
	if(x>hi)
		x=hi;
	return x;
}

//error handling
char				first_error_msg[g_buf_size]={0}, latest_error_msg[g_buf_size]={0};
int					log_error(const char *file, int line, const char *format, ...)
{
	int firsttime=first_error_msg[0]=='\0';

	int size=strlen(file), start=size-1;
	for(;start>=0&&file[start]!='/'&&file[start]!='\\';--start);
	start+=start==-1||file[start]=='/'||file[start]=='\\';

	int printed=sprintf_s(latest_error_msg, g_buf_size, "%s(%d): ", file+start, line);
	va_list args;
	va_start(args, format);
	printed+=vsprintf_s(latest_error_msg+printed, g_buf_size-printed, format, args);
	va_end(args);

	if(firsttime)
		memcpy(first_error_msg, latest_error_msg, printed+1);
	messagebox(MBOX_OK, "Error", latest_error_msg);
	return firsttime;
}
int					valid(const void *p)
{
	switch((size_t)p)
	{
	case 0:
	case 0xCCCCCCCC:
	case 0xFEEEFEEE:
	case 0xEEFEEEFE:
	case 0xCDCDCDCD:
	case 0xFDFDFDFD:
	case 0xBAAD0000:
		return 0;
	}
	return 1;
}
const char*			glerr2str(int error);
void 				gl_check(const char *file, int line);
void				gl_error(const char *file, int line);
#ifdef NO_GL_CHECK
#define				GL_CHECK()
#else
#define				GL_CHECK()		gl_check(file, __LINE__)
#endif
#define				GL_ERROR()		gl_error(file, __LINE__)

void				console_pause()
{
	char c=0;
	scanf("%c", &c);
}

#if defined _MSC_VER || defined _WIN32 || defined _WINDOWS
#include			<Windows.h>
#include			<Windowsx.h>//for GET_X_LPARAM
#include			<GL/gl.h>

#include			<io.h>//for console
#include			<fcntl.h>

#pragma				comment(lib, "OpenGL32.lib")
const char			file[]=__FILE__;
HWND				ghWnd=0;
RECT				R={0};
HDC					ghDC=0;
HGLRC				hRC=0;

double				time_ms()
{
	static double inv_f=0;
	LARGE_INTEGER li;
	//if(!inv_f)
	//{
		QueryPerformanceFrequency(&li);
		inv_f=1/(double)li.QuadPart;
	//}
	QueryPerformanceCounter(&li);
	return 1000.*(double)li.QuadPart*inv_f;
}
void				set_window_title(const char *format, ...)
{
	va_list args;
	va_start(args, format);
	vsnprintf(g_buf, g_buf_size, format, args);
	va_end(args);
	SetWindowTextA(ghWnd, g_buf);
}
void				set_mouse(int x, int y)
{
	POINT p={x, y};
	ClientToScreen(ghWnd, &p);
	SetCursorPos(p.x, p.y);
	mouse_bypass=1;
}
void				get_mouse(int *px, int *py)
{
	POINT p;
	GetCursorPos(&p);
	ScreenToClient(ghWnd, &p);
	*px=p.x, *py=p.y;
}
void				show_mouse(int show)
{
	ShowCursor(show);
}

int					console_active=0;
void				console_show()//https://stackoverflow.com/questions/191842/how-do-i-get-console-output-in-c-with-a-windows-program
{
	if(!console_active)
	{
		console_active=1;
		int hConHandle;
		long lStdHandle;
		CONSOLE_SCREEN_BUFFER_INFO coninfo;
		FILE *fp;

		//allocate a console for this app
		AllocConsole();

		//set the screen buffer to be big enough to let us scroll text
		GetConsoleScreenBufferInfo(GetStdHandle(STD_OUTPUT_HANDLE), &coninfo);
		coninfo.dwSize.Y=4000;
		SetConsoleScreenBufferSize(GetStdHandle(STD_OUTPUT_HANDLE), coninfo.dwSize);

		//redirect unbuffered STDOUT to the console
		lStdHandle=(long)GetStdHandle(STD_OUTPUT_HANDLE);
		hConHandle=_open_osfhandle(lStdHandle, _O_TEXT);
		fp=_fdopen(hConHandle, "w");
		*stdout=*fp;
		setvbuf(stdout, 0, _IONBF, 0);

		//redirect unbuffered STDIN to the console
		lStdHandle=(long)GetStdHandle(STD_INPUT_HANDLE);
		hConHandle=_open_osfhandle(lStdHandle, _O_TEXT);
		fp=_fdopen(hConHandle, "r");
		*stdin=*fp;
		setvbuf(stdin, 0, _IONBF, 0);

		//redirect unbuffered STDERR to the console
		lStdHandle=(long)GetStdHandle(STD_ERROR_HANDLE);
		hConHandle=_open_osfhandle(lStdHandle, _O_TEXT);
		fp=_fdopen(hConHandle, "w");
		*stderr=*fp;
		setvbuf(stderr, 0, _IONBF, 0);

		//make cout, wcout, cin, wcin, wcerr, cerr, wclog and clog point to console as well
#ifdef __cplusplus
		std::ios::sync_with_stdio();
#endif

		printf("\n\tWARNING: CLOSING THIS WINDOW WILL CLOSE THE PROGRAM\n\n");
	}
}
void				console_hide()
{
	if(console_active)
	{
		FreeConsole();
		console_active=0;
	}
}

int					sys_check(const char *file, int line, const char *info)
{
	int error=GetLastError();
	if(error)
	{
		char *messageBuffer=0;
		size_t size=FormatMessageA(FORMAT_MESSAGE_ALLOCATE_BUFFER|FORMAT_MESSAGE_FROM_SYSTEM|FORMAT_MESSAGE_IGNORE_INSERTS, NULL, error, MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT), (LPSTR)&messageBuffer, 0, NULL);
		log_error(file, line, "%s%sGetLastError() returned %d: %s", info?info:"", info?"\n":"", error, messageBuffer);
		LocalFree(messageBuffer);
	}
	return 0;
}
#define				SYS_ASSERT(SUCCESS)		((void)((SUCCESS)!=0||sys_check(file, __LINE__, 0)))

static int			format_utf8_message(const char *title, const char *format, char *args)//returns idx of title in g_wbuf
{
	int len=vsprintf_s(g_buf, g_buf_size, format, args);
	len=MultiByteToWideChar(CP_UTF8, 0, g_buf, len, g_wbuf, g_buf_size);	SYS_ASSERT(len);
	g_wbuf[len]='\0';
	++len;
	int len2=MultiByteToWideChar(CP_UTF8, 0, title, strlen(title), g_wbuf+len, g_buf_size-len);	SYS_ASSERT(len2);
	g_wbuf[len+len2]='\0';
	return len;
}
int					messagebox(MessageBoxType type, const char *title, const char *format, ...)//returns index of pressed button
{
	int len=format_utf8_message(title, format, (char*)(&format+1));
	int wintypes[]={MB_OK, MB_OKCANCEL, MB_YESNOCANCEL};
	int result=MessageBoxW(ghWnd, g_wbuf, g_wbuf+len, wintypes[type]);
	switch(type)
	{
	case MBOX_OK:result=0;
	case MBOX_OKCANCEL:
		switch(result)
		{
		case IDOK:
			result=0;
			break;
		case IDCANCEL:
		default:
			result=1;
			break;
		}
		break;
	case MBOX_YESNOCANCEL:
		switch(result)
		{
		case IDYES:		result=0;	break;
		case IDNO:		result=1;	break;
		default:
		case IDCANCEL:	result=2;	break;
		}
		break;
	}
	return result;
}

#define				UTF8TOWCHAR(U8, LEN, RET_U16, RET_BUF_SIZE, RET_LEN)	RET_LEN=MultiByteToWideChar(CP_UTF8, 0, U8, LEN, RET_U16, RET_BUF_SIZE)
//const wchar_t*		multibyte2wide(const char *str, int len, int *ret_len)
//{
//	int len2=MultiByteToWideChar(CP_UTF8, 0, str, strlen(str), g_wbuf, g_buf_size);	SYS_ASSERT(len2);
//	if(ret_len)
//		*ret_len=len2;
//	return g_wbuf;
//}
const char*			dialog_open_folder(int multiple)//C++?
{
	HRESULT hr=OleInitialize(0);
	if(hr!=S_OK)
	{
		OleUninitialize();
		return false;
	}
	IFileOpenDialog *pFileOpenDialog=0;
	IShellItem *pShellItem=0;
	LPWSTR fullpath=0;
	hr=CoCreateInstance(CLSID_FileOpenDialog, 0, CLSCTX_INPROC_SERVER, IID_PPV_ARGS(&pFileOpenDialog));
	bool success=false;
	if(SUCCEEDED(hr))
	{
		hr=pFileOpenDialog->SetOptions(FOS_PICKFOLDERS|FOS_FORCEFILESYSTEM);
		hr=pFileOpenDialog->Show(0);
		success=SUCCEEDED(hr);
		if(success)
		{
			hr=pFileOpenDialog->GetResult(&pShellItem);
			pShellItem->GetDisplayName(SIGDN_FILESYSPATH, &fullpath);
			path=fullpath;
			CoTaskMemFree(fullpath);
		}
		pFileOpenDialog->Release();
	}
	OleUninitialize();
	return success;
}
const char*			dialog_open_file(Filter *filters, int nfilters, int multiple)//TODO: multiple
{
	ArrayHandle winfilts=0;
	ARRAY_ALLOC(wchar_t, winfilts, 0, 1);
	for(int k=0;k<nfilters;++k)
	{
		int len=0;

		UTF8TOWCHAR(filters[k].comment, strlen(filters[k].comment), g_wbuf, g_buf_size, len);
		//const wchar_t *str=multibyte2wide(filters[k].comment, strlen(filters[k].comment), &len);
		if(!len)
			break;
		ARRAY_APPEND(winfilts, g_wbuf, len, 1, 1);

		UTF8TOWCHAR(filters[k].ext, strlen(filters[k].ext), g_wbuf, g_buf_size, len);
		//str=multibyte2wide(filters[k].ext, strlen(filters[k].ext), &len);
		if(!len)
			break;
		ARRAY_APPEND(winfilts, str, len, 1, 1);
	}

	g_wbuf[0]=0;
	OPENFILENAMEW ofn=
	{
		sizeof(OPENFILENAMEW),
		ghWnd, ghInstance,
		&WSTR_AT(winfilts), 0, 0, 1,
		g_wbuf, g_buf_size,
		0, 0,//initial filename
		0,
		0,//dialog title
		OFN_CREATEPROMPT|OFN_PATHMUSTEXIST,
		0,//file offset
		0,//extension offset
		L"txt",//default extension
		0, 0,//data & hook
		0,//template name
		0,//reserved
		0,//reserved
		0,//flags ex
	};
	int success=GetOpenFileNameW(&ofn);
	array_free(&winfilts);
	if(!success)
		return 0;

	int len=WideCharToMultiByte(CP_UTF8, 0, ofn.lpstrFile, wcslen(ofn.lpstrFile), g_buf, g_buf_size, 0, 0);	SYS_ASSERT(len);
	if(!len)
		return 0;
	g_buf[len]='\0';
	return g_buf;
}
const wchar_t		initialname[]=L"Untitled.txt";
const char*			dialog_save_file(Filter *filters, int nfilters)
{
	memcpy(g_wbuf, initialname, sizeof(initialname));
	//g_wbuf[0]=0;
	OPENFILENAMEW ofn=
	{
		sizeof(OPENFILENAMEW), ghWnd, ghInstance,
		
		L"Text File (.txt)\0*.txt\0"	//<- filter
		L"C++ Source (.cpp; .cc; .cxx)\0*.cpp\0"
		L"C Source (.c)\0*.c\0"
		L"C/C++ Header (.h; .hpp)\0*.h\0",
	//	L"No Extension\0*\0",
		
		0, 0,//custom filter & count
		1,								//<- initial filter index
		g_wbuf, g_buf_size,				//<- output filename
		0, 0,//initial filename
		0,
		0,//dialog title
		OFN_NOTESTFILECREATE|OFN_PATHMUSTEXIST|OFN_EXTENSIONDIFFERENT|OFN_OVERWRITEPROMPT,
		0, 8,							//<- file offset & extension offset
		L"txt",							//<- default extension (if user didn't type one)
		0, 0,//data & hook
		0,//template name
		0, 0,//reserved
		0,//flags ex
	};
	int success=GetSaveFileNameW(&ofn);
	if(!success)
		return 0;

	//char c[]="?";
	//int invalid=false;
	int len=WideCharToMultiByte(CP_UTF8, 0, ofn.lpstrFile, wcslen(ofn.lpstrFile), g_buf, g_buf_size, 0, 0);	SYS_ASSERT(len);
	if(!len)
		return 0;
	g_buf[len]='\0';
	return g_buf;
	//memcpy(g_wbuf, ofn.lpstrFile, wcslen(ofn.lpstrFile)*sizeof(wchar_t));
	//return g_wbuf;
}

void				copy_to_clipboard_c(const char *a, int size)//size not including null terminator
{
	char *clipboard=(char*)LocalAlloc(LMEM_FIXED, (size+1)*sizeof(char));
	memcpy(clipboard, a, (size+1)*sizeof(char));
	clipboard[size]='\0';
	OpenClipboard(ghWnd);
	EmptyClipboard();
	SetClipboardData(CF_OEMTEXT, (void*)clipboard);
	CloseClipboard();
}
char*				paste_from_clipboard(int loud, int *ret_len)
{
	OpenClipboard(ghWnd);
	char *a=(char*)GetClipboardData(CF_OEMTEXT);
	if(!a)
	{
		CloseClipboard();
		if(loud)
			messagebox(MBOX_OK, "Error", "Failed to paste from clipboard");
		return 0;
	}
	int len0=strlen(a);

	char *str=(char*)malloc(len0+1);
	if(!str)
		LOG_ERROR("paste_from_clipboard: malloc(%d) returned 0", len0+1);
	int len=0;
	for(int k2=0;k2<len0;++k2)
	{
		if(a[k2]!='\r')
			str[len]=a[k2], ++len;
	}
	str[len]='\0';

	CloseClipboard();
	if(ret_len)
		*ret_len=len;
	return str;
}

int					error_fatal=0;
#if 1
void				(__stdcall *glBlendEquation)(unsigned mode)=0;
//void				(__stdcall *glGenVertexArrays)(int n, unsigned *arrays)=0;//OpenGL 3.0
//void				(__stdcall *glDeleteVertexArrays)(int n, unsigned *arrays)=0;//OpenGL 3.0
void				(__stdcall *glBindVertexArray)(unsigned arr)=0;
void				(__stdcall *glGenBuffers)(int n, unsigned *buffers)=0;
void				(__stdcall *glBindBuffer)(unsigned target, unsigned buffer)=0;
void				(__stdcall *glBufferData)(unsigned target, int size, const void *data, unsigned usage)=0;
void				(__stdcall *glBufferSubData)(unsigned target, int offset, int size, const void *data)=0;
void				(__stdcall *glEnableVertexAttribArray)(unsigned index)=0;
void				(__stdcall *glVertexAttribPointer)(unsigned index, int size, unsigned type, unsigned char normalized, int stride, const void *pointer)=0;
void				(__stdcall *glDisableVertexAttribArray)(unsigned index)=0;
unsigned			(__stdcall *glCreateShader)(unsigned shaderType)=0;
void				(__stdcall *glShaderSource)(unsigned shader, int count, const char **string, const int *length)=0;
void				(__stdcall *glCompileShader)(unsigned shader)=0;
void				(__stdcall *glGetShaderiv)(unsigned shader, unsigned pname, int *params)=0;
void				(__stdcall *glGetShaderInfoLog)(unsigned shader, int maxLength, int *length, char *infoLog)=0;
unsigned			(__stdcall *glCreateProgram)()=0;
void				(__stdcall *glAttachShader)(unsigned program, unsigned shader)=0;
void				(__stdcall *glLinkProgram)(unsigned program)=0;
void				(__stdcall *glGetProgramiv)(unsigned program, unsigned pname, int *params)=0;
void				(__stdcall *glGetProgramInfoLog)(unsigned program, int maxLength, int *length, char *infoLog)=0;
void				(__stdcall *glDetachShader)(unsigned program, unsigned shader)=0;
void				(__stdcall *glDeleteShader)(unsigned shader)=0;
void				(__stdcall *glUseProgram)(unsigned program)=0;
int					(__stdcall *glGetAttribLocation)(unsigned program, const char *name)=0;
void				(__stdcall *glDeleteProgram)(unsigned program)=0;
void				(__stdcall *glDeleteBuffers)(int n, const unsigned *buffers)=0;
int					(__stdcall *glGetUniformLocation)(unsigned program, const char *name)=0;
void				(__stdcall *glUniformMatrix3fv)(int location, int count, unsigned char transpose, const float *value)=0;
void				(__stdcall *glUniformMatrix4fv)(int location, int count, unsigned char transpose, const float *value)=0;
void				(__stdcall *glGetBufferParameteriv)(unsigned target, unsigned value, int *data)=0;
void				(__stdcall *glActiveTexture)(unsigned texture)=0;
void				(__stdcall *glUniform1i)(int location, int v0)=0;
void				(__stdcall *glUniform2i)(int location, int v0, int v1)=0;
void				(__stdcall *glUniform1f)(int location, float v0)=0;
void				(__stdcall *glUniform2f)(int location, float v0, float v1)=0;
void				(__stdcall *glUniform3f)(int location, float v0, float v1, float v2)=0;
void				(__stdcall *glUniform3fv)(int location, int count, const float *value)=0;
void				(__stdcall *glUniform4f)(int location, float v0, float v1, float v2, float v3)=0;
void				(__stdcall *glUniform4fv)(int location, int count, float *value)=0;
#endif

const char*			wm2str(int message)
{
	const char *a="???";
	switch(message)
	{//message case
#define		MC(x)	case x:a=#x;break;
	MC(WM_NULL)
	MC(WM_CREATE)
	MC(WM_DESTROY)
	MC(WM_MOVE)
	MC(WM_SIZE)

	MC(WM_ACTIVATE)
	MC(WM_SETFOCUS)
	MC(WM_KILLFOCUS)
	MC(WM_ENABLE)
	MC(WM_SETREDRAW)
	MC(WM_SETTEXT)
	MC(WM_GETTEXT)
	MC(WM_GETTEXTLENGTH)
	MC(WM_PAINT)
	MC(WM_CLOSE)

	MC(WM_QUERYENDSESSION)
	MC(WM_QUERYOPEN)
	MC(WM_ENDSESSION)

	MC(WM_QUIT)
	MC(WM_ERASEBKGND)
	MC(WM_SYSCOLORCHANGE)
	MC(WM_SHOWWINDOW)
	MC(WM_WININICHANGE)
//	MC(WM_SETTINGCHANGE)//==WM_WININICHANGE

	MC(WM_DEVMODECHANGE)
	MC(WM_ACTIVATEAPP)
	MC(WM_FONTCHANGE)
	MC(WM_TIMECHANGE)
	MC(WM_CANCELMODE)
	MC(WM_SETCURSOR)
	MC(WM_MOUSEACTIVATE)
	MC(WM_CHILDACTIVATE)
	MC(WM_QUEUESYNC)

	MC(WM_GETMINMAXINFO)

	MC(WM_PAINTICON)
	MC(WM_ICONERASEBKGND)
	MC(WM_NEXTDLGCTL)
	MC(WM_SPOOLERSTATUS)
	MC(WM_DRAWITEM)
	MC(WM_MEASUREITEM)
	MC(WM_DELETEITEM)
	MC(WM_VKEYTOITEM)
	MC(WM_CHARTOITEM)
	MC(WM_SETFONT)
	MC(WM_GETFONT)
	MC(WM_SETHOTKEY)
	MC(WM_GETHOTKEY)
	MC(WM_QUERYDRAGICON)
	MC(WM_COMPAREITEM)

	MC(WM_GETOBJECT)

	MC(WM_COMPACTING)
	MC(WM_COMMNOTIFY)
	MC(WM_WINDOWPOSCHANGING)
	MC(WM_WINDOWPOSCHANGED)

	MC(WM_POWER)

	MC(WM_COPYDATA)
	MC(WM_CANCELJOURNAL)

	MC(WM_NOTIFY)
	MC(WM_INPUTLANGCHANGEREQUEST)
	MC(WM_INPUTLANGCHANGE)
	MC(WM_TCARD)
	MC(WM_HELP)
	MC(WM_USERCHANGED)
	MC(WM_NOTIFYFORMAT)

	MC(WM_CONTEXTMENU)
	MC(WM_STYLECHANGING)
	MC(WM_STYLECHANGED)
	MC(WM_DISPLAYCHANGE)
	MC(WM_GETICON)
	MC(WM_SETICON)

	MC(WM_NCCREATE)
	MC(WM_NCDESTROY)
	MC(WM_NCCALCSIZE)
	MC(WM_NCHITTEST)
	MC(WM_NCPAINT)
	MC(WM_NCACTIVATE)
	MC(WM_GETDLGCODE)

	MC(WM_SYNCPAINT)

	MC(WM_NCMOUSEMOVE)
	MC(WM_NCLBUTTONDOWN)
	MC(WM_NCLBUTTONUP)
	MC(WM_NCLBUTTONDBLCLK)
	MC(WM_NCRBUTTONDOWN)
	MC(WM_NCRBUTTONUP)
	MC(WM_NCRBUTTONDBLCLK)
	MC(WM_NCMBUTTONDOWN)
	MC(WM_NCMBUTTONUP)
	MC(WM_NCMBUTTONDBLCLK)

	MC(WM_NCXBUTTONDOWN  )
	MC(WM_NCXBUTTONUP    )
	MC(WM_NCXBUTTONDBLCLK)

	MC(WM_INPUT_DEVICE_CHANGE)

	MC(WM_INPUT)

//	MC(WM_KEYFIRST   )//==WM_KEYDOWN
	MC(WM_KEYDOWN    )
	MC(WM_KEYUP      )
	MC(WM_CHAR       )
	MC(WM_DEADCHAR   )
	MC(WM_SYSKEYDOWN )
	MC(WM_SYSKEYUP   )
	MC(WM_SYSCHAR    )
	MC(WM_SYSDEADCHAR)

	MC(WM_UNICHAR)
//	MC(WM_KEYLAST)		//==WM_UNICHAR
	MC(UNICODE_NOCHAR)	//0xFFFF

	MC(WM_IME_STARTCOMPOSITION)
	MC(WM_IME_ENDCOMPOSITION)
	MC(WM_IME_COMPOSITION)
//	MC(WM_IME_KEYLAST)	//==WM_IME_KEYLAST

	MC(WM_INITDIALOG   )
	MC(WM_COMMAND      )
	MC(WM_SYSCOMMAND   )
	MC(WM_TIMER        )
	MC(WM_HSCROLL      )
	MC(WM_VSCROLL      )
	MC(WM_INITMENU     )
	MC(WM_INITMENUPOPUP)

	MC(WM_GESTURE      )
	MC(WM_GESTURENOTIFY)

	MC(WM_MENUSELECT)
	MC(WM_MENUCHAR  )
	MC(WM_ENTERIDLE )

	MC(WM_MENURBUTTONUP  )
	MC(WM_MENUDRAG       )
	MC(WM_MENUGETOBJECT  )
	MC(WM_UNINITMENUPOPUP)
	MC(WM_MENUCOMMAND    )

	MC(WM_CHANGEUISTATE)
	MC(WM_UPDATEUISTATE)
	MC(WM_QUERYUISTATE )

	MC(WM_CTLCOLORMSGBOX   )
	MC(WM_CTLCOLOREDIT     )
	MC(WM_CTLCOLORLISTBOX  )
	MC(WM_CTLCOLORBTN      )
	MC(WM_CTLCOLORDLG      )
	MC(WM_CTLCOLORSCROLLBAR)
	MC(WM_CTLCOLORSTATIC   )
	MC(MN_GETHMENU         )

//	MC(WM_MOUSEFIRST   )
	MC(WM_MOUSEMOVE    )
	MC(WM_LBUTTONDOWN  )
	MC(WM_LBUTTONUP    )
	MC(WM_LBUTTONDBLCLK)
	MC(WM_RBUTTONDOWN  )
	MC(WM_RBUTTONUP    )
	MC(WM_RBUTTONDBLCLK)
	MC(WM_MBUTTONDOWN  )
	MC(WM_MBUTTONUP    )
	MC(WM_MBUTTONDBLCLK)

	MC(WM_MOUSEWHEEL)

	MC(WM_XBUTTONDOWN  )
	MC(WM_XBUTTONUP    )
	MC(WM_XBUTTONDBLCLK)

//	MC(WM_MOUSELAST)	//==WM_MOUSEWHEEL

	MC(WM_PARENTNOTIFY )
	MC(WM_ENTERMENULOOP)
	MC(WM_EXITMENULOOP )

	MC(WM_NEXTMENU      )
	MC(WM_SIZING        )
	MC(WM_CAPTURECHANGED)
	MC(WM_MOVING        )

	MC(WM_POWERBROADCAST)

	MC(WM_DEVICECHANGE)

	MC(WM_MDICREATE     )
	MC(WM_MDIDESTROY    )
	MC(WM_MDIACTIVATE   )
	MC(WM_MDIRESTORE    )
	MC(WM_MDINEXT       )
	MC(WM_MDIMAXIMIZE   )
	MC(WM_MDITILE       )
	MC(WM_MDICASCADE    )
	MC(WM_MDIICONARRANGE)
	MC(WM_MDIGETACTIVE  )

	MC(WM_MDISETMENU    )
	MC(WM_ENTERSIZEMOVE )
	MC(WM_EXITSIZEMOVE  )
	MC(WM_DROPFILES     )
	MC(WM_MDIREFRESHMENU)

//	MC(WM_POINTERDEVICECHANGE    )
//	MC(WM_POINTERDEVICEINRANGE   )
//	MC(WM_POINTERDEVICEOUTOFRANGE)

	MC(WM_TOUCH)

//	MC(WM_NCPOINTERUPDATE      )
//	MC(WM_NCPOINTERDOWN        )
//	MC(WM_NCPOINTERUP          )
//	MC(WM_POINTERUPDATE        )
//	MC(WM_POINTERDOWN          )
//	MC(WM_POINTERUP            )
//	MC(WM_POINTERENTER         )
//	MC(WM_POINTERLEAVE         )
//	MC(WM_POINTERACTIVATE      )
//	MC(WM_POINTERCAPTURECHANGED)
//	MC(WM_TOUCHHITTESTING      )
//	MC(WM_POINTERWHEEL         )
//	MC(WM_POINTERHWHEEL        )
//	MC(DM_POINTERHITTEST       )

	MC(WM_IME_SETCONTEXT     )
	MC(WM_IME_NOTIFY         )
	MC(WM_IME_CONTROL        )
	MC(WM_IME_COMPOSITIONFULL)
	MC(WM_IME_SELECT         )
	MC(WM_IME_CHAR           )

	MC(WM_IME_REQUEST)

	MC(WM_IME_KEYDOWN)
	MC(WM_IME_KEYUP  )

	MC(WM_MOUSEHOVER)
	MC(WM_MOUSELEAVE)

	MC(WM_NCMOUSEHOVER)
	MC(WM_NCMOUSELEAVE)

	MC(WM_WTSSESSION_CHANGE)

	MC(WM_TABLET_FIRST)
	MC(WM_TABLET_LAST )

//	MC(WM_DPICHANGED)

	MC(WM_CUT              )
	MC(WM_COPY             )
	MC(WM_PASTE            )
	MC(WM_CLEAR            )
	MC(WM_UNDO             )
	MC(WM_RENDERFORMAT     )
	MC(WM_RENDERALLFORMATS )
	MC(WM_DESTROYCLIPBOARD )
	MC(WM_DRAWCLIPBOARD    )
	MC(WM_PAINTCLIPBOARD   )
	MC(WM_VSCROLLCLIPBOARD )
	MC(WM_SIZECLIPBOARD    )
	MC(WM_ASKCBFORMATNAME  )
	MC(WM_CHANGECBCHAIN    )
	MC(WM_HSCROLLCLIPBOARD )
	MC(WM_QUERYNEWPALETTE  )
	MC(WM_PALETTEISCHANGING)
	MC(WM_PALETTECHANGED   )
	MC(WM_HOTKEY           )

	MC(WM_PRINT      )
	MC(WM_PRINTCLIENT)

	MC(WM_APPCOMMAND)

	MC(WM_THEMECHANGED)

	MC(WM_CLIPBOARDUPDATE)

	MC(WM_DWMCOMPOSITIONCHANGED      )
	MC(WM_DWMNCRENDERINGCHANGED      )
	MC(WM_DWMCOLORIZATIONCOLORCHANGED)
	MC(WM_DWMWINDOWMAXIMIZEDCHANGE   )

	MC(WM_DWMSENDICONICTHUMBNAIL        )
	MC(WM_DWMSENDICONICLIVEPREVIEWBITMAP)

	MC(WM_GETTITLEBARINFOEX)

	MC(WM_HANDHELDFIRST)
	MC(WM_HANDHELDLAST )

	MC(WM_AFXFIRST)
	MC(WM_AFXLAST )

	MC(WM_PENWINFIRST)
	MC(WM_PENWINLAST )

	MC(WM_APP)

	MC(WM_USER)
#undef		MC
	}
	return a;
}
int					get_key_state(int key)
{
	short result=GetAsyncKeyState(key);
	keyboard[key]=(result>>15)!=0;
	return keyboard[key]!=0;
}
void				update_main_key_states()
{
	keyboard[VK_CONTROL]=get_key_state(VK_LCONTROL)<<1|get_key_state(VK_RCONTROL);
	keyboard[VK_SHIFT]=get_key_state(VK_LSHIFT)<<1|get_key_state(VK_RSHIFT);
	keyboard[VK_MENU]=get_key_state(VK_LMENU)<<1|get_key_state(VK_RMENU);
}
void				timer_start()
{
	if(!timer)
		SetTimer(ghWnd, 0, 10, 0), timer=1;
}
void				timer_stop()
{
	if(timer)
		KillTimer(ghWnd, 0), timer=0;
}
long __stdcall		WndProc(HWND hWnd, unsigned int message, unsigned int wParam, long lParam)
{
	switch(message)
	{
	case WM_SIZE:
		GetClientRect(ghWnd, &R);
		w=R.right-R.left, h=R.bottom-R.top;
		set_region_immediate(0, w, 0, h);
		io_resize();
		break;
	case WM_TIMER:
		io_timer();
		io_render();
		prof_print();
		SwapBuffers(ghDC);
		break;
	case WM_PAINT:
		io_render();
		prof_print();
		SwapBuffers(ghDC);
		break;
	case WM_ACTIVATE:
		update_main_key_states();
		break;

	case WM_MOUSEMOVE:
		mx=GET_X_LPARAM(lParam), my=GET_Y_LPARAM(lParam);
		if(mouse_bypass)
			mouse_bypass=0;
		else
		{
			if(io_mousemove())
				InvalidateRect(hWnd, 0, 0);
		}
		break;
	case WM_MOUSEWHEEL:
		if(io_mousewheel(GET_Y_LPARAM(wParam)))
			InvalidateRect(hWnd, 0, 0);
		break;

	case WM_LBUTTONDOWN:
	case WM_LBUTTONDBLCLK:
		if(io_keydn(KEY_LBUTTON, message==WM_LBUTTONDBLCLK))
			InvalidateRect(hWnd, 0, 0);
		keyboard[VK_LBUTTON]=1;
		break;
	case WM_LBUTTONUP:
		if(io_keyup(KEY_LBUTTON, 0))
			InvalidateRect(hWnd, 0, 0);
		keyboard[VK_LBUTTON]=0;
		break;
		
	case WM_MBUTTONDOWN:
	case WM_MBUTTONDBLCLK:
		if(io_keydn(KEY_MBUTTON, message==WM_LBUTTONDBLCLK))
			InvalidateRect(hWnd, 0, 0);
		keyboard[VK_MBUTTON]=1;
		break;
	case WM_MBUTTONUP:
		if(io_keyup(KEY_MBUTTON, 0))
			InvalidateRect(hWnd, 0, 0);
		keyboard[VK_MBUTTON]=0;
		break;
		
	case WM_RBUTTONDOWN:
	case WM_RBUTTONDBLCLK:
		if(io_keydn(KEY_RBUTTON, message==WM_LBUTTONDBLCLK))
			InvalidateRect(hWnd, 0, 0);
		keyboard[VK_RBUTTON]=1;
		break;
	case WM_RBUTTONUP:
		if(io_keyup(KEY_RBUTTON, 0))
			InvalidateRect(hWnd, 0, 0);
		keyboard[VK_RBUTTON]=0;
		break;
		
	case WM_KEYDOWN:
	case WM_SYSKEYDOWN:
		if(wParam==KEY_F4&&keyboard[KEY_ALT])
		{
			if(!io_quit_request())
				return 0;
			PostQuitMessage(0);
		}
		else if(io_keydn(wParam, 0))
			InvalidateRect(hWnd, 0, 0);
		keyboard[wParam]=1;
		break;
	case WM_KEYUP:
	case WM_SYSKEYUP:
		if(io_keyup(wParam, 0))
			InvalidateRect(hWnd, 0, 0);
		keyboard[wParam]=0;
		update_main_key_states();
		break;

	case WM_CLOSE:
		if(!io_quit_request())
			return 0;
		PostQuitMessage(0);
		break;
	}
	return DefWindowProcA(hWnd, message, wParam, lParam);
}
int __stdcall		WinMain(HINSTANCE hInstance, HINSTANCE hPrev, char *pCmdLine, int nCmdShow)
{
	WNDCLASSEXA wndClassEx=
	{
		sizeof(WNDCLASSEXA), CS_HREDRAW|CS_VREDRAW|CS_DBLCLKS,
		WndProc, 0, 0, hInstance,
		LoadIconA(0, (char*)0x00007F00),
		LoadCursorA(0, (char*)0x00007F00),

		0,
	//	(HBRUSH)(COLOR_WINDOW+1),

		0, "New format", 0
	};
	MSG msg;
	int ret=0;
	
	int len=GetModuleFileNameW(0, g_wbuf, g_buf_size);
	int len2=WideCharToMultiByte(CP_UTF8, 0, g_wbuf, len, g_buf, g_buf_size, 0, 0);
	int k=len2-1;
	for(;k>0&&g_buf[k-1]!='/'&&g_buf[k-1]!='\\';--k);
	exe_dir_len=k;
	exe_dir=(char*)malloc(k+1);
	memcpy(exe_dir, g_buf, k);
	exe_dir[k]='\0';
	for(k=0;k<exe_dir_len;++k)
		if(exe_dir[k]=='\\')
			exe_dir[k]='/';
	
	init_math_constants();

	short success=RegisterClassExA(&wndClassEx);	SYS_ASSERT(success);
	ghWnd=CreateWindowExA(WS_EX_ACCEPTFILES, wndClassEx.lpszClassName, "", WS_CAPTION|WS_SYSMENU|WS_THICKFRAME|WS_MINIMIZEBOX|WS_MAXIMIZEBOX|WS_CLIPCHILDREN, CW_USEDEFAULT, 0, CW_USEDEFAULT, 0, 0, 0, hInstance, 0);	SYS_ASSERT(ghWnd);//2022-06-11

	GetClientRect(ghWnd, &R);
	w=R.right-R.left, h=R.bottom-R.top;
	ghDC=GetDC(ghWnd);
	PIXELFORMATDESCRIPTOR pfd=
	{
		sizeof(PIXELFORMATDESCRIPTOR), 1,
		PFD_DRAW_TO_WINDOW|PFD_SUPPORT_OPENGL|PFD_DOUBLEBUFFER,
		PFD_TYPE_RGBA, 32,
		0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
		16,//depth bits
		0, 0, PFD_MAIN_PLANE, 0, 0, 0, 0
	};
	int PixelFormat=ChoosePixelFormat(ghDC, &pfd);
	SetPixelFormat(ghDC, PixelFormat, &pfd);
	hRC=wglCreateContext(ghDC);
	wglMakeCurrent(ghDC, hRC);

	GLversion=(const char*)glGetString(GL_VERSION);

	//load extended OpenGL API
#if 1
#pragma warning(push)
#pragma warning(disable:4113)
#define	GET_GL_FUNC(glFunc)				glFunc=wglGetProcAddress(#glFunc), (glFunc!=0||sys_check(file, __LINE__, #glFunc " == nullptr"))
#define	GET_GL_FUNC_UNCHECKED(glFunc)	glFunc=wglGetProcAddress(#glFunc)
	GET_GL_FUNC(glBlendEquation);
//	GET_GL_FUNC(glGenVertexArrays);//OpenGL 3.0
//	GET_GL_FUNC(glDeleteVertexArrays);//OpenGL 3.0
	GET_GL_FUNC(glBindVertexArray);
	GET_GL_FUNC(glGenBuffers);
	GET_GL_FUNC(glBindBuffer);
	GET_GL_FUNC(glBufferData);
	GET_GL_FUNC(glBufferSubData);
	GET_GL_FUNC(glEnableVertexAttribArray);
	GET_GL_FUNC(glVertexAttribPointer);
	GET_GL_FUNC(glDisableVertexAttribArray);
	GET_GL_FUNC(glCreateShader);
	GET_GL_FUNC(glShaderSource);
	GET_GL_FUNC(glCompileShader);
	GET_GL_FUNC(glGetShaderiv);
	GET_GL_FUNC(glGetShaderInfoLog);
	GET_GL_FUNC(glCreateProgram);
	GET_GL_FUNC(glAttachShader);
	GET_GL_FUNC(glLinkProgram);
	GET_GL_FUNC(glGetProgramiv);
	GET_GL_FUNC(glGetProgramInfoLog);
	GET_GL_FUNC(glDetachShader);
	GET_GL_FUNC(glDeleteShader);
	GET_GL_FUNC(glUseProgram);
	GET_GL_FUNC(glGetAttribLocation);
	GET_GL_FUNC(glDeleteProgram);
	GET_GL_FUNC(glDeleteBuffers);
	GET_GL_FUNC(glGetUniformLocation);
	GET_GL_FUNC(glUniformMatrix3fv);
	GET_GL_FUNC(glUniformMatrix4fv);
	GET_GL_FUNC(glGetBufferParameteriv);
	GET_GL_FUNC(glActiveTexture);
	GET_GL_FUNC(glUniform1i);
	GET_GL_FUNC(glUniform2i);
	GET_GL_FUNC(glUniform1f);
	GET_GL_FUNC(glUniform2f);
	GET_GL_FUNC(glUniform3f);
	GET_GL_FUNC(glUniform3fv);
	GET_GL_FUNC(glUniform4f);
	GET_GL_FUNC(glUniform4fv);
#undef	GET_GL_FUNC
#undef	GET_GL_FUNC_UNCHECKED
#pragma warning(pop)
#endif
	if(error_fatal)
		return EXIT_FAILURE;
	
	if(!init_gl())
		return EXIT_FAILURE;

	if(!io_init(__argc-1, __argv+1))
		return EXIT_FAILURE;

	ShowWindow(ghWnd, nCmdShow);

	for(;ret=GetMessageA(&msg, 0, 0, 0);)
	{
		if(ret==-1)
		{
			LOG_ERROR("GetMessage returned -1 with: hwnd=%08X, msg=%s, wP=%d, lP=%d. \nQuitting.", msg.hwnd, wm2str(msg.message), msg.wParam, msg.lParam);
			break;
		}
		TranslateMessage(&msg);
		DispatchMessageA(&msg);
	}

		//finish
		io_cleanup();
		ReleaseDC(ghWnd, ghDC);

	return msg.wParam;
}
#elif defined __linux__
//https://github.com/gamedevtech/X11OpenGLWindow/blob/master/X11.cpp
//g++ window.cpp -o window -lX11 -lGL
#include<stdio.h>
#include<string.h>
#include<stdarg.h>

#include<X11/Xlib.h>
#include<X11/Xutil.h>
#include<X11/keysymdef.h>

#include<gtk/gtk.h>

#define GL_GLEXT_PROTOTYPES
#include<GL/gl.h>
#include<GL/glx.h>

//#include<sys/time.h>
#include<time.h>
#include<unistd.h>
#include<errno.h>
#include<x86intrin.h>
const char file[]=__FILE__;

#define WINDOW_WIDTH	800
#define WINDOW_HEIGHT	600

Display	*display=0;
Window	window;
Screen	*screen=0;
int		screenId=0;
Atom	atomWmDeleteWindow;
GLXContext context=0;

double	time_ms()
{
	struct timespec t;
	clock_gettime(CLOCK_REALTIME, &t);
	return t.tv_sec*1000+t.tv_nsec*1e-6;
}
void	set_window_title(const char *format, ...)
{
	va_list args;
	va_start(args, format);
	vsnprintf(g_buf, g_buf_size, format, args);
	va_end(args);
	int ret=XStoreName(display, window, g_buf);
}
void	set_mouse(int x, int y)
{
	//printf("set_mouse (%d, %d) -> (%d, %d)\n", mx, my, x, y);//
	//XWarpPointer(display, None, window, 0, 0, 0, 0, x-mx, y-my);
	XWarpPointer(display, None, window, 0, 0, 0, 0, x, y);
	mouse_bypass=1;
}
void	get_mouse(int *px, int *py)
{
	Window ret_root=0, ret_child=0;
	int x2=0, y2=0;
	unsigned ret_mask=0;
	XQueryPointer(display, window, &ret_root, &ret_child, px, py, &x2, &y2, &ret_mask);
	XTranslateCoordinates(display, ret_root, window, *px, *py, px, py, &ret_child);
	//printf("Mouse is at (%d, %d)\n", *px, *py);//
}
void	show_mouse(int show)
{
	//TODO
	//https://stackoverflow.com/questions/660613/how-do-you-hide-the-mouse-pointer-under-linux-x11
}

typedef struct DialogDataStruct
{
	int type, result;
} DialogData;
static gboolean display_dialog(gpointer user_data)
{
	DialogData *dialog_data=(DialogData*)user_data;
	GtkWidget *dialog;

	GtkButtonsType buttons=GTK_BUTTONS_NONE;
	switch(dialog_data->type)
	{
	case MBOX_OK:
		buttons=GTK_BUTTONS_OK;
		break;
	case MBOX_OKCANCEL:
		buttons=GTK_BUTTONS_OK_CANCEL;
		break;
	case MBOX_YESNOCANCEL:
		buttons=GTK_BUTTONS_YES_NO;
		break;
	}
	dialog=gtk_message_dialog_new(0, GTK_DIALOG_MODAL, GTK_MESSAGE_INFO, buttons, "%s", g_buf);
	
	// Set title, etc.

	dialog_data->result=gtk_dialog_run((GtkDialog*)dialog);
	gtk_widget_destroy(dialog);

	gtk_main_quit();
	return 0;
}
int		messagebox(MessageBoxType type, const char *title, const char *format, ...)
{
	va_list args;
	va_start(args, format);
	int len=vsnprintf(g_buf, g_buf_size, format, args);
	va_end(args);

	DialogData dialog_data={type, 0};
	g_idle_add(display_dialog, &dialog_data);
	gtk_main();
	return dialog_data.result;
}

//https://stackoverflow.com/questions/6145910/cross-platform-native-open-save-file-dialogs
//https://github.com/AndrewBelt/osdialog
typedef enum OSDialogActionEnum
{
	OSDIALOG_OPENFILE,
	OSDIALOG_OPENFOLDER,
	OSDIALOG_SAVEFILE,
} OSDialogAction;
//typedef struct osdialog_filtersStruct
//{
//	const char *name, **patterns;
//	int npatterns;
//} osdialog_filters;
static ArrayHandle osdialog_file(OSDialogAction action, Filter *filters, int nfilters, int select_multiple)
{
	GtkFileChooserAction gtkAction;
	const char* title, *acceptText;
	switch(action)
	{
	case OSDIALOG_OPENFILE:
		title="Open File";
		acceptText="Open";
		gtkAction=GTK_FILE_CHOOSER_ACTION_OPEN;
		break;
	case OSDIALOG_OPENFOLDER:
		title="Open Folder";
		acceptText="Open Folder";
		gtkAction=GTK_FILE_CHOOSER_ACTION_SELECT_FOLDER;
		break;
	case OSDIALOG_SAVEFILE:
		title="Save File";
		acceptText="Save";
		gtkAction=GTK_FILE_CHOOSER_ACTION_SAVE;
		break;
	default:
		return 0;
	}
	if(!gtk_init_check(NULL, NULL))
		return 0;

	GtkWidget* dialog=gtk_file_chooser_dialog_new(title, NULL, gtkAction,
		"_Cancel", GTK_RESPONSE_CANCEL,
		acceptText, GTK_RESPONSE_ACCEPT, NULL);

	for(int k=0;k<nfilters;++k)
	{
		GtkFileFilter *fileFilter=gtk_file_filter_new();
		gtk_file_filter_set_name(fileFilter, filters[k].comment);
		gtk_file_filter_add_pattern(fileFilter, filters[k].ext);
		gtk_file_chooser_add_filter(GTK_FILE_CHOOSER(dialog), fileFilter);
	}
	//for(int k=0;k<nfilters;++k)
	//{
	//	auto filter=filters+k;
	//	auto fileFilter=gtk_file_filter_new();
	//	gtk_file_filter_set_name(fileFilter, filter->name);
	//	for(int k2=0;k2<filter->npatterns;++k2)
	//		gtk_file_filter_add_pattern(fileFilter, filter->patterns[k2]);
	//	gtk_file_chooser_add_filter(GTK_FILE_CHOOSER(dialog), fileFilter);
	//}

	if(action==OSDIALOG_OPENFILE||action==OSDIALOG_OPENFOLDER)
		gtk_file_chooser_set_select_multiple(GTK_FILE_CHOOSER(dialog), select_multiple);

	if(action==OSDIALOG_SAVEFILE)
		gtk_file_chooser_set_do_overwrite_confirmation(GTK_FILE_CHOOSER(dialog), TRUE);

	//if(default_path)
	//	gtk_file_chooser_set_current_folder(GTK_FILE_CHOOSER(dialog), default_path);
	//
	//if(action==OSDIALOG_SAVEFILE&&filename)
	//	gtk_file_chooser_set_current_name(GTK_FILE_CHOOSER(dialog), filename);

	int ret=gtk_dialog_run(GTK_DIALOG(dialog))==GTK_RESPONSE_ACCEPT;
	ArrayHandle result=0;
	if(ret)
	{
		GSList *list=gtk_file_chooser_get_filenames(GTK_FILE_CHOOSER(dialog)), *it=list;
		ARRAY_ALLOC(char*, result, 0, 0);
		while(it)
		{
			char *filename=(char*)it->data;
			size_t len=strlen(filename);
			char *fn2=(char*)malloc(len+1);
			memcpy(fn2, filename, len+1);
			ARRAY_APPEND(result, &fn2, 1, 1, 0);
			g_free(filename);
			it=g_slist_next(it);
		}
		g_slist_free(list);
	}
	//char *chosen_filename=0;
	//if(ret)
	//	chosen_filename=gtk_file_chooser_get_filename(GTK_FILE_CHOOSER(dialog));//returns one at random
	gtk_widget_destroy(dialog);

	while(gtk_events_pending())
		gtk_main_iteration();
	return result;
}
ArrayHandle	dialog_open_folder(int multiple)
{
	return osdialog_file(OSDIALOG_OPENFOLDER, 0, 0, multiple);
}
ArrayHandle	dialog_open_file(Filter *filters, int nfilters, int multiple)
{
	return osdialog_file(OSDIALOG_OPENFILE, filters, nfilters, multiple);
}
const char*	dialog_save_file(Filter *filters, int nfilters)
{
	ArrayHandle arr=osdialog_file(OSDIALOG_SAVEFILE, filters, nfilters, 0);
	char *filename=0;
	if(arr)
	{
		filename=*(char**)array_at(&arr, 0);
		for(int k=1, size=array_size(&arr);k<size;++k)
			free(*(void**)array_at(&arr, k));
		array_free(&arr);
	}
	return filename;
}

typedef struct ClipboardDataStruct
{
	union
	{
		const char *send;
		char *receive;
	};
	int len;
} ClipboardData;
void		clipboard_callback(GtkClipboard *clipboard, const gchar *text, gpointer param)
{
	//https://stackoverflow.com/questions/52204996/how-do-i-use-clipboard-in-gtk
	ClipboardData *data=(ClipboardData*)param;
	if(data->send)//send to clipboard
		gtk_clipboard_set_text(clipboard, data->send, data->len);
	else//receive from clipboard
	{
		data->len=strlen(text);
		data->receive=(char*)malloc(data->len+1);
		memcpy(data->receive, text, data->len+1);
	}
	gtk_main_quit();
}
void		copy_to_clipboard_c(const char *str, int len)
{
	//https://stackoverflow.com/questions/52204996/how-do-i-use-clipboard-in-gtk
	ClipboardData data={{str}, len};
	GtkClipboard *clipboard=gtk_clipboard_get(GDK_SELECTION_CLIPBOARD);
	gtk_clipboard_request_text(clipboard, clipboard_callback, &data);
	gtk_main();
}
char*		paste_from_clipboard(int loud, int *ret_len)//don't forget to free memory
{
	//https://stackoverflow.com/questions/52204996/how-do-i-use-clipboard-in-gtk
	ClipboardData data={0};
	GtkClipboard *clipboard=gtk_clipboard_get(GDK_SELECTION_CLIPBOARD);
	gtk_clipboard_request_text(clipboard, clipboard_callback, &data);
	gtk_main();
	if(ret_len)
		*ret_len=data.len;
	return data.receive;
}

typedef GLXContext (*glXCreateContextAttribsARBProc)(Display*, GLXFBConfig, GLXContext, Bool, const int*);
static int isExtensionSupported(const char *extList, const char *extension)
{
	return strstr(extList, extension)!=0;
}
void				timer_start()
{
	timer=1;
}
void				timer_stop()
{
	timer=0;
}
typedef enum EventResultEnum
{
	RESULT_NONE,
	RESULT_REDRAW,
	RESULT_QUIT,
} EventResult;
const char translate_ASCII[128]=
{
//	 0    1    2    3    4    5    6    7    8    9    A    B    C    D    E    F
	' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ',//0
	' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ',//1

//	' '   !     "    #    $    %    &     '    (    )    *    +    ,    -    .    /
	' ', '1', '\'', '3', '4', '5', '7', '\'', '9', '0', '8', '=', ',', '-', '.', '/',//2

//	 0    1    2    3    4    5    6    7    8    9    :    ;    <    =    >    ?
	'0', '1', '2', '3', '4', '5', '6', '7', '8', '9', ';', ';', ',', '=', '.', '/',//3

//	 @    A    B    C    D    E    F    G    H    I    J    K    L    M    N    O
	'2', 'A', 'B', 'C', 'D', 'E', 'F', 'G', 'H', 'I', 'J', 'K', 'L', 'M', 'N', 'O',//4

//	 P    Q    R    S    T    U    V    W    X    Y    Z    [     \    ]    ^    _
	'P', 'Q', 'R', 'S', 'T', 'U', 'V', 'W', 'X', 'Y', 'Z', '[', '\\', ']', '6', '-',//5

//	 `    a    b    c    d    e    f    g    h    i    j    k    l    m    n    o
	'`', 'A', 'B', 'C', 'D', 'E', 'F', 'G', 'H', 'I', 'J', 'K', 'L', 'M', 'N', 'O',//6

//	 p    q    r    s    t    u    v    w    x    y    z    {     |    }    ~   del
	'P', 'Q', 'R', 'S', 'T', 'U', 'V', 'W', 'X', 'Y', 'Z', '[', '\\', ']', '`', '\0',//7
};
IOKey	translate_key(KeySym key, char *mask, char *c)
{
	*mask=1;
	if(!(key>>7))
	{
		*c=key&0x7F;
		return (IOKey)translate_ASCII[*c];
	}
	*c=0;
	switch(key)
	{
	case XK_BackSpace:		return KEY_BKSP;
	case XK_Tab:			return KEY_TAB;

	case XK_KP_Enter:		*mask=2;
	case XK_Return:			*c='\n'; return KEY_ENTER;

	case XK_Pause:			return KEY_PAUSE;
	case XK_Scroll_Lock:	return KEY_SCROLLLOCK;
	case XK_Sys_Req:		return KEY_PRINTSCR;
	case XK_Escape:			return KEY_ESC;
	case XK_Home:			return KEY_HOME;
	case XK_Left:			return KEY_LEFT;
	case XK_Up:				return KEY_UP;
	case XK_Right:			return KEY_RIGHT;
	case XK_Down:			return KEY_DOWN;
	case XK_Page_Up:		return KEY_PGUP;
	case XK_Page_Down:		return KEY_PGDN;
	case XK_End:			return KEY_END;
	case XK_Insert:			return KEY_INSERT;
	case XK_Break:			return KEY_BREAK;
	case XK_Num_Lock:		return KEY_NUMLOCK;

	case XK_KP_0:			*c='0';
	case XK_KP_Insert:		return KEY_NP_0;

	case XK_KP_1:			*c='1';
	case XK_KP_End:			return KEY_NP_1;

	case XK_KP_2:			*c='2';
	case XK_KP_Down:		return KEY_NP_2;

	case XK_KP_3:			*c='3';
	case XK_KP_Page_Down:	return KEY_NP_3;

	case XK_KP_4:			*c='4';
	case XK_KP_Left:		return KEY_NP_4;

	case XK_KP_5:			*c='5'; return KEY_NP_5;
	
	case XK_KP_6:			*c='6';
	case XK_KP_Right:		return KEY_NP_6;

	case XK_KP_7:			*c='7';
	case XK_KP_Home:		return KEY_NP_7;

	case XK_KP_8:			*c='8';
	case XK_KP_Up:			return KEY_NP_8;

	case XK_KP_9:			*c='9';
	case XK_KP_Page_Up:		return KEY_NP_9;
	
	case XK_KP_Delete:		*c=127; return KEY_NP_PERIOD;
	case XK_KP_Decimal:		*c='.'; return KEY_NP_PERIOD;
	case XK_KP_Multiply:	*c='*'; return KEY_NP_MUL;
	case XK_KP_Add:			*c='+'; return KEY_NP_PLUS;
	case XK_KP_Subtract:	*c='-'; return KEY_NP_MINUS;
	case XK_KP_Divide:		*c='/'; return KEY_NP_DIV;

	case XK_F1:				return KEY_F1;
	case XK_F2:				return KEY_F2;
	case XK_F3:				return KEY_F3;
	case XK_F4:				return KEY_F4;
	case XK_F5:				return KEY_F5;
	case XK_F6:				return KEY_F6;
	case XK_F7:				return KEY_F7;
	case XK_F8:				return KEY_F8;
	case XK_F9:				return KEY_F9;
	case XK_F10:			return KEY_F10;
	case XK_F11:			return KEY_F11;
	case XK_F12:			return KEY_F12;

	case XK_Shift_L:		*mask=2;
	case XK_Shift_R:		return KEY_SHIFT;

	case XK_Control_L:		*mask=2;
	case XK_Control_R:		return KEY_CTRL;

	case XK_Caps_Lock:		return KEY_CAPSLOCK;

	case XK_Alt_L:			*mask=2;
	case XK_Alt_R:			return KEY_ALT;

	case XK_Super_L:		*mask=2;
	case XK_Super_R:		return KEY_START;

	case XK_Delete:			return KEY_DEL;
	}
	return KEY_UNKNOWN;
}
int		process_event()
{
	int result=RESULT_NONE;
	XEvent event;
	KeySym key;
	XNextEvent(display, &event);
	switch(event.type)
	{
	case KeyPress:
		{
			XLookupString(&event.xkey, g_buf, g_buf_size, &key, 0);
			char mask=0, c=0;
			IOKey k=translate_key(key, &mask, &c);
			result=io_keydn(k, c);
			keyboard[k]|=mask;
			//printf("%016llX down\n", (long long)key);
			//printf("down:\t%s\n", g_buf);
			//printf("down:\t%s\t%ld\n", g_buf, strlen(g_buf));
		}
		break;
	case KeyRelease:
		{
			XLookupString(&event.xkey, g_buf, g_buf_size, &key, 0);
			char mask=0, c=0;
			IOKey k=translate_key(key, &mask, &c);
			result=io_keyup(k, c);
			keyboard[k]&=~mask;
			//printf("%016llX up\n", (long long)key);
			//printf("up:\t%s\n", g_buf);
		}
		break;
	case ButtonPress:
		if(event.xbutton.button==4||event.xbutton.button==5)
			result=io_mousewheel(event.xbutton.button==4);
		else
			result=io_keydn((IOKey)event.xbutton.button, 0);
		break;
	case ButtonRelease:
		if(event.xbutton.button!=4&&event.xbutton.button!=5)
			result=io_keyup((IOKey)event.xbutton.button, 0);
		break;
	case MotionNotify:
		mx=event.xmotion.x, my=event.xmotion.y;
		if(!mouse_bypass)
			result=io_mousemove();
		else
			mouse_bypass=0;
		break;
	//case EnterNotify:
	//	printf("Enter at (%d, %d)\n", event.xcrossing.x, event.xcrossing.y);
	//	break;
	//case LeaveNotify:
	//	printf("Leave at (%d, %d)\n", event.xcrossing.x, event.xcrossing.y);
	//	break;
	//case FocusIn:
	//	printf("FocusIn\n");
	//	break;
	//case FocusOut:
	//	printf("FocusOut\n");
	//	break;
	//case KeymapNotify:
	//	printf("KeymapNotify\n");
	//	break;
	case Expose:
		{
			XWindowAttributes attribs;
			XGetWindowAttributes(display, window, &attribs);
			w=attribs.width, h=attribs.height;
			set_region_immediate(0, w, 0, h);
			io_resize();
			result=RESULT_REDRAW;
		}
		break;
	//case GraphicsExpose:
	//	printf("GraphicsExpose\n");
	//	break;
	//case NoExpose:
	//	printf("NoExpose\n");
	//	break;
	//case VisibilityNotify:
	//	printf("VisibilityNotify\n");
	//	break;
	//case CreateNotify:
	//	printf("CreateNotify\n");
	//	break;
	case DestroyNotify:
		result=RESULT_QUIT;
	//case UnmapNotify:
	//	printf("UnmapNotify\n");
	//	break;
	//case MapNotify:
	//	printf("MapNotify\n");
	//	break;
	//case MapRequest:
	//	printf("MapRequest\n");
	//	break;
	//case ReparentNotify:
	//	printf("ReparentNotify\n");
	//	break;
	//case ConfigureNotify:
	//	printf("ConfigureNotify\n");
	//	break;
	//case ConfigureRequest:
	//	printf("ConfigureRequest\n");
	//	break;
	//case GravityNotify:
	//	printf("GravityNotify\n");
	//	break;
	//case ResizeRequest:
	//	printf("ResizeRequest\n");
	//	break;
	//case CirculateNotify:
	//	printf("CirculateNotify\n");
	//	break;
	//case CirculateRequest:
	//	printf("CirculateRequest\n");
	//	break;
	//case PropertyNotify:
	//	printf("PropertyNotify\n");
	//	break;
	//case SelectionClear:
	//	printf("SelectionClear\n");
	//	break;
	//case SelectionRequest:
	//	printf("SelectionRequest\n");
	//	break;
	//case SelectionNotify:
	//	printf("SelectionNotify\n");
	//	break;
	//case ColormapNotify:
	//	printf("ColormapNotify\n");
	//	break;
	case ClientMessage:
		if(event.xclient.data.l[0]==atomWmDeleteWindow)
		{
			if(io_quit_request())
				result=RESULT_QUIT;
		}
		break;
	//case MappingNotify:
	//	printf("MappingNotify\n");
	//	break;
	//case GenericEvent:
	//	printf("GenericEvent\n");
	//	break;
	//default:
	//	printf("unknown event %d\n", event.type);
	//	break;
	}
	return result;
}
int		main(int argc, char** argv)
{
	gtk_init(&argc, &argv);
	init_math_constants();

	int len=strlen(argv[0]);
	int k=len-1;
	for(;k>0&&argv[0][k-1]!='/'&&argv[0][k-1]!='\\';--k);
	exe_dir_len=k;
	exe_dir=(char*)malloc(k+1);
	memcpy(exe_dir, argv[0], k);
	exe_dir[k]='\0';

	//Open the display
	display=XOpenDisplay(0);
	if(!display)
	{
		printf("Could not open display\n");
		return 1;
	}
	screen=DefaultScreenOfDisplay(display);
	screenId=DefaultScreen(display);
	
	//Check GLX version
	GLint majorGLX, minorGLX = 0;
	glXQueryVersion(display, &majorGLX, &minorGLX);
	if(majorGLX <= 1 && minorGLX < 2)
	{
		printf("GLX 1.2 or greater is required.\n");
		XCloseDisplay(display);
		return 1;
	}

	GLint glxAttribs[]=
	{
		GLX_X_RENDERABLE    , True,
		GLX_DRAWABLE_TYPE   , GLX_WINDOW_BIT,
		GLX_RENDER_TYPE     , GLX_RGBA_BIT,
		GLX_X_VISUAL_TYPE   , GLX_TRUE_COLOR,
		GLX_RED_SIZE        , 8,
		GLX_GREEN_SIZE      , 8,
		GLX_BLUE_SIZE       , 8,
		GLX_ALPHA_SIZE      , 8,
		GLX_DEPTH_SIZE      , 24,
		GLX_STENCIL_SIZE    , 8,
		GLX_DOUBLEBUFFER    , True,
		None
	};
	
	int fbcount;
	GLXFBConfig *fbc = glXChooseFBConfig(display, screenId, glxAttribs, &fbcount);
	if(!fbc)
	{
		printf("Failed to retrieve framebuffer.\n");
		XCloseDisplay(display);
		return 1;
	}

	// Pick the FB config/visual with the most samples per pixel
	int best_fbc = -1, worst_fbc = -1, best_num_samp = -1, worst_num_samp = 999;
	for (int i = 0; i < fbcount; ++i)
	{
		XVisualInfo *vi = glXGetVisualFromFBConfig( display, fbc[i] );
		if ( vi != 0)
		{
			int samp_buf, samples;
			glXGetFBConfigAttrib( display, fbc[i], GLX_SAMPLE_BUFFERS, &samp_buf );
			glXGetFBConfigAttrib( display, fbc[i], GLX_SAMPLES       , &samples  );

			if ( best_fbc < 0 || (samp_buf && samples > best_num_samp) )
			{
				best_fbc = i;
				best_num_samp = samples;
			}
			if ( worst_fbc < 0 || !samp_buf || samples < worst_num_samp )
				worst_fbc = i;
			worst_num_samp = samples;
		}
		XFree( vi );
	}
	GLXFBConfig bestFbc = fbc[ best_fbc ];
	XFree( fbc ); // Make sure to free this!

	
	XVisualInfo* visual = glXGetVisualFromFBConfig( display, bestFbc );
	if (visual == 0)
	{
		printf("Could not create correct visual window.\n");
		XCloseDisplay(display);
		return 1;
	}
	
	if (screenId != visual->screen)
	{
		printf("screenId(%d) does not match visual->screen(%d).\n", screenId, visual->screen);
		XCloseDisplay(display);
		return 1;
	}

	//Open the window
	int dis_w=screen->width, dis_h=screen->height;
	XSetWindowAttributes windowAttribs;
	windowAttribs.border_pixel = BlackPixel(display, screenId);
	windowAttribs.background_pixel = WhitePixel(display, screenId);
	windowAttribs.override_redirect = True;
	windowAttribs.colormap = XCreateColormap(display, RootWindow(display, screenId), visual->visual, AllocNone);
	windowAttribs.event_mask=0//0x1FFFFFF
		|ExposureMask
		|KeyPressMask|KeyReleaseMask//|KeymapStateMask
		|ButtonPressMask|ButtonReleaseMask
		|PointerMotionMask|EnterWindowMask|LeaveWindowMask
		;
	window=XCreateWindow(display, RootWindow(display, screenId), 0, 0, w=WINDOW_WIDTH, h=WINDOW_HEIGHT, 0, visual->depth, InputOutput, visual->visual, CWBackPixel | CWColormap | CWBorderPixel | CWEventMask, &windowAttribs);

	//XStoreName(display, window, "IO Test");
	//XSetStandardProperties(display, window, "IO Test", "What is this", None, argv, argc, 0);

	// Redirect Close
	atomWmDeleteWindow=XInternAtom(display, "WM_DELETE_WINDOW", False);
	XSetWMProtocols(display, window, &atomWmDeleteWindow, 1);

	// Create GLX OpenGL context
	glXCreateContextAttribsARBProc glXCreateContextAttribsARB=0;
	glXCreateContextAttribsARB=(glXCreateContextAttribsARBProc)glXGetProcAddressARB( (const GLubyte *) "glXCreateContextAttribsARB" );
	
	int context_attribs[]=
	{
	//	GLX_CONTEXT_MAJOR_VERSION_ARB, 3,
	//	GLX_CONTEXT_MINOR_VERSION_ARB, 2,
	//	GLX_CONTEXT_FLAGS_ARB, GLX_CONTEXT_FORWARD_COMPATIBLE_BIT_ARB,
		None
	};

	const char *glxExts=glXQueryExtensionsString(display, screenId);
	if(!isExtensionSupported( glxExts, "GLX_ARB_create_context"))
	{
		printf("GLX_ARB_create_context not supported\n");
		context=glXCreateNewContext(display, bestFbc, GLX_RGBA_TYPE, 0, True);
	}
	else
		context=glXCreateContextAttribsARB(display, bestFbc, 0, 1, context_attribs);
	XSync(display, False);

	if(!glXIsDirect(display, context))//Verifying that context is a direct context
		printf("Indirect GLX rendering context obtained\n");
	else
		printf("Direct GLX rendering context obtained\n");
	glXMakeCurrent(display, window, context);

	GLversion=(const char*)glGetString(GL_VERSION);
	printf("GL Renderer: %s\n", glGetString(GL_RENDERER));
	printf("GL Version: %s\n", GLversion);
	printf("GLSL Version: %s\n", glGetString(GL_SHADING_LANGUAGE_VERSION));
	
	if(!init_gl()||!io_init(argc, argv))
	{
		glXDestroyContext(display, context);
		XFree(visual);
		XFreeColormap(display, windowAttribs.colormap);
		XDestroyWindow(display, window);
		XCloseDisplay(display);
		return 1;
	}

	XClearWindow(display, window);
	XMapRaised(display, window);//Show the window
	get_mouse(&mx, &my);

	int signal;
	for(;;)//message loop
	{
		signal=0;
		if(timer)
		{
			while(XPending(display))
				signal|=process_event();
		}
		else
		{
			do
				signal|=process_event();
			while(XPending(display));
		}

		if(signal>>1)
			break;
		if(timer)
			io_timer();
		if(timer||signal&1)
		{
			io_render();
			prof_print();
			glXSwapBuffers(display, window);
		}

		//usleep(500000);
		
		//gettimeofday(&time, NULL);
		//nextGameTick += (1000 / FPS);
		//sleepTime = nextGameTick - ((time.tv_sec * 1000) + (time.tv_usec / 1000));
		//usleep((unsigned int)(sleepTime / 1000));
	}

	printf("Quitting...\n");
	io_cleanup();

	// Cleanup GLX
	glXDestroyContext(display, context);

	// Cleanup X11
	XFree(visual);
	XFreeColormap(display, windowAttribs.colormap);
	XDestroyWindow(display, window);
	XCloseDisplay(display);
	return 0;
}
#else
#error "Unknown OS"
#endif

//C array
#if 1
static void		array_realloc(ArrayHandle *arr, size_t count, size_t pad)//CANNOT be nullptr, array must be initialized with array_alloc()
{
	ArrayHandle p2;
	size_t size, newcap;

	ASSERT_P(*arr);
	size=(count+pad)*arr[0]->esize, newcap=arr[0]->esize;
	for(;newcap<size;newcap<<=1);
	if(newcap>arr[0]->cap)
	{
		p2=(ArrayHandle)realloc(*arr, sizeof(ArrayHeader)+newcap);
		ASSERT_P(p2);
		*arr=p2;
		if(arr[0]->cap<newcap)
			memset(arr[0]->data+arr[0]->cap, 0, newcap-arr[0]->cap);
		arr[0]->cap=newcap;
	}
	arr[0]->count=count;
}

//Array API
ArrayHandle		array_construct(const void *src, size_t esize, size_t count, size_t rep, size_t pad, DebugInfo debug_info)
{
	ArrayHandle arr;
	size_t srcsize, dstsize, cap;
	
	srcsize=count*esize;
	dstsize=rep*srcsize;
	for(cap=esize+pad*esize;cap<dstsize;cap<<=1);
	arr=(ArrayHandle)malloc(sizeof(ArrayHeader)+cap);
	ASSERT_P(arr);
	arr->count=count;
	arr->esize=esize;
	arr->cap=cap;
	arr->debug_info=debug_info;
	if(src)
	{
		ASSERT_P(src);
		memfill(arr->data, src, dstsize, srcsize);
	}
	if(cap-dstsize>0)
		memset(arr->data+dstsize, 0, cap-dstsize);
	return arr;
}
ArrayHandle		array_copy(ArrayHandle *arr, DebugInfo debug_info)
{
	ArrayHandle a2;
	size_t bytesize;

	if(!*arr)
		return 0;
	bytesize=sizeof(ArrayHeader)+arr[0]->cap;
	a2=(ArrayHandle)malloc(bytesize);
	ASSERT_P(a2);
	memcpy(a2, *arr, bytesize);
	a2->debug_info=debug_info;
	return a2;
}
void			array_free(ArrayHandle *arr)//can be nullptr
{
	free(*arr);
	*arr=0;
}
void			array_clear(ArrayHandle *arr)//can be nullptr
{
	if(*arr)
		arr[0]->count=0;
}
void*			array_insert(ArrayHandle *arr, size_t idx, void *data, size_t count, size_t rep, size_t pad)
{
	size_t start, srcsize, dstsize, movesize;
	
	ASSERT_P(*arr);
	start=idx*arr[0]->esize;
	srcsize=count*arr[0]->esize;
	dstsize=rep*srcsize;
	movesize=arr[0]->count*arr[0]->esize-start;
	array_realloc(arr, arr[0]->count+rep*count, pad);
	memmove(arr[0]->data+start+dstsize, arr[0]->data+start, movesize);
	if(data)
		memfill(arr[0]->data+start, data, dstsize, srcsize);
	else
		memset(arr[0]->data+start, 0, dstsize);
	return arr[0]->data+start;
}
void			array_fit(ArrayHandle *arr, size_t pad)//can be nullptr
{
	ArrayHandle p2;
	if(!*arr)
		return;
	arr[0]->cap=(arr[0]->count+pad)*arr[0]->esize;
	p2=(ArrayHandle)realloc(*arr, sizeof(ArrayHeader)+arr[0]->cap);
	ASSERT_P(p2);
	*arr=p2;
}
size_t			array_size(ArrayHandle const *arr)//can be nullptr
{
	if(!arr[0])
		return 0;
	return arr[0]->count;
}
void*			array_at(ArrayHandle *arr, size_t idx)
{
	if(!arr[0])
		return 0;
	if(idx>=arr[0]->count)
		return 0;
	return arr[0]->data+idx*arr[0]->esize;
}
const void*		array_at_const(ArrayHandle const *arr, int idx)
{
	if(!arr[0])
		return 0;
	return arr[0]->data+idx*arr[0]->esize;
}
void*			array_back(ArrayHandle *arr)
{
	if(!*arr||!arr[0]->count)
		return 0;
	return arr[0]->data+(arr[0]->count-1)*arr[0]->esize;
}
const void*		array_back_const(ArrayHandle const *arr)
{
	if(!*arr||!arr[0]->count)
		return 0;
	return arr[0]->data+(arr[0]->count-1)*arr[0]->esize;
}
#endif

//profiler
#if 1
char				prof_on=0;
double				elapsed_ms()//since last call
{
	static double t1=0;
	double t2=time_ms(), diff=t2-t1;
	t1=t2;
	return diff;
}
double				elapsed_cycles()//since last call
{
	static long long t1=0;
	long long t2=__rdtsc();
	double diff=(double)(t2-t1);
	t1=t2;
	return diff;
}
#ifdef PROFILER_CYCLES
#define		ELAPSED_FN	elapsed_cycles
#else
#define		ELAPSED_FN	elapsed_ms
#endif
typedef struct		ProfInfoStruct
{
	const char *label;
	double elapsed;
} ProfInfo;
ArrayHandle			prof=0;
int					prof_size=0, prof_cap=0;
int					prof_array_start_idx=0;
static void			prof_insert(ProfInfo *info)
{
	if(!prof)
		ARRAY_ALLOC(ProfInfo, prof, 0, 1);
	ARRAY_APPEND(prof, info, 1, 1, 0);
}
void				prof_start(){double elapsed=ELAPSED_FN();}
void				prof_add(const char *label)
{
	if(prof_on)
	{
		ProfInfo info={label, ELAPSED_FN()};
		prof_insert(&info);
	}
}
void				prof_sum(const char *label, int count)//add the sum of last 'count' steps
{
	if(prof_on)
	{
		ProfInfo info={label, 0};
		for(int k=array_size(&prof)-1, k2=0;k>=0&&k2<count;--k, ++k2)
			info.elapsed+=((ProfInfo*)array_at(&prof, k))->elapsed;
		prof_insert(&info);
	}
}
void				prof_loop_start(const char **labels, int n)//describe the loop body parts in 'labels'
{
	if(prof_on)
	{
		prof_array_start_idx=array_size(&prof);
		for(int k=0;k<n;++k)
		{
			ProfInfo info={labels[k], 0};
			prof_insert(&info);
		}
	}
}
void				prof_add_loop(int idx)//call on each part of loop body
{
	if(prof_on)
	{
		double elapsed=ELAPSED_FN();
		((ProfInfo*)array_at(&prof, prof_array_start_idx+idx))->elapsed+=elapsed;
	}
}
void				prof_print()
{
	if(prof_on)
	{
		float xpos=(float)(w-400), xpos2=(float)(w-200);
		for(int k=0, kEnd=array_size(&prof);k<kEnd;++k)
		{
			ProfInfo *p=(ProfInfo*)array_at(&prof, k);
			float ypos=k*tdy;
			GUIPrint(xpos, xpos, ypos, 1, "%s", p->label);
			GUIPrint(xpos, xpos2, ypos, 1, "%lf", p->elapsed);
		}
		static double t1=0;
		if(!t1)
			t1=time_ms();
		double t2=time_ms();
		GUIPrint(xpos, xpos, (array_size(&prof)+1)*tdy, 1, "fps=%lf, T=%lfms", 1000/(t2-t1), t2-t1);
		t1=t2;

		array_clear(&prof);
	}
}
#endif

//OpenGL
const char*			glerr2str(int error)
{
#define 			EC(x)	case x:a=(const char*)#x;break
	const char *a=0;
	switch(error)
	{
	case 0:a="SUCCESS";break;
	EC(GL_INVALID_ENUM);
	EC(GL_INVALID_VALUE);
	EC(GL_INVALID_OPERATION);
	case 0x0503:a="GL_STACK_OVERFLOW";break;
	case 0x0504:a="GL_STACK_UNDERFLOW";break;
	EC(GL_OUT_OF_MEMORY);
	case 0x0506:a="GL_INVALID_FRAMEBUFFER_OPERATION";break;
	case 0x0507:a="GL_CONTEXT_LOST";break;
	case 0x8031:a="GL_TABLE_TOO_LARGE";break;
	default:a="???";break;
	}
	return a;
#undef				EC
}
void 				gl_check(const char *file, int line)
{
	int err=glGetError();
	if(err)
		log_error(file, line, "GL %d: %s", err, glerr2str(err));
}
void				gl_error(const char *file, int line)
{
	int err=glGetError();
	log_error(file, line, "GL %d: %s", err, glerr2str(err));
}

//shader API
unsigned			CompileShader(const char *src, unsigned type, const char *programname)
{
	unsigned shaderID=glCreateShader(type);
	glShaderSource(shaderID, 1, &src, 0);
	glCompileShader(shaderID);
	int success=0;
	glGetShaderiv(shaderID, GL_COMPILE_STATUS, &success);
	if(!success)
	{
		int infoLogLength;
		glGetShaderiv(shaderID, GL_INFO_LOG_LENGTH, &infoLogLength);
		char *errorMessage=(char*)malloc(infoLogLength+1);
		glGetShaderInfoLog(shaderID, infoLogLength, 0, errorMessage);
		copy_to_clipboard_c(errorMessage, infoLogLength);
		if(programname)
			LOG_ERROR("%s shader compilation failed. Output copied to cipboard.", programname);
		else
			GL_ERROR();
		free(errorMessage);
		return 0;
	}
	return shaderID;
}
unsigned			make_gl_program_impl(const char *vertSrc, const char *fragSrc, const char *programname)
{
	unsigned
		vertShaderID=CompileShader(vertSrc, GL_VERTEX_SHADER, programname),
		fragShaderID=CompileShader(fragSrc, GL_FRAGMENT_SHADER, programname);

	if(!vertShaderID||!fragShaderID)
	{
		GL_ERROR();
		return 0;
	}
	unsigned ProgramID=glCreateProgram();
	glAttachShader(ProgramID, vertShaderID);
	glAttachShader(ProgramID, fragShaderID);
	glLinkProgram(ProgramID);

	int success=0;
	glGetProgramiv(ProgramID, GL_LINK_STATUS, &success);
	if(!success)
	{
		int infoLogLength;
		glGetProgramiv(ProgramID, GL_INFO_LOG_LENGTH, &infoLogLength);
		char *errorMessage=(char*)malloc(infoLogLength+1);
		glGetProgramInfoLog(ProgramID, infoLogLength, 0, errorMessage);
		copy_to_clipboard_c(errorMessage, infoLogLength);
		if(programname)
			LOG_ERROR("%s shader link failed. Output copied to cipboard.", programname);
		else
			GL_ERROR();
		free(errorMessage);
		return 0;
	}
	glDetachShader(ProgramID, vertShaderID);
	glDetachShader(ProgramID, fragShaderID);
	glDeleteShader(vertShaderID);
	glDeleteShader(fragShaderID);

	GL_CHECK();
	return ProgramID;
}
int					make_gl_program(ShaderProgram *p)
{
	p->program=make_gl_program_impl(p->vsrc, p->fsrc, p->name);
	if(!p->program)
		return 0;
	for(int ka=0;ka<p->n_attr;++ka)
	{
		ShaderVar *attr=p->attributes+ka;
		if((*attr->pvar=glGetAttribLocation(p->program, attr->name))==-1)
		{
			LOG_ERROR("%s: attribute %s == -1", p->name, attr->name);
			return 0;
		}
	}
	for(int ku=0;ku<p->n_unif;++ku)
	{
		ShaderVar *unif=p->uniforms+ku;
		if((*unif->pvar=glGetUniformLocation(p->program, unif->name))==-1)
		{
			LOG_ERROR("%s: uniform %s == -1", p->name, unif->name);
			return 0;
		}
	}
	return 1;
}
unsigned			current_program=0;
void				setGLProgram(unsigned program)
{
	if(current_program!=program)
	{
		glUseProgram(current_program=program);
		GL_CHECK();
	}
}
void				send_color(unsigned location, int color)
{
	unsigned char *p=(unsigned char*)&color;
	__m128 m_255=_mm_set1_ps(inv255);

	__m128 c=_mm_castsi128_ps(_mm_set_epi32(p[3], p[2], p[1], p[0]));
	c=_mm_cvtepi32_ps(_mm_castps_si128(c));
	c=_mm_mul_ps(c, m_255);
	ALIGN(16) float comp[4];
	_mm_store_ps(comp, c);
	glUniform4fv(location, 1, comp);

	//glUniform4f(location, p[0]*inv255, p[1]*inv255, p[2]*inv255, p[3]*inv255);
	
	GL_CHECK();
}
void				send_color_rgb(unsigned location, int color)
{
	unsigned char *p=(unsigned char*)&color;
	__m128 m_255=_mm_set1_ps(inv255);

	__m128 c=_mm_castsi128_ps(_mm_set_epi32(p[3], p[2], p[1], p[0]));
	c=_mm_cvtepi32_ps(_mm_castps_si128(c));
	c=_mm_mul_ps(c, m_255);
	ALIGN(16) float comp[4];
	_mm_store_ps(comp, c);
	glUniform3fv(location, 1, comp);
	//glUniform3f(location, p[0]*inv255, p[1]*inv255, p[2]*inv255);

	GL_CHECK();
}
void				send_texture_pot(unsigned gl_texture, int *rgba, int txw, int txh)
{
	glBindTexture(GL_TEXTURE_2D, gl_texture);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, txw, txh, 0, GL_RGBA,  GL_UNSIGNED_BYTE, rgba);
}
void				send_texture_pot_linear(unsigned gl_texture, unsigned char *bmp, int txw, int txh)
{
	glBindTexture(GL_TEXTURE_2D, gl_texture);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
	glTexImage2D(GL_TEXTURE_2D, 0, GL_RED, txw, txh, 0, GL_RED, GL_UNSIGNED_BYTE, bmp);
}
void				select_texture(unsigned tx_id, int u_location)
{
	glActiveTexture(GL_TEXTURE0);			GL_CHECK();
	glBindTexture(GL_TEXTURE_2D, tx_id);	GL_CHECK();//select texture
	glUniform1i(u_location, 0);				GL_CHECK();
}
void				set_region_immediate(int x1, int x2, int y1, int y2)
{
	rx0=x1, ry0=y1, rdx=x2-x1, rdy=y2-y1;
	glViewport(rx0, h-y2, rdx, rdy);

	SN_x1=2.f/rdx, SN_x0=(float)(-2*rx0-(rdx-1))/rdx;//frac bias
	SN_y1=-2.f/rdy, SN_y0=(float)(rdy-1+2*ry0)/rdy;

	//SN_x1=2.f/rdx, SN_x0=-rx0*SN_x1-(float)(rdx-1)/rdx;//frac bias
	//SN_y1=-2.f/rdy, SN_y0=(float)(rdy-1)/rdy-ry0*SN_y1;

	//SN_x1=2.f/(rdx-(rdx!=0)), SN_x0=-rx0*SN_x1-1;//X vernier lines
	//SN_y1=-2.f/(rdy-(rdy!=0)), SN_y0=1-ry0*SN_y1;

	//SN_x1=2.f/rdx, SN_x0=-rx0*SN_x1-1;//X old equation
	//SN_y1=-2.f/rdy, SN_y0=1-ry0*SN_y1;


	NS_x1=rdx/2.f, NS_x0=rx0+NS_x1;
	NS_y1=-rdy/2.f, NS_y0=ry0-NS_x1;
}
int					init_gl()
{
	glGenBuffers(1, &vertex_buffer);
	glEnable(GL_BLEND);									GL_CHECK();//vast majority of applications need alpha blend
	glBlendEquation(GL_FUNC_ADD);						GL_CHECK();
	glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);	GL_CHECK();
	//glDisable(GL_BLEND);	GL_CHECK();

	//glDisable(GL_LINE_SMOOTH);	GL_CHECK();//does nothing

#ifndef NO_3D
	glEnable(GL_DEPTH_TEST);
#endif

#define	SHADER(NAME)	if(!make_gl_program(&shader_##NAME))return 0;
	SHADER_LIST
#undef	SHADER
	
	//load fonts
#if 1
	int iw=0, ih=0, bytespp=0;
	sprintf_s(g_buf, g_buf_size, "%sfont.PNG", exe_dir);
	int *rgb=(int*)stbi_load(g_buf, &iw, &ih, &bytespp, 4);
	if(!rgb)
	{
		LOG_ERROR("Font texture not found.\nPlace a \'font.PNG\' file with the program.\n");
		exit(1);
	}
	tdx=(float)(rgb[0]&0xFF), tdy=(float)(rgb[1]&0xFF);
	if(!tdx||!tdy)
	{
		LOG_ERROR("Invalid font texture character dimensions: dx=%d, dy=%d", tdx, tdy);
		exit(1);
	}
	for(int k=0, size=iw*ih;k<size;++k)
		if(rgb[k]&0x00FFFFFF)
			rgb[k]=0xFFFFFFFF;
	for(int c=32;c<127;++c)
	{
		QuadCoords *rect=font_coords+c-32;
		int px=(iw>>3)*(c&7), py=(ih>>4)*(c>>3);
		rect->x1=(float)px/iw, rect->x2=(float)(px+tdx)/iw;
		rect->y1=(float)py/ih, rect->y2=(float)(py+tdy)/ih;
	}
	glGenTextures(1, &font_txid);
	send_texture_pot(font_txid, rgb, iw, ih);
	stbi_image_free(rgb);
	
	sprintf_s(g_buf, g_buf_size, "%sfont_sdf.PNG", exe_dir);
	unsigned char *bmp=(unsigned char*)stbi_load(g_buf, &iw, &ih, &bytespp, 1);
	if(bmp)
	{
		sdf_available=1;
		SDFTextureHeader header;
		memcpy(&header, bmp, sizeof(header));
		sdf_dx=header.csize_x;
		sdf_dy=header.csize_y;

		sdf_slope=(float)header.slope;
		for(int c=32;c<127;++c)
		{
			QuadCoords *rect=sdf_glyph_coords+c-32;
			int px=header.grid_start_x+header.cell_size_x*(c&7),
				py=header.grid_start_y+header.cell_size_y*((c>>3)-4);
			rect->x1=(float)px/iw;
			rect->x2=(float)(px+sdf_dx)/iw;
			rect->y1=(float)py/ih;
			rect->y2=(float)(py+sdf_dy)/ih;
		}
		sdf_txh=sdf_dy;
		sdf_dx*=16.f/sdf_dy;
		sdf_dy=16;

		glGenTextures(1, &sdf_atlas_txid);
		send_texture_pot_linear(sdf_atlas_txid, bmp, iw, ih);
		stbi_image_free(bmp);
		sdf_active=1;
		set_text_colors(colors_text);
		sdf_active=0;
	}
	set_text_colors(colors_text);
	toggle_sdftext();
	prof_add("Load font");
#endif
	return 1;
}

//immediate drawing functions
float				g_fbuf[16]={0};
unsigned			vertex_buffer=0;
void				draw_line(float x1, float y1, float x2, float y2, int color)
{
	g_fbuf[0]=screen2NDC_x(x1), g_fbuf[1]=screen2NDC_y(y1);
	g_fbuf[2]=screen2NDC_x(x2), g_fbuf[3]=screen2NDC_y(y2);
	setGLProgram(shader_2D.program);		GL_CHECK();
	send_color(u_2D_color, color);			GL_CHECK();

	glEnableVertexAttribArray(a_2D_coords);	GL_CHECK();

	glBindBuffer(GL_ARRAY_BUFFER, vertex_buffer);							GL_CHECK();
	glBufferData(GL_ARRAY_BUFFER, 4*sizeof(float), g_fbuf, GL_STATIC_DRAW);	GL_CHECK();
	glVertexAttribPointer(a_2D_coords, 2, GL_FLOAT, GL_FALSE, 0, 0);		GL_CHECK();
	
//	glBindBuffer(GL_ARRAY_BUFFER, 0);										GL_CHECK();
//	glVertexAttribPointer(a_2D_coords, 2, GL_FLOAT, GL_FALSE, 0, g_fbuf);	GL_CHECK();

#ifndef NO_3D
	glDisable(GL_DEPTH_TEST);
#endif
	glDrawArrays(GL_LINES, 0, 2);			GL_CHECK();
	glDisableVertexAttribArray(a_2D_coords);GL_CHECK();
#ifndef NO_3D
	glEnable(GL_DEPTH_TEST);
#endif
}
void				draw_line_i(int x1, int y1, int x2, int y2, int color){draw_line((float)x1, (float)y1, (float)x2, (float)y2, color);}
void				draw_rectangle(float x1, float x2, float y1, float y2, int color)
{
	float
		X1=screen2NDC_x_bias(x1), Y1=screen2NDC_y_bias(y1),
		X2=screen2NDC_x_bias(x2), Y2=screen2NDC_y_bias(y2);
	g_fbuf[0]=X1, g_fbuf[1]=Y1;
	g_fbuf[2]=X2, g_fbuf[3]=Y1;
	g_fbuf[4]=X2, g_fbuf[5]=Y2;
	g_fbuf[6]=X1, g_fbuf[7]=Y2;
	g_fbuf[8]=X1, g_fbuf[9]=Y1;
	setGLProgram(shader_2D.program);		GL_CHECK();
	send_color(u_2D_color, color);			GL_CHECK();

	glEnableVertexAttribArray(a_2D_coords);	GL_CHECK();

	glBindBuffer(GL_ARRAY_BUFFER, vertex_buffer);							GL_CHECK();
	glBufferData(GL_ARRAY_BUFFER, 10*sizeof(float), g_fbuf, GL_STATIC_DRAW);GL_CHECK();
	glVertexAttribPointer(a_2D_coords, 2, GL_FLOAT, GL_FALSE, 0, 0);		GL_CHECK();
	
//	glBindBuffer(GL_ARRAY_BUFFER, 0);										GL_CHECK();
//	glVertexAttribPointer(a_2D_coords, 2, GL_FLOAT, GL_FALSE, 0, g_fbuf);	GL_CHECK();
	
#ifndef NO_3D
	glDisable(GL_DEPTH_TEST);
#endif
	glDrawArrays(GL_TRIANGLE_FAN, 0, 5);	GL_CHECK();
	glDisableVertexAttribArray(a_2D_coords);GL_CHECK();
#ifndef NO_3D
	glEnable(GL_DEPTH_TEST);
#endif
}
void				draw_rectangle_i(int x1, int x2, int y1, int y2, int color){draw_rectangle((float)x1, (float)x2, (float)y1, (float)y2, color);}
void				draw_rectangle_hollow(float x1, float x2, float y1, float y2, int color)
{
	float
		X1=screen2NDC_x(x1), Y1=screen2NDC_y(y1),
		X2=screen2NDC_x(x2), Y2=screen2NDC_y(y2);
	g_fbuf[0]=X1, g_fbuf[1]=Y1;
	g_fbuf[2]=X1, g_fbuf[3]=Y2;
	g_fbuf[4]=X2, g_fbuf[5]=Y2;
	g_fbuf[6]=X2, g_fbuf[7]=Y1;
	setGLProgram(shader_2D.program);		GL_CHECK();
	send_color(u_2D_color, color);			GL_CHECK();

	glEnableVertexAttribArray(a_2D_coords);	GL_CHECK();

	glBindBuffer(GL_ARRAY_BUFFER, vertex_buffer);							GL_CHECK();
	glBufferData(GL_ARRAY_BUFFER, 8*sizeof(float), g_fbuf, GL_STATIC_DRAW);	GL_CHECK();
	glVertexAttribPointer(a_2D_coords, 2, GL_FLOAT, GL_FALSE, 0, 0);		GL_CHECK();
	
//	glBindBuffer(GL_ARRAY_BUFFER, 0);										GL_CHECK();
//	glVertexAttribPointer(a_2D_coords, 2, GL_FLOAT, GL_FALSE, 0, g_fbuf);	GL_CHECK();
	
#ifndef NO_3D
	glDisable(GL_DEPTH_TEST);
#endif
	glDrawArrays(GL_LINE_LOOP, 0, 4);		GL_CHECK();
	glDisableVertexAttribArray(a_2D_coords);GL_CHECK();
#ifndef NO_3D
	glEnable(GL_DEPTH_TEST);
#endif
}

float				*vrtx=0;
int					vrtx_bcap=0;
int					vrtx_resize(int vcount, int floatspervertex)
{
	int bytesize=vcount*floatspervertex*sizeof(float), bcap=vrtx_bcap?vrtx_bcap:1;
	for(;bcap<bytesize;bcap<<=1);
	if(bcap!=vrtx_bcap)
	{
		void *p2=realloc(vrtx, bcap);
		if(!p2)
		{
			LOG_ERROR("realloc(%p, %d) returned 0", vrtx, bcap);
			return 0;
		}
		vrtx=(float*)p2;
		vrtx_bcap=bcap;
	}
	return 1;
}
void				draw_ellipse(float x1, float x2, float y1, float y2, int color)
{
	float ya, yb;
	if(y1<y2)
		ya=y1, yb=y2;
	else
		ya=y2, yb=y1;
	int line_count=(int)ceil(yb)-(int)floor(ya);
	if(!vrtx_resize(line_count*2, 2))
		return;
	float x0=(x1+x2)*0.5f, y0=(y1+y2)*0.5f, rx=fabsf(x2-x0), ry=yb-y0;
	int nlines=0;
	float ygain=2.f/line_count;
	for(int kl=0;kl<line_count;++kl)//pixel-perfect ellipse (no anti-aliasing) drawn as horizontal lines
	{
		//ellipse equation: sq[(x-x0)/rx] + sq[(y-y0)/ry] = 1	->	x0 +- rx*sqrt(1 - sq[(y-y0)/ry])
		float
			y=kl*ygain-1,
			x=1-y*y;
		if(x<0)
			continue;
		x=rx*sqrtf(x);
		y=y*ry+y0;

		int idx=nlines<<2;
		y=screen2NDC_y(y);
		vrtx[idx  ]=screen2NDC_x(x0-x), vrtx[idx+1]=y;
		vrtx[idx+2]=screen2NDC_x(x0+x+1), vrtx[idx+3]=y;
		++nlines;
	}
	setGLProgram(shader_2D.program);		GL_CHECK();
	send_color(u_2D_color, color);			GL_CHECK();

	glEnableVertexAttribArray(a_2D_coords);	GL_CHECK();

	glBindBuffer(GL_ARRAY_BUFFER, vertex_buffer);								GL_CHECK();
	glBufferData(GL_ARRAY_BUFFER, nlines*4*sizeof(float), vrtx, GL_STATIC_DRAW);GL_CHECK();
	glVertexAttribPointer(a_2D_coords, 2, GL_FLOAT, GL_FALSE, 0, 0);			GL_CHECK();

//	glBindBuffer(GL_ARRAY_BUFFER, 0);		GL_CHECK();
//	glVertexAttribPointer(a_2D_coords, 2, GL_FLOAT, GL_FALSE, 0, vrtx);	GL_CHECK();
	
#ifndef NO_3D
	glDisable(GL_DEPTH_TEST);
#endif
	glDrawArrays(GL_LINES, 0, nlines*2);	GL_CHECK();
	glDisableVertexAttribArray(a_2D_coords);GL_CHECK();
#ifndef NO_3D
	glEnable(GL_DEPTH_TEST);
#endif
}

//text API
int					toggle_sdftext()
{
	if(sdf_available)
	{
		sdf_active=!sdf_active;
		memswap(&tdx, &sdf_dx, sizeof(float));
		memswap(&tdy, &sdf_dy, sizeof(float));
		if(sdf_active)
			font_zoom_min=0.25, font_zoom_max=64;
		else
		{
			if(font_zoom<1)
				font_zoom=1;
			else
				font_zoom=(float)(1<<floor_log2((int)font_zoom));
			font_zoom_min=1, font_zoom_max=32;
		}
	}
	return sdf_available;
}
long long			set_text_colors(long long colors)
{
	memswap(&colors_text, &colors, sizeof(long long));

	int *comp=(int*)&colors_text;
	if(sdf_active)
	{
		setGLProgram(shader_sdftext.program);
		send_color(u_sdftext_txtColor, comp[0]);
		send_color(u_sdftext_bkColor, comp[1]);
	}
	else
	{
		setGLProgram(shader_text.program);
		send_color(u_text_txtColor, comp[0]);
		send_color(u_text_bkColor, comp[1]);
	}
	return colors;
}
float				print_line(float tab_origin, float x, float y, float zoom, const char *msg, int msg_length, int req_cols, int *ret_idx, int *ret_cols)
{
	if(msg_length<1)
		return 0;
	float rect[4]={0};
	QuadCoords *txc=0, *atlas=sdf_active?sdf_glyph_coords:font_coords;
	//if(sdf_active)
	//	zoom*=16.f/sdf_txh;
	float width=tdx*zoom, height=tdy*zoom;
	int tab_origin_cols=(int)(tab_origin/width), idx, printable_count=0, cursor_cols=ret_cols?*ret_cols:0, advance_cols;
	if(y+height<ry0||y>=ry0+rdy)//off-screen optimization
		return 0;//no need to do anything, this line is outside the screen
	//	return idx2col(msg, msg_length, (int)(tab_origin/width))*width;
	float CX1=2.f/rdx, CX0=CX1*(x-rx0)-1;//delta optimization
	rect[1]=1-(y-ry0)*2.f/rdy;
	rect[3]=1-(y+height-ry0)*2.f/rdy;
	vrtx_resize(msg_length<<2, 4);//vx, vy, txx, txy		x4 vertices/char
	int k=ret_idx?*ret_idx:0;
	if(req_cols<0||cursor_cols<req_cols)
	{
		CX1*=width;
		for(;k<msg_length;++k)
		{
			char c=msg[k];
			if(c>=32&&c<0xFF)
				advance_cols=1;
			else if(c=='\t')
				advance_cols=tab_count-mod(cursor_cols-tab_origin_cols, tab_count), c=' ';
			else
				advance_cols=0;
			if(advance_cols)
			{
				if(x+(cursor_cols+advance_cols)*width>=rx0&&x+cursor_cols*width<rx0+rdx)//off-screen optimization
				{
					rect[0]=CX1*cursor_cols+CX0;//xn1
					cursor_cols+=advance_cols;
					rect[2]=CX1*cursor_cols+CX0;//xn2

					//rect[0]=(x+msg_width-rx0)*2.f/rdx-1;//xn1
					//rect[1]=1-(y-ry0)*2.f/rdy;//yn1
					//rect[2]=(x+msg_width+width-rx0)*2.f/rdx-1;//xn2
					//rect[3]=1-(y+height-ry0)*2.f/rdy;//yn2

					//toNDC_nobias(float(x+msg_width		), float(y			), rect[0], rect[1]);
					//toNDC_nobias(float(x+msg_width+width	), float(y+height	), rect[2], rect[3]);//y2<y1

					idx=printable_count<<4;
					txc=atlas+c-32;
					vrtx[idx   ]=rect[0], vrtx[idx+ 1]=rect[1],		vrtx[idx+ 2]=txc->x1, vrtx[idx+ 3]=txc->y1;//top left
					vrtx[idx+ 4]=rect[0], vrtx[idx+ 5]=rect[3],		vrtx[idx+ 6]=txc->x1, vrtx[idx+ 7]=txc->y2;//bottom left
					vrtx[idx+ 8]=rect[2], vrtx[idx+ 9]=rect[3],		vrtx[idx+10]=txc->x2, vrtx[idx+11]=txc->y2;//bottom right
					vrtx[idx+12]=rect[2], vrtx[idx+13]=rect[1],		vrtx[idx+14]=txc->x2, vrtx[idx+15]=txc->y1;//top right

					++printable_count;
				}
				else
					cursor_cols+=advance_cols;
				if(req_cols>=0&&cursor_cols>=req_cols)
				{
					++k;
					break;
				}
			}
		}
		if(printable_count)
		{
#ifndef NO_3D
			glDisable(GL_DEPTH_TEST);
#endif
			if(sdf_active)
			{
				setGLProgram(shader_sdftext.program);
				glUniform1f(u_sdftext_zoom, zoom*16.f/(sdf_txh*sdf_slope));
				select_texture(sdf_atlas_txid, u_sdftext_atlas);

				glBindBuffer(GL_ARRAY_BUFFER, vertex_buffer);							GL_CHECK();
				glBufferData(GL_ARRAY_BUFFER, printable_count<<6, vrtx, GL_STATIC_DRAW);GL_CHECK();
				glVertexAttribPointer(a_sdftext_coords, 4, GL_FLOAT, GL_TRUE, 0, 0);	GL_CHECK();

			//	glBindBuffer(GL_ARRAY_BUFFER, 0);										GL_CHECK();
			//	glVertexAttribPointer(a_sdftext_coords, 4, GL_FLOAT, GL_TRUE, 0, vrtx);	GL_CHECK();

				glEnableVertexAttribArray(a_sdftext_coords);	GL_CHECK();
				glDrawArrays(GL_QUADS, 0, printable_count<<2);	GL_CHECK();
				glDisableVertexAttribArray(a_sdftext_coords);	GL_CHECK();
			}
			else
			{
				setGLProgram(shader_text.program);
				select_texture(font_txid, u_text_atlas);

				glBindBuffer(GL_ARRAY_BUFFER, vertex_buffer);							GL_CHECK();
				glBufferData(GL_ARRAY_BUFFER, printable_count<<6, vrtx, GL_STATIC_DRAW);GL_CHECK();//set vertices & texcoords
				glVertexAttribPointer(a_text_coords, 4, GL_FLOAT, GL_TRUE, 0, 0);		GL_CHECK();

			//	glBindBuffer(GL_ARRAY_BUFFER, 0);									GL_CHECK();
			//	glVertexAttribPointer(a_text_coords, 4, GL_FLOAT, GL_TRUE, 0, vrtx);GL_CHECK();

				glEnableVertexAttribArray(a_text_coords);		GL_CHECK();
				glDrawArrays(GL_QUADS, 0, printable_count<<2);	GL_CHECK();//draw the quads: 4 vertices per character quad
				glDisableVertexAttribArray(a_text_coords);		GL_CHECK();
			}
#ifndef NO_3D
			glEnable(GL_DEPTH_TEST);
#endif
		}
	}
	if(ret_idx)
		*ret_idx=k;
	if(ret_cols)
		*ret_cols=cursor_cols;
	return cursor_cols*width;
}
float				GUIPrint(float tab_origin, float x, float y, float zoom, const char *format, ...)
{
	int len;
	va_list args;

	va_start(args, format);
	len=vsnprintf(g_buf, g_buf_size, format, args);
	va_end(args);
	return print_line(tab_origin, x, y, zoom, g_buf, len, -1, 0, 0);
}

void			display_texture_i(int x1, int x2, int y1, int y2, int *rgb, int txw, int txh, float alpha)
{
	static unsigned tx_id=0;
	float _2_w=2.f/w, _2_h=2.f/h;
	float rect[]=
	{
		x1*_2_w-1, 1-y1*_2_h,
		x2*_2_w-1, 1-y2*_2_h//y2<y1
	};
	float vrtx[]=
	{
		rect[0], rect[1],		0, 0,//top left
		rect[0], rect[3],		0, 1,//bottom left
		rect[2], rect[3],		1, 1,//bottom right
		rect[2], rect[1],		1, 0 //top right
	};
	if(rgb)
	{
	#define	NPOT_ATIX2300_FIX
		int *rgb2, w2, h2;
#ifdef NPOT_ATIX2300_FIX
		int expand;
		int logw=floor_log2(txw), logh=floor_log2(txh);
		if(expand=(!GLversion||GLversion[0]<'3')&&(txw>1<<logw||txh>1<<logh))
		{
			w2=txw>1<<logw?1<<(logw+1):txw;
			h2=txh>1<<logh?1<<(logh+1):txh;
			int size=w2*h2;
			rgb2=(int*)malloc(size<<2);
			memset(rgb2, 0, size<<2);
			for(int ky=0;ky<txh;++ky)
				memcpy(rgb2+w2*ky, rgb+txw*ky, txw<<2);
			float nw=(float)txw/w2, nh=(float)txh/h2;
			vrtx[ 2]=0,		vrtx[ 3]=0;
			vrtx[ 6]=0,		vrtx[ 7]=nh;
			vrtx[10]=nw,	vrtx[11]=nh;
			vrtx[14]=nw,	vrtx[15]=0;
		}
		else
#endif
			rgb2=rgb, w2=txw, h2=txh;
		setGLProgram(shader_texture.program);	GL_CHECK();
		glUniform1f(u_texture_alpha, alpha);	GL_CHECK();
		if(!tx_id)//generate texture id once
		{
			glGenTextures(1, &tx_id);			GL_CHECK();
		}
		glBindTexture(GL_TEXTURE_2D, tx_id);								GL_CHECK();
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);	GL_CHECK();
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);	GL_CHECK();
		glTexImage2D(GL_TEXTURE_2D,			0, GL_RGBA, w2, h2, 0, GL_RGBA, GL_UNSIGNED_BYTE, rgb2);	GL_CHECK();//send bitmap to GPU

		select_texture(tx_id, u_texture_texture);
		glBindBuffer(GL_ARRAY_BUFFER, vertex_buffer);				GL_CHECK();
		glBufferData(GL_ARRAY_BUFFER, 16<<2, vrtx, GL_STATIC_DRAW);	GL_CHECK();//send vertices & texcoords

		glVertexAttribPointer(a_texture_coords, 4, GL_FLOAT, GL_FALSE, 4<<2, (void*)0);		GL_CHECK();//select vertices & texcoord

#ifndef NO_3D
		glDisable(GL_DEPTH_TEST);
#endif
		glEnableVertexAttribArray(a_texture_coords);	GL_CHECK();
		glDrawArrays(GL_QUADS, 0, 4);					GL_CHECK();//draw the quad
		glDisableVertexAttribArray(a_texture_coords);	GL_CHECK();
#ifndef NO_3D
		glEnable(GL_DEPTH_TEST);
#endif
#ifdef NPOT_ATIX2300_FIX
		if(expand)
			free(rgb2);
#endif
	}
}

//3D
#ifndef NO_3D
void			mat4_lookAt(float *dst, const float *cam, const float *center, const float *up)
{
	float f[3], t1, u[3], s[3];
	vec3_sub(f, center, cam);
	vec3_normalize(f, f, t1);
	vec3_normalize(u, up, t1);
	vec3_cross(s, f, u);
	vec3_normalize(s, s, t1);
	vec3_cross(u, s, f);
	dst[0]=s[0], dst[4]=s[1], dst[8]=s[2], dst[12]=-vec3_dot(s, cam);
	dst[1]=u[0], dst[5]=u[1], dst[9]=u[2], dst[13]=-vec3_dot(u, cam);
	dst[2]=f[0], dst[6]=f[1], dst[10]=f[2], dst[14]=-vec3_dot(f, cam);
	dst[3]=0, dst[7]=0, dst[11]=0, dst[15]=1;
}
void			mat4_FPSView(float *dst, const float *campos, float yaw, float pitch)
{
	float _cam[]={-campos[1], campos[2], -campos[0]};
	//float _cam[]={campos[0], campos[1], campos[2]};
	float cos_p=cosf(pitch), sin_p=sinf(pitch), cos_y=cosf(yaw), sin_y=sinf(yaw);
	float
		xaxis[]={cos_y, 0, -sin_y},
		yaxis[]={sin_y*sin_p, cos_p, cos_y*sin_p},
		zaxis[]={sin_y*cos_p, -sin_p, cos_p*cos_y};

	dst[0]=xaxis[2], dst[1]=yaxis[2], dst[2]=zaxis[2], dst[3]=0;
	dst[4]=xaxis[0], dst[5]=yaxis[0], dst[6]=zaxis[0], dst[7]=0;
	dst[8]=xaxis[1], dst[9]=yaxis[1], dst[10]=zaxis[1], dst[11]=0;

	//dst[0]=xaxis[0], dst[1]=yaxis[0], dst[2]=zaxis[0], dst[3]=0;
	//dst[4]=xaxis[1], dst[5]=yaxis[1], dst[6]=zaxis[1], dst[7]=0;
	//dst[8]=xaxis[2], dst[9]=yaxis[2], dst[10]=zaxis[2], dst[11]=0;

	dst[12]=-vec3_dot(xaxis, _cam), dst[13]=-vec3_dot(yaxis, _cam), dst[14]=-vec3_dot(zaxis, _cam), dst[15]=1;
}
void			mat4_perspective(float *dst, float tanfov, float w_by_h, float znear, float zfar)
{
	dst[0]=1/tanfov,	dst[1]=0,				dst[2]=0,							dst[3]=0;
	dst[4]=0,			dst[5]=w_by_h/tanfov,	dst[6]=0,							dst[7]=0;
	dst[8]=0,			dst[9]=0,				dst[10]=-(zfar+znear)/(zfar-znear), dst[11]=-1;
	dst[12]=0,			dst[13]=0,				dst[14]=-2*zfar*znear/(zfar-znear), dst[15]=0;
}
void			GetTransformInverseNoScale(float *dst, const float *src)// Requires this matrix to be transform matrix, NoScale version requires this matrix be of scale 1
{
	//https://lxjk.github.io/2017/09/03/Fast-4x4-Matrix-Inverse-with-SSE-SIMD-Explained.html
#define MakeShuffleMask(x,y,z,w)           (x | (y<<2) | (z<<4) | (w<<6))

// vec(0, 1, 2, 3) -> (vec[x], vec[y], vec[z], vec[w])
#define VecSwizzleMask(vec, mask)          _mm_castsi128_ps(_mm_shuffle_epi32(_mm_castps_si128(vec), mask))
#define VecSwizzle(vec, x, y, z, w)        VecSwizzleMask(vec, MakeShuffleMask(x,y,z,w))
#define VecSwizzle1(vec, x)                VecSwizzleMask(vec, MakeShuffleMask(x,x,x,x))
// special swizzle
#define VecSwizzle_0022(vec)               _mm_moveldup_ps(vec)
#define VecSwizzle_1133(vec)               _mm_movehdup_ps(vec)

// return (vec1[x], vec1[y], vec2[z], vec2[w])
#define VecShuffle(vec1, vec2, x,y,z,w)    _mm_shuffle_ps(vec1, vec2, MakeShuffleMask(x,y,z,w))
// special shuffle
#define VecShuffle_0101(vec1, vec2)        _mm_movelh_ps(vec1, vec2)
#define VecShuffle_2323(vec1, vec2)        _mm_movehl_ps(vec2, vec1)
	__m128 inM[4], ret[4];
	
	inM[0]=_mm_loadu_ps(src);
	inM[1]=_mm_loadu_ps(src+4);
	inM[2]=_mm_loadu_ps(src+8);
	inM[3]=_mm_loadu_ps(src+12);

	// transpose 3x3, we know m03 = m13 = m23 = 0
	__m128 t0 = VecShuffle_0101(inM[0], inM[1]); // 00, 01, 10, 11
	__m128 t1 = VecShuffle_2323(inM[0], inM[1]); // 02, 03, 12, 13
	ret[0] = VecShuffle(t0, inM[2], 0,2,0,3); // 00, 10, 20, 23(=0)
	ret[1] = VecShuffle(t0, inM[2], 1,3,1,3); // 01, 11, 21, 23(=0)
	ret[2] = VecShuffle(t1, inM[2], 0,2,2,3); // 02, 12, 22, 23(=0)

	// last line
	ret[3] =                    _mm_mul_ps(ret[0], VecSwizzle1(inM[3], 0));
	ret[3] = _mm_add_ps(ret[3], _mm_mul_ps(ret[1], VecSwizzle1(inM[3], 1)));
	ret[3] = _mm_add_ps(ret[3], _mm_mul_ps(ret[2], VecSwizzle1(inM[3], 2)));
	ret[3] = _mm_sub_ps(_mm_setr_ps(0.f, 0.f, 0.f, 1.f), ret[3]);

	_mm_storeu_ps(dst, ret[0]);
	_mm_storeu_ps(dst+4, ret[1]);
	_mm_storeu_ps(dst+8, ret[2]);
	_mm_storeu_ps(dst+12, ret[3]);
#undef MakeShuffleMask
#undef VecSwizzleMask
#undef VecSwizzle
#undef VecSwizzle1
#undef VecSwizzle_0022
#undef VecSwizzle_1133
#undef VecShuffle
#undef VecShuffle_0101
#undef VecShuffle_2323
}
void			mat4_normalmat3(float *dst, float *m4)//inverse transpose of top left 3x3 submatrix
{
	float temp[16];
	GetTransformInverseNoScale(temp, m4);
	vec3_copy(dst, temp);
	vec3_copy(dst+3, temp+4);
	vec3_copy(dst+6, temp+8);
}
void			gpubuf_send_VNT(GPUModel *dst, const float *VVVNNNTT, int n_floats, const int *indices, int n_ints)
{
	dst->n_elements=n_ints, dst->stride=8*sizeof(float);
	dst->vertices_start=0;
	dst->normals_start=3*sizeof(float);
	dst->txcoord_start=6*sizeof(float);
	glGenBuffers(2, &dst->VBO);
	glBindBuffer(GL_ARRAY_BUFFER, dst->VBO);											GL_CHECK();
	glBufferData(GL_ARRAY_BUFFER, n_floats*sizeof(float), VVVNNNTT, GL_STATIC_DRAW);	GL_CHECK();
	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, dst->EBO);									GL_CHECK();
	glBufferData(GL_ELEMENT_ARRAY_BUFFER, n_ints*sizeof(int), indices, GL_STATIC_DRAW);	GL_CHECK();
}
void			draw_L3D(Camera const *cam, GPUModel const *model, const float *modelpos, const float *lightpos, int lightcolor)
{
	float mView[16], mProj[16], mVP[16], mMVP_Model[32], mNormal[9], sceneInfo[9];
	__m128 temp1;

	setGLProgram(shader_L3D.program);

	mat4_FPSView(mView, &cam->x, cam->ax, cam->ay);
	mat4_perspective(mProj, cam->tanfov, (float)(w)/h, 0.1f, 1000.f);
	mat4_mul_mat4(mVP, mProj, mView, temp1);

	float *mMVP=mMVP_Model, *mModel=mMVP_Model+16;
	mat4_identity(mModel, 1);
	mat4_translate(mModel, modelpos, temp1);
	mat4_normalmat3(mNormal, mModel);
	mat4_mul_mat4(mMVP, mVP, mModel, temp1);

	unsigned char *p=(unsigned char*)&lightcolor;
	memcpy(sceneInfo, lightpos, 3*sizeof(float));
	sceneInfo[3]=p[0]*inv255;
	sceneInfo[4]=p[1]*inv255;
	sceneInfo[5]=p[2]*inv255;
	memcpy(sceneInfo+6, &cam->x, 3*sizeof(float));
	
	glUniformMatrix4fv(u_L3D_matVP_Model, 2, GL_FALSE, mMVP_Model);	GL_CHECK();
	glUniformMatrix3fv(u_L3D_matNormal, 1, GL_FALSE, mNormal);		GL_CHECK();
	glUniform3fv(u_L3D_sceneInfo, 3, sceneInfo);	GL_CHECK();
//	glBindVertexArray(s_VAO);						GL_CHECK();

	select_texture(model->txid, u_L3D_texture);
		
	glBindBuffer(GL_ARRAY_BUFFER, model->VBO);			GL_CHECK();
	glEnableVertexAttribArray(a_L3D_vertex);			GL_CHECK();
	glVertexAttribPointer(a_L3D_vertex, 3, GL_FLOAT, GL_FALSE, model->stride, (void*)(long long)model->vertices_start);	GL_CHECK();
	glEnableVertexAttribArray(a_L3D_normal);			GL_CHECK();
	glVertexAttribPointer(a_L3D_normal, 3, GL_FLOAT, GL_FALSE, model->stride, (void*)(long long)model->normals_start);	GL_CHECK();
	glEnableVertexAttribArray(a_L3D_texcoord);			GL_CHECK();
	glVertexAttribPointer(a_L3D_texcoord, 2, GL_FLOAT, GL_FALSE, model->stride, (void*)(long long)model->txcoord_start);GL_CHECK();
	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, model->EBO);	GL_CHECK();

	glDrawElements(GL_TRIANGLES, model->n_elements, GL_UNSIGNED_INT, 0);	GL_CHECK();
	glDisableVertexAttribArray(a_L3D_vertex);			GL_CHECK();
	glDisableVertexAttribArray(a_L3D_normal);			GL_CHECK();
	glDisableVertexAttribArray(a_L3D_texcoord);			GL_CHECK();
}
#endif
