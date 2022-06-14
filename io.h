//io.h - A cross-platform window creation, event handling and immediate graphics library
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

#pragma once
#ifndef IO_H
#define IO_H
#ifdef __cplusplus
extern "C"
{
#endif
#ifdef _MSC_VER
#define _CRT_SECURE_NO_WARNINGS
#endif
#include<stddef.h>//for size_t


//globals
extern int
	w, h,	//window dimensions
	mx, my;	//mouse position inside window
extern char	timer, keyboard[256];

#define	g_buf_size	1024
extern char	g_buf[g_buf_size];
extern char	*exe_dir;
extern int	exe_dir_len;

//OpenGL globals
extern const char	*GLversion;
extern int			rx0, ry0, rdx, rdy;//current OpenGL region
extern float		SN_x0, SN_x1, SN_y0, SN_y1, NS_x0, NS_x1, NS_y0, NS_y1;//screen-NDC conversion
#define				screen2NDC_x(Xs)		(SN_x1*(Xs		)+SN_x0)
#define				screen2NDC_x_bias(Xs)	(SN_x1*(Xs+0.5f	)+SN_x0)
#define				screen2NDC_y(Ys)		(SN_y1*(Ys		)+SN_y0)
#define				screen2NDC_y_bias(Ys)	(SN_y1*(Ys+0.5f	)+SN_y0)
#define				NDC2screen_x(X)			(NS_x1*(X)+NS_x0)
#define				NDC2screen_x_bias(X)	(NS_x1*(X)+NS_x0-0.5f)
#define				NDC2screen_y(Y)			(NS_y1*(Y)+NS_y0)
#define				NDC2screen_y_bias(Y)	(NS_y1*(Y)+NS_y0-0.5f)

extern char			sdf_available, sdf_active;
extern short		tab_count;//default is 8 characters
extern float		tdx, tdy;//non-tab character dimensions at 1x zoom
extern float		font_zoom, font_zoom_min, font_zoom_max, sdf_dzoom;
extern long long	colors_text;//0xBKBKBKBK_TXTXTXTX

//math constants
extern float
	_pi, _2pi, pi_2, inv_2pi,
	sqrt2,
	torad, todeg,
	infinity,
	inv255;

//utility
#define		SIZEOF(X)	(sizeof(X)/sizeof(*(X)))
#ifdef _MSC_VER
#define		ALIGN(X)	__declspec(align(X))
#elif defined __GNUC__
#define		ALIGN(X)	__attribute__((aligned(X)))
#endif
void memswap(void *p1, void *p2, size_t size);
void memfill(void *dst, const void *src, size_t dstbytes, size_t srcbytes);

//math
int		mod(int x, int n);
float	modulof(float x, float n);
int		floor_log2(unsigned n);
int		minimum(int a, int b);
int		maximum(int a, int b);
int		clamp(int lo, int x, int hi);
float	clampf(float lo, float x, float hi);

//array
#if 1
#ifdef DEBUG_INFO_STR
typedef const char *DebugInfo;
#else
typedef size_t DebugInfo;
#endif
typedef struct ArrayHeaderStruct
{
	size_t count, esize, cap;//cap is in bytes
	DebugInfo debug_info;
	unsigned char data[];
} ArrayHeader, *ArrayHandle;
ArrayHandle		array_construct(const void *src, size_t esize, size_t count, size_t rep, size_t pad, DebugInfo debug_info);
ArrayHandle		array_copy(ArrayHandle *arr, DebugInfo debug_info);//shallow
void			array_free(ArrayHandle *arr);
void			array_clear(ArrayHandle *arr);//keeps allocation
void			array_fit(ArrayHandle *arr, size_t pad);

void*			array_insert(ArrayHandle *arr, size_t idx, void *data, size_t count, size_t rep, size_t pad);//cannot be nullptr

size_t			array_size(ArrayHandle const *arr);
void*			array_at(ArrayHandle *arr, size_t idx);
const void*		array_at_const(ArrayHandle const *arr, int idx);
void*			array_back(ArrayHandle *arr);
const void*		array_back_const(ArrayHandle const *arr);

#ifdef DEBUG_INFO_STR
#define			ARRAY_ALLOC(ELEM_TYPE, ARR, COUNT, PAD, DEBUG_INFO)	ARR=array_construct(0, sizeof(ELEM_TYPE), COUNT, 1, PAD, DEBUG_INFO)
#else
#define			ARRAY_ALLOC(ELEM_TYPE, ARR, COUNT, PAD)				ARR=array_construct(0, sizeof(ELEM_TYPE), COUNT, 1, PAD, __LINE__)
#endif
#define			ARRAY_APPEND(ARR, DATA, COUNT, REP, PAD)	array_insert(&(ARR), array_size(&(ARR)), DATA, COUNT, REP, PAD)
#define			ARRAY_DATA(ARR)			(ARR)->data
#define			ARRAY_I(ARR, IDX)		*(int*)array_at(&ARR, IDX)
#define			ARRAY_U(ARR, IDX)		*(unsigned*)array_at(&ARR, IDX)
#define			ARRAY_F(ARR, IDX)		*(double*)array_at(&ARR, IDX)


//null terminated array
#ifdef DEBUG_INFO_STR
#define			ESTR_ALLOC(TYPE, STR, LEN, DEBUG_INFO)				STR=array_construct(0, sizeof(TYPE), 0, 1, LEN+1, DEBUG_INFO)
#define			ESTR_COPY(TYPE, STR, SRC, LEN, REP, DEBUG_INFO)		STR=array_construct(SRC, sizeof(TYPE), LEN, REP, 1, DEBUG_INFO)
#else
#define			ESTR_ALLOC(TYPE, STR, LEN)				STR=array_construct(0, sizeof(TYPE), 0, 1, LEN+1, __LINE__)
#define			ESTR_COPY(TYPE, STR, SRC, LEN, REP)		STR=array_construct(SRC, sizeof(TYPE), LEN, REP, 1, __LINE__)
#endif
#define			STR_APPEND(STR, SRC, LEN, REP)			array_insert(&(STR), array_size(&(STR)), SRC, LEN, REP, 1)
#define			STR_FIT(STR)							array_fit(&STR, 1)
#define			ESTR_AT(TYPE, STR, IDX)					*(TYPE*)array_at(&(STR), IDX)

#define			STR_ALLOC(STR, LEN, ...)				ESTR_ALLOC(char, STR, LEN, ##__VA_ARGS__)
#define			STR_COPY(STR, SRC, LEN, REP, ...)		ESTR_COPY(char, STR, SRC, LEN, REP, ##__VA_ARGS__)
#define			STR_AT(STR, IDX)						ESTR_AT(char, STR, IDX)

#define			WSTR_ALLOC(STR, LEN, ...)				ESTR_ALLOC(wchar_t, STR, LEN, ##__VA_ARGS__)
#define			WSTR_COPY(STR, SRC, LEN, REP, ...)		ESTR_COPY(wchar_t, STR, SRC, LEN, REP, ##__VA_ARGS__)
#define			WSTR_AT(STR, IDX)						ESTR_AT(wchar_t, STR, IDX)
#endif


//profiler

//		#define PROFILER_CYCLES

extern char	prof_on;
void		prof_start();
void		prof_add(const char *label);
void		prof_sum(const char *label, int count);//add the sum of last 'count' steps
void		prof_loop_start(const char **labels, int n);//describe the loop body parts in 'labels'
void		prof_add_loop(int idx);//call on each part of loop body
void		prof_print();


typedef enum IOKeyEnum
{
#if defined _MSC_VER || defined _WINDOWS || defined _WIN32
#	define	IOKEY(LinVAL, VAL, LABEL)	KEY_##LABEL=VAL,
#elif defined __linux__
#	define	IOKEY(VAL, WinVAL, LABEL)	KEY_##LABEL=VAL,
#endif
	
//IO value, win32 value, label
IOKEY(0x00, 0x00, UNKNOWN)


//direct map keys
IOKEY(0x08, 0x08, BKSP)
IOKEY(0x09, 0x09, TAB)

IOKEY(0x0D, 0x0D, ENTER)

IOKEY(0x1B, 0x1B, ESC)

IOKEY(0x20, 0x20, SPACE)
IOKEY(0x30, 0x30, 0)
IOKEY(0x31, 0x31, 1)
IOKEY(0x32, 0x32, 2)
IOKEY(0x33, 0x33, 3)
IOKEY(0x34, 0x34, 4)
IOKEY(0x35, 0x35, 5)
IOKEY(0x36, 0x36, 6)
IOKEY(0x37, 0x37, 7)
IOKEY(0x38, 0x38, 8)
IOKEY(0x39, 0x39, 9)
IOKEY(0x41, 0x41, A)
IOKEY(0x41, 0x41, B)
IOKEY(0x41, 0x41, C)
IOKEY(0x41, 0x41, D)
IOKEY(0x41, 0x41, E)
IOKEY(0x41, 0x41, F)
IOKEY(0x41, 0x41, G)
IOKEY(0x41, 0x41, H)
IOKEY(0x41, 0x41, I)
IOKEY(0x41, 0x41, J)
IOKEY(0x41, 0x41, K)
IOKEY(0x41, 0x41, L)
IOKEY(0x41, 0x41, M)
IOKEY(0x41, 0x41, N)
IOKEY(0x41, 0x41, O)
IOKEY(0x41, 0x41, P)
IOKEY(0x41, 0x41, Q)
IOKEY(0x41, 0x41, R)
IOKEY(0x41, 0x41, S)
IOKEY(0x41, 0x41, T)
IOKEY(0x41, 0x41, U)
IOKEY(0x41, 0x41, V)
IOKEY(0x41, 0x41, W)
IOKEY(0x41, 0x41, X)
IOKEY(0x41, 0x41, Y)
IOKEY(0x41, 0x41, Z)


//other keys
IOKEY(0x13, 0x13, PAUSE)
IOKEY(0x14, 0x91, SCROLLLOCK)
IOKEY(0x15, 0x2C, PRINTSCR)

IOKEY(0x27, 0xDE, QUOTE)		//inserted '\'' with '\"' 0x22
IOKEY(0x2B, 0xBB, PLUS)			//inserted '+' with '=' 0x3D
IOKEY(0x2C, 0xBC, COMMA)		//inserted ',' with '<' 0x3C
IOKEY(0x2D, 0xBD, MINUS)		//inserted '-' with '_' 0x5F
IOKEY(0x2E, 0xBE, PERIOD)		//inserted '.' with '>' 0x3E
IOKEY(0x2F, 0xBF, SLASH)		//inserted '/' with '?' 0x3F

IOKEY(0x3B, 0xBA, SEMICOLON)	//inserted ';' with ':' 0x3A

IOKEY(0x5B, 0xDB, LBRACKET)		//inserted '[' with '{' 0x7B
IOKEY(0x5C, 0xDC, BACKSLASH)	//inserted '\\' with '|' 0x7C
IOKEY(0x5D, 0xDD, RBRACKET)		//inserted ']' with '}' 0x7D
IOKEY(0x60, 0xC0, GRAVEACCENT)	//inserted '`' with '~' 0x7E

IOKEY(0x7F, 0x2E, DEL)

IOKEY(0x81, 0x01, LBUTTON)		//inserted
IOKEY(0x82, 0x04, MBUTTON)		//inserted
IOKEY(0x83, 0x02, RBUTTON)		//inserted

IOKEY(0x84, 0x10, SHIFT)		//inserted
IOKEY(0x85, 0x11, CTRL)			//inserted
IOKEY(0x86, 0x12, ALT)			//inserted
IOKEY(0x87, 0x00, START)		//inserted

IOKEY(0xA0, 0x60, NP_0)
IOKEY(0xA1, 0x61, NP_1)
IOKEY(0xA2, 0x62, NP_2)
IOKEY(0xA3, 0x63, NP_3)
IOKEY(0xA4, 0x64, NP_4)
IOKEY(0xA5, 0x65, NP_5)
IOKEY(0xA6, 0x66, NP_6)
IOKEY(0xA7, 0x67, NP_7)
IOKEY(0xA8, 0x68, NP_8)
IOKEY(0xA9, 0x69, NP_9)
IOKEY(0xAA, 0x6A, NP_MUL)
IOKEY(0xAB, 0x6B, NP_PLUS)
IOKEY(0xAC, 0x6D, NP_MINUS)
IOKEY(0xAD, 0x6E, NP_PERIOD)
IOKEY(0xAE, 0x6F, NP_DIV)

IOKEY(0xBE, 0x70, F1)
IOKEY(0xBF, 0x71, F2)
IOKEY(0xC0, 0x72, F3)
IOKEY(0xC1, 0x73, F4)
IOKEY(0xC2, 0x74, F5)
IOKEY(0xC3, 0x75, F6)
IOKEY(0xC4, 0x76, F7)
IOKEY(0xC5, 0x77, F8)
IOKEY(0xC6, 0x78, F9)
IOKEY(0xC7, 0x79, F10)
IOKEY(0xC8, 0x7A, F11)
IOKEY(0xC9, 0x7B, F12)

IOKEY(0xD0, 0x24, HOME)
IOKEY(0xD1, 0x25, LEFT)
IOKEY(0xD2, 0x26, UP)
IOKEY(0xD3, 0x27, RIGHT)
IOKEY(0xD4, 0x28, DOWN)
IOKEY(0xD5, 0x21, PGUP)
IOKEY(0xD6, 0x22, PGDN)
IOKEY(0xD7, 0x23, END)

IOKEY(0xE5, 0x14, CAPSLOCK)

IOKEY(0xE3, 0x2D, INSERT)
IOKEY(0xEB, 0x03, BREAK)

IOKEY(0xFF, 0x90, NUMLOCK)

#undef	IOKEY
} IOKey;
#if defined _MSC_VER || defined _WINDOWS || defined _WIN32
int			get_key_state(int key);
#else
#define		get_key_state(KEY)	keyboard[KEY]
#endif


//callbacks - implement these in your application:
int io_init(int argc, char **argv);//return false to abort
void io_resize();
int io_mousemove();//return true to redraw
int io_mousewheel(int forward);
int io_keydn(IOKey key, char c);
int io_keyup(IOKey key, char c);
void io_timer();
void io_render();
int io_quit_request();//return 1 to exit
void io_cleanup();//cleanup

//shaders
#ifdef IO_IMPLEMENTATION
#define		SHADER_LIST		SHADER(2D) SHADER(text) SHADER(sdftext) SHADER(texture)

//shader_2D
#define		ATTR_2D			ATTR(2D, coords)
#define		UNIF_2D			UNIF(2D, color)
const char
	src_vert_2D[]=
		"#version 120\n"
		"attribute vec2 a_coords;\n"		//attributes: a_coords
		"void main()\n"
		"{\n"
		"    gl_Position=vec4(a_coords, 0., 1.);\n"
		"}",
	src_frag_2D[]=
		"#version 120\n"
		"uniform vec4 u_color;\n"			//uniforms: u_color
		"void main()\n"
		"{\n"
		"    gl_FragColor=u_color;\n"
		"}";

//shader_text
#define		ATTR_text		ATTR(text, coords)
#define		UNIF_text		UNIF(text, atlas) UNIF(text, txtColor) UNIF(text, bkColor)
const char
	src_vert_text[]=
		"#version 120\n"
		"attribute vec4 a_coords;"			//attributes: a_coords
		"varying vec2 v_texcoord;\n"
		"void main()\n"
		"{\n"
		"    gl_Position=vec4(a_coords.xy, 0., 1.);\n"
		"    v_texcoord=a_coords.zw;\n"
		"}",
	src_frag_text[]=
		"#version 120\n"
		"varying vec2 v_texcoord;\n"
		"uniform sampler2D u_atlas;\n"		//uniforms: u_atlas, u_txtColor, u_bkColor
		"uniform vec4 u_txtColor, u_bkColor;\n"
		"void main()\n"
		"{\n"
		"    vec4 region=texture2D(u_atlas, v_texcoord);\n"
		"    gl_FragColor=mix(u_txtColor, u_bkColor, region.r);\n"//u_txtColor*(1-region.r) + u_bkColor*region.r
		"}";

//shader_sdftext
#define		ATTR_sdftext	ATTR(sdftext, coords)
#define		UNIF_sdftext	UNIF(sdftext, atlas) UNIF(sdftext, txtColor) UNIF(sdftext, bkColor) UNIF(sdftext, zoom)
const char
	src_vert_sdftext[]=
		"#version 120\n"
		"attribute vec4 a_coords;"			//attributes: a_coords
		"varying vec2 v_texcoord;\n"
		"void main()\n"
		"{\n"
		"    gl_Position=vec4(a_coords.xy, 0., 1.);\n"
		"    v_texcoord=a_coords.zw;\n"
		"}",
	src_frag_sdftext[]=
		"#version 120\n"
		"varying vec2 v_texcoord;\n"
		"uniform sampler2D u_atlas;\n"		//uniforms: u_atlas, u_txtColor, u_bkColor, u_zoom
		"uniform vec4 u_txtColor, u_bkColor;\n"
		"uniform float u_zoom;\n"
		"void main()\n"
		"{\n"
		"    vec4 region=texture2D(u_atlas, v_texcoord);\n"

		"    float temp=clamp(u_zoom*(0.5f+0.45f/u_zoom-region.r), 0, 1);\n"
	//	"    float temp=clamp(u_zoom*(0.5f+0.001f*u_zoom-region.r), 0, 1);\n"
		"    gl_FragColor=mix(u_txtColor, u_bkColor, temp);\n"

	//	"    gl_FragColor=region.r>=0.5f?u_txtColor:u_bkColor;\n"//no anti-aliasing
		"}";

//shader_texture
#define		ATTR_texture	ATTR(texture, coords)
#define		UNIF_texture	UNIF(texture, texture) UNIF(texture, alpha)
const char
	src_vert_texture[]=
		"#version 120\n"
		"attribute vec4 a_coords;"			//attributes: a_coords
		"varying vec2 v_texcoord;\n"
		"void main()\n"
		"{\n"
		"    gl_Position=vec4(a_coords.xy, 0., 1.);\n"
		"    v_texcoord=a_coords.zw;\n"
		"}",
	src_frag_texture[]=
		"#version 120\n"
		"varying vec2 v_texcoord;\n"
		"uniform sampler2D u_texture;\n"	//uniforms: u_texture, u_alpha
		"uniform float u_alpha;\n"
		"void main()\n"
		"{\n"
		"    gl_FragColor=texture2D(u_texture, v_texcoord);\n"
		"    gl_FragColor.a*=u_alpha;\n"
		"}";
#endif


//API
double		time_ms();
void		set_window_title(const char *format, ...);

typedef enum MessageBoxTypeEnum
{
	MBOX_OK,
	MBOX_OKCANCEL,
	MBOX_YESNOCANCEL,
} MessageBoxType;
int			messagebox(MessageBoxType type, const char *title, const char *format, ...);//returns index of pressed button

void		copy_to_clipboard_c(const char *a, int size);
char*		paste_from_clipboard(int loud, int *ret_len);//don't forget to free memory

void		timer_start();
void		timer_stop();

int			log_error(const char *file, int line, const char *format, ...);
int			valid(const void *p);
#define		LOG_ERROR(format, ...)	log_error(file, __LINE__, format, ##__VA_ARGS__)
#define		ASSERT(SUCCESS)			((SUCCESS)!=0||log_error(file, __LINE__, #SUCCESS))
#define		ASSERT_P(POINTER)		(valid(POINTER)||log_error(file, __LINE__, #POINTER " == 0"))

#if defined _MSC_VER || defined _WINDOWS || defined _WIN32
void		console_show();
void		console_hide();
#else
#define		console_show()
#define		console_hide()
#endif
void		console_pause();


//Graphics API
#if 1
//extended OpenGL API
#if defined _MSC_VER || defined _WINDOWS || defined _WIN32
#define				GL_FUNC_ADD				0x8006//GL/glew.h
#define				GL_MIN					0x8007
#define				GL_MAX					0x8008
#define				GL_MAJOR_VERSION		0x821B
#define				GL_MINOR_VERSION		0x821C
#define				GL_TEXTURE0				0x84C0
#define				GL_TEXTURE1				0x84C1
#define				GL_TEXTURE2				0x84C2
#define				GL_TEXTURE3				0x84C3
#define				GL_TEXTURE4				0x84C4
#define				GL_TEXTURE5				0x84C5
#define				GL_TEXTURE6				0x84C6
#define				GL_TEXTURE7				0x84C7
#define				GL_TEXTURE8				0x84C8
#define				GL_TEXTURE9				0x84C9
#define				GL_TEXTURE10			0x84CA
#define				GL_TEXTURE11			0x84CB
#define				GL_TEXTURE12			0x84CC
#define				GL_TEXTURE13			0x84CD
#define				GL_TEXTURE14			0x84CE
#define				GL_TEXTURE15			0x84CF
#define				GL_TEXTURE_RECTANGLE	0x84F5
#define				GL_PROGRAM_POINT_SIZE	0x8642
#define				GL_BUFFER_SIZE			0x8764
#define				GL_ARRAY_BUFFER			0x8892
#define				GL_ELEMENT_ARRAY_BUFFER	0x8893
#define				GL_STATIC_DRAW			0x88E4
#define				GL_FRAGMENT_SHADER		0x8B30
#define				GL_VERTEX_SHADER		0x8B31
#define				GL_COMPILE_STATUS		0x8B81
#define				GL_LINK_STATUS			0x8B82
#define				GL_INFO_LOG_LENGTH		0x8B84
#define				GL_DEBUG_OUTPUT			0x92E0//OpenGL 4.3+
extern void			(__stdcall *glBlendEquation)(unsigned mode);
//extern void		(__stdcall *glGenVertexArrays)(int n, unsigned *arrays);//OpenGL 3.0
//extern void		(__stdcall *glDeleteVertexArrays)(int n, unsigned *arrays);//OpenGL 3.0
extern void			(__stdcall *glBindVertexArray)(unsigned arr);//OpenGL 3.0
extern void			(__stdcall *glGenBuffers)(int n, unsigned *buffers);
extern void			(__stdcall *glBindBuffer)(unsigned target, unsigned buffer);
extern void			(__stdcall *glBufferData)(unsigned target, int size, const void *data, unsigned usage);
extern void			(__stdcall *glBufferSubData)(unsigned target, int offset, int size, const void *data);
extern void			(__stdcall *glEnableVertexAttribArray)(unsigned index);
extern void			(__stdcall *glVertexAttribPointer)(unsigned index, int size, unsigned type, unsigned char normalized, int stride, const void *pointer);
extern void			(__stdcall *glDisableVertexAttribArray)(unsigned index);
extern unsigned		(__stdcall *glCreateShader)(unsigned shaderType);
extern void			(__stdcall *glShaderSource)(unsigned shader, int count, const char **string, const int *length);
extern void			(__stdcall *glCompileShader)(unsigned shader);
extern void			(__stdcall *glGetShaderiv)(unsigned shader, unsigned pname, int *params);
extern void			(__stdcall *glGetShaderInfoLog)(unsigned shader, int maxLength, int *length, char *infoLog);
extern unsigned		(__stdcall *glCreateProgram)();
extern void			(__stdcall *glAttachShader)(unsigned program, unsigned shader);
extern void			(__stdcall *glLinkProgram)(unsigned program);
extern void			(__stdcall *glGetProgramiv)(unsigned program, unsigned pname, int *params);
extern void			(__stdcall *glGetProgramInfoLog)(unsigned program, int maxLength, int *length, char *infoLog);
extern void			(__stdcall *glDetachShader)(unsigned program, unsigned shader);
extern void			(__stdcall *glDeleteShader)(unsigned shader);
extern void			(__stdcall *glUseProgram)(unsigned program);
extern int			(__stdcall *glGetAttribLocation)(unsigned program, const char *name);
extern void			(__stdcall *glDeleteProgram)(unsigned program);
extern void			(__stdcall *glDeleteBuffers)(int n, const unsigned *buffers);
extern int			(__stdcall *glGetUniformLocation)(unsigned program, const char *name);
extern void			(__stdcall *glUniformMatrix3fv)(int location, int count, unsigned char transpose, const float *value);
extern void			(__stdcall *glUniformMatrix4fv)(int location, int count, unsigned char transpose, const float *value);
extern void			(__stdcall *glGetBufferParameteriv)(unsigned target, unsigned value, int *data);
extern void			(__stdcall *glActiveTexture)(unsigned texture);
extern void			(__stdcall *glUniform1i)(int location, int v0);
extern void			(__stdcall *glUniform2i)(int location, int v0, int v1);
extern void			(__stdcall *glUniform1f)(int location, float v0);
extern void			(__stdcall *glUniform2f)(int location, float v0, float v1);
extern void			(__stdcall *glUniform3f)(int location, float v0, float v1, float v2);
extern void			(__stdcall *glUniform3fv)(int location, int count, const float *value);
extern void			(__stdcall *glUniform4f)(int location, float v0, float v1, float v2, float v3);
#endif

extern unsigned		current_program;
typedef struct		ShaderVarStruct
{
	int *pvar;//initialize to -1
	const char *name;
} ShaderVar;
typedef struct		ShaderProgramStruct
{
	const char *name,//program name for error reporting
		*vsrc, *fsrc;
	ShaderVar *attributes, *uniforms;
	int n_attr, n_unif;
	unsigned program;//initialize to 0
} ShaderProgram;
int					make_gl_program(ShaderProgram *p);
void				set_region_immediate(int x1, int x2, int y1, int y2);

//immediate drawing functions
void				draw_line(float x1, float y1, float x2, float y2, int color);
void				draw_line_i(int x1, int y1, int x2, int y2, int color);
void				draw_rectangle(float x1, float x2, float y1, float y2, int color);
void				draw_rectangle_i(int x1, int x2, int y1, int y2, int color);
void				draw_rectangle_hollow(float x1, float x2, float y1, float y2, int color);
void				draw_ellipse(float x1, float x2, float y1, float y2, int color);

int					toggle_sdftext();
long long			set_text_colors(long long colors);//0xBKBKBKBK_TXTXTXTX
float				print_line(float tab_origin, float x, float y, float zoom, const char *msg, int msg_length, int req_cols, int *ret_idx, int *ret_cols);
float				GUIPrint(float tab_origin, float x, float y, float zoom, const char *format, ...);
#endif

unsigned char*		stbi_load(const char *filename, int *x, int *y, int *channels_in_file, int desired_channels);//stb_image.h


#ifdef __cplusplus
}
#endif
#endif