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


//	#define		NO_3D
//	#define		NO_GL_CHECK


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
extern float		tdx, tdy;//non-tab character dimensions at 1x zoom
extern short		tab_count;//default is 8 characters
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
#ifdef _MSC_VER
#pragma warning(push)
#pragma warning(disable:4200)//no default-constructor for struct with zero-length array
#endif
typedef struct ArrayHeaderStruct
{
	size_t count, esize, cap;//cap is in bytes
	void (*destructor)(void*);
	unsigned char data[];
} ArrayHeader, *ArrayHandle;
#ifdef _MSC_VER
#pragma warning(pop)
#endif
ArrayHandle		array_construct(const void *src, size_t esize, size_t count, size_t rep, size_t pad, void (*destructor)(void*));
ArrayHandle		array_copy(ArrayHandle *arr);//shallow
void			array_free(ArrayHandle *arr);
void			array_clear(ArrayHandle *arr);//keeps allocation
void			array_fit(ArrayHandle *arr, size_t pad);

void*			array_insert(ArrayHandle *arr, size_t idx, void *data, size_t count, size_t rep, size_t pad);//cannot be nullptr

size_t			array_size(ArrayHandle const *arr);
void*			array_at(ArrayHandle *arr, size_t idx);
const void*		array_at_const(ArrayHandle const *arr, int idx);
void*			array_back(ArrayHandle *arr);
const void*		array_back_const(ArrayHandle const *arr);

#define			ARRAY_ALLOC(ELEM_TYPE, ARR, COUNT, PAD, DESTRUCTOR)		ARR=array_construct(0, sizeof(ELEM_TYPE), COUNT, 1, PAD, DESTRUCTOR)
#define			ARRAY_APPEND(ARR, DATA, COUNT, REP, PAD)				array_insert(&(ARR), (ARR)->count, DATA, COUNT, REP, PAD)
#define			ARRAY_I(ARR, IDX)		*(int*)array_at(&ARR, IDX)
#define			ARRAY_U(ARR, IDX)		*(unsigned*)array_at(&ARR, IDX)
#define			ARRAY_F(ARR, IDX)		*(double*)array_at(&ARR, IDX)


//null terminated array
#define			ESTR_ALLOC(TYPE, STR, LEN)				STR=array_construct(0, sizeof(TYPE), LEN+1, 1, 0, 0)
#define			ESTR_COPY(TYPE, STR, SRC, LEN, REP)		STR=array_construct(SRC, sizeof(TYPE), LEN, REP, 1, 0)
#define			STR_APPEND(STR, SRC, LEN, REP)			array_insert(&(STR), (STR)->count, SRC, LEN, REP, 1)
#define			STR_FIT(STR)							array_fit(&STR, 1)
#define			ESTR_AT(TYPE, STR, IDX)					*(TYPE*)array_at(&(STR), IDX)

#define			STR_ALLOC(STR, LEN)				ESTR_ALLOC(char, STR, LEN)
#define			STR_COPY(STR, SRC, LEN, REP)	ESTR_COPY(char, STR, SRC, LEN, REP)
#define			STR_AT(STR, IDX)				ESTR_AT(char, STR, IDX)

#define			WSTR_ALLOC(STR, LEN)			ESTR_ALLOC(wchar_t, STR, LEN)
#define			WSTR_COPY(STR, SRC, LEN, REP)	ESTR_COPY(wchar_t, STR, SRC, LEN, REP)
#define			WSTR_AT(STR, IDX)				ESTR_AT(wchar_t, STR, IDX)
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
IOKEY(0x01, 0x01, LBUTTON)		//inserted
IOKEY(0x02, 0x04, MBUTTON)		//inserted
IOKEY(0x03, 0x02, RBUTTON)		//inserted

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
IOKEY(0x42, 0x42, B)
IOKEY(0x43, 0x43, C)
IOKEY(0x44, 0x44, D)
IOKEY(0x45, 0x45, E)
IOKEY(0x46, 0x46, F)
IOKEY(0x47, 0x47, G)
IOKEY(0x48, 0x48, H)
IOKEY(0x49, 0x49, I)
IOKEY(0x4A, 0x4A, J)
IOKEY(0x4B, 0x4B, K)
IOKEY(0x4C, 0x4C, L)
IOKEY(0x4D, 0x4D, M)
IOKEY(0x4E, 0x4E, N)
IOKEY(0x4F, 0x4F, O)
IOKEY(0x50, 0x50, P)
IOKEY(0x51, 0x51, Q)
IOKEY(0x52, 0x52, R)
IOKEY(0x53, 0x53, S)
IOKEY(0x54, 0x54, T)
IOKEY(0x55, 0x55, U)
IOKEY(0x56, 0x56, V)
IOKEY(0x57, 0x57, W)
IOKEY(0x58, 0x58, X)
IOKEY(0x59, 0x59, Y)
IOKEY(0x5A, 0x5A, Z)


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
#ifdef NO_3D
#define		SHADER_LIST		SHADER(2D) SHADER(texture) SHADER(text) SHADER(sdftext)
#else
#define		SHADER_LIST		SHADER(2D) SHADER(texture) SHADER(text) SHADER(sdftext) SHADER(3D) SHADER(L3D)
#endif

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
//#ifndef NO_3D
//		"    gl_FragDepth=0.;\n"
//#endif
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
//#ifndef NO_3D
//		"    gl_FragDepth=0.;\n"
//#endif
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
//#ifndef NO_3D
//		"    gl_FragDepth=0.;\n"
//#endif
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
//#ifndef NO_3D
//		"    gl_FragDepth=0.;\n"
//#endif
		"}";

//shader_3D
#define		ATTR_3D			ATTR(3D, vertex) ATTR(3D, texcoord)
#define		UNIF_3D			UNIF(3D, matrix) UNIF(3D, texture)
const char
	src_vert_3D[]=
		"#version 120\n"
		"uniform mat4 u_matrix;\n"			//attributes: a_vertex, a_texcoord
		"attribute vec3 a_vertex;\n"
		"attribute vec2 a_texcoord;\n"
		"varying vec2 v_texcoord;\n"
		"varying vec4 v_glposition;\n"
		"void main()\n"
		"{\n"
		"    gl_Position=u_matrix*vec4(a_vertex, 1.);\n"
		"    v_glposition=gl_Position;\n"
		"    gl_Position.z=0.;\n"
		"    v_texcoord=a_texcoord;\n"
		"}",
	src_frag_3D[]=
		"#version 120\n"
		"varying vec2 v_texcoord;\n"		//uniforms: u_matrix, u_texture
		"varying vec4 v_glposition;\n"
		"uniform sampler2D u_texture;\n"//use 1x1 texture for solid color
		"void main()\n"
		"{\n"
		"	 gl_FragColor=texture2D(u_texture, v_texcoord);\n"//alpha is in the texture
		"    gl_FragDepth=(-(1000.+0.1)*(-v_glposition.w)-2.*1000.*0.1)/((1000.-0.1)*v_glposition.w);\n"//USE mProj.znear=0.1f, mProj.zfar=1000.f
		"}";

//shader_L3D
#define		ATTR_L3D		ATTR(L3D, vertex) ATTR(L3D, normal) ATTR(L3D, texcoord)
#define		UNIF_L3D		UNIF(L3D, matVP_Model) UNIF(L3D, matNormal) UNIF(L3D, texture) UNIF(L3D, sceneInfo)
const char
	src_vert_L3D[]=
		"#version 120\n"
		"uniform mat4 u_matVP_Model[2];\n"	//attributes: a_vertex, a_normal, a_texcoord
		"uniform mat3 u_matNormal;\n"
		"attribute vec3 a_vertex;\n"
		"attribute vec3 a_normal;\n"
		"attribute vec2 a_texcoord;\n"
		"varying vec3 v_fragpos;\n"
		"varying vec3 v_normal;\n"
		"varying vec2 v_texcoord;\n"
		"varying vec4 v_glposition;\n"
		"void main()\n"
		"{\n"
		"    vec4 fullpos=vec4(a_vertex, 1.);\n"
		"    gl_Position=u_matVP_Model[0]*fullpos;\n"
		"    v_glposition=gl_Position;\n"
		"    gl_Position.z=0.;\n"
		"    v_fragpos=vec3(u_matVP_Model[1]*fullpos);\n"
		"    v_normal=u_matNormal*a_normal;\n"
		"    v_texcoord=a_texcoord;\n"
		"}",
	src_frag_L3D[]=
		"#version 120\n"
		"varying vec3 v_fragpos;\n"		//uniforms: u_matVP_Model, u_matNormal,
		"varying vec3 v_normal;\n"		//	u_texture, u_sceneInfo
		"varying vec4 v_glposition;\n"
		"varying vec2 v_texcoord;\n"
		"uniform sampler2D u_texture;\n"//use 1x1 texture for solid color
		"uniform vec3 u_sceneInfo[3];\n"
		"void main()\n"
		"{\n"
		"    vec3 lightPos=u_sceneInfo[0], lightColor=u_sceneInfo[1], viewPos=u_sceneInfo[1];\n"
		"	 vec4 objectColor=texture2D(u_texture, v_texcoord);\n"//alpha is in the texture

		"    vec3 normal=normalize(v_normal);\n"
		"    vec3 lightdir=normalize(lightPos-v_fragpos);\n"
				
		"    float specularstrength=0.5;\n"
		"    vec3 viewdir=normalize(viewPos-v_fragpos), reflectdir=reflect(-lightdir, normal);\n"
		"    vec3 specular=specularstrength*lightColor*pow(max(dot(viewdir, reflectdir), 0.), 32);\n"

		"    vec3 diffuse=max(dot(normal, lightdir), 0.)*lightColor;\n"
		"    gl_FragColor=vec4((0.1*lightColor+diffuse+specular)*objectColor.rgb, objectColor.a);\n"

		"    gl_FragDepth=(-(1000.+0.1)*(-v_glposition.w)-2.*1000.*0.1)/((1000.-0.1)*v_glposition.w);\n"//USE mProj.znear=0.1f, mProj.zfar=1000.f
		"}";
#endif


//API
double		time_ms();
void		set_window_title(const char *format, ...);
void		set_mouse(int x, int y);//client coordinates
void		get_mouse(int *px, int *py);//client coordinates
void		show_mouse(int show);

typedef enum MessageBoxTypeEnum
{
	MBOX_OK,
	MBOX_OKCANCEL,
	MBOX_YESNOCANCEL,
} MessageBoxType;
int			messagebox(MessageBoxType type, const char *title, const char *format, ...);//returns index of pressed button

typedef struct FilterStruct
{
	const char *comment, *ext;
} Filter;
ArrayHandle	dialog_open_folder(int multiple);//utf-8, free array of strings after use
ArrayHandle	dialog_open_file(Filter *filters, int nfilters, int multiple);//utf-8, free array of strings after use
const char*	dialog_save_file(Filter *filters, int nfilters, const char *initialname);//utf-8, free string after use
#define		FREE_ARRAY_OF_POINTERS(ARR, TEMP_2I)\
	do\
	{\
		(TEMP_2I)[1]=array_size(&(ARR));\
		for((TEMP_2I)[0]=0;(TEMP_2I)[0]<(TEMP_2I)[1];++(TEMP_2I)[0])\
			free(*(void**)array_at(&(ARR), (TEMP_2I)[0]));\
		array_free(&(ARR));\
	}while(0)
//void		free_array_of_pointers(ArrayHandle *a);//C has no destructors, but this is not part of the array API

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
extern void			(__stdcall *glUniform4fv)(int location, int count, float *value);
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
void				send_texture_pot(unsigned gl_texture, int *rgba, int txw, int txh);
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

void				display_texture_i(int x1, int x2, int y1, int y2, int *rgb, int txw, int txh, float alpha);
#endif

unsigned char*		stbi_load(const char *filename, int *x, int *y, int *channels_in_file, int desired_channels);//stb_image.h


//3D
#ifndef NO_3D
//DO NOT NEST THE MACROS LIKE f(g(x), h(y))
#define		vec2_copy(DST, SRC)		(DST)[0]=(SRC)[0], (DST)[1]=(SRC)[1]
#define		vec2_add(DST, A, B)		(DST)[0]=(A)[0]+(B)[0], (DST)[1]=(A)[1]+(B)[1]
#define		vec2_sub(DST, A, B)		(DST)[0]=(A)[0]-(B)[0], (DST)[1]=(A)[1]-(B)[1]
#define		vec2_add1(DST, V, S)	(DST)[0]=(V)[0]+(S), (DST)[1]=(V)[1]+(S)
#define		vec2_sub1(DST, V, S)	(DST)[0]=(V)[0]-(S), (DST)[1]=(V)[1]-(S)
#define		vec2_mul1(DST, V, S)	(DST)[0]=(V)[0]*(S), (DST)[1]=(V)[1]*(S)
#define		vec2_div1(DST, V, S)	(DST)[0]=(V)[0]/(S), (DST)[1]=(V)[1]/(S)
#define		vec2_dot(A, B)			((A)[0]*(B)[0]+(A)[1]*(B)[1])
#define		vec2_cross(DST, A, B)	((A)[0]*(B)[1]-(A)[1]*(B)[0])
#define		vec2_abs(A)				sqrtf(vec2_dot(A, A))
#define		vec2_abs2(A)			vec2_dot(A, A)
#define		vec2_arg(A)				atan((A)[1]/(A)[0])
#define		vec2_arg2(A)			atan2((A)[1], (A)[0])
#define		vec2_eq(A, B)			((A)[0]==(B)[0]&&(A)[1]==(B)[1])
#define		vec2_ne(A, B)			((A)[0]!=(B)[0]||(A)[1]!=(B)[1])
#define		vec2_neg(DST, A)		(DST)[0]=-(A)[0], (DST)[1]=-(A)[1]

#define		mat2_mul_vec2(DST, M2, V2)	(DST)[0]=(M2)[0]*(V2)[0]+(M2)[1]*(V2)[1], (DST)[1]=(M2)[2]*(V2)[0]+(M2)[3]*(V2)[1]

#define		vec3_copy(DST, SRC)		(DST)[0]=(SRC)[0], (DST)[1]=(SRC)[1], (DST)[2]=(SRC)[2]
#define		vec3_set1(V3, GAIN)		(V3)[0]=(V3)[1]=(V3)[2]=GAIN
#define		vec3_setp(V3, POINTER)	(V3)[0]=(POINTER)[0], (V3)[1]=(POINTER)[1], (V3)[2]=(POINTER)[2]
#define		vec3_seti(V3, X, Y, Z)	(V3)[0]=X, (V3)[1]=Y, (V3)[2]=Z
#define		vec3_add(DST, A, B)		(DST)[0]=(A)[0]+(B)[0], (DST)[1]=(A)[1]+(B)[1], (DST)[2]=(A)[2]+(B)[2]
#define		vec3_sub(DST, A, B)		(DST)[0]=(A)[0]-(B)[0], (DST)[1]=(A)[1]-(B)[1], (DST)[2]=(A)[2]-(B)[2]
#define		vec3_add1(DST, V, S)	(DST)[0]=(V)[0]+(S), (DST)[1]=(V)[1]+(S), (DST)[2]=(V)[2]+(S)
#define		vec3_sub1(DST, V, S)	(DST)[0]=(V)[0]-(S), (DST)[1]=(V)[1]-(S), (DST)[2]=(V)[2]-(S)
#define		vec3_mul1(DST, V, S)	(DST)[0]=(V)[0]*(S), (DST)[1]=(V)[1]*(S), (DST)[2]=(V)[2]*(S)
#define		vec3_div1(DST, V, S)	(DST)[0]=(V)[0]/(S), (DST)[1]=(V)[1]/(S), (DST)[2]=(V)[2]/(S)
#define		vec3_dot(A, B)			((A)[0]*(B)[0]+(A)[1]*(B)[1]+(A)[2]*(B)[2])
#define		vec3_cross(DST, A, B)\
	(DST)[0]=(A)[1]*(B)[2]-(A)[2]*(B)[1],\
	(DST)[1]=(A)[2]*(B)[0]-(A)[0]*(B)[2],\
	(DST)[2]=(A)[0]*(B)[1]-(A)[1]*(B)[0]
#define		vec3_triple_product(DST, A, B, C, TEMP_F1, TEMP_F2)\
	TEMP_F1=vec3_dot(A, C), TEMP_F2=vec3_dot(B, C), (DST)[0]=TEMP_F1*(B)[0]-TEMP_F2*(C)[0], (DST)[1]=TEMP_F1*(B)[1]-TEMP_F2*(C)[1], (DST)[2]=TEMP_F1*(B)[2]-TEMP_F2*(C)[2]
#define		vec3_abs(A)				sqrtf(vec3_dot(A, A))
#define		vec3_abs2(A)			vec3_dot(A, A)
#define		vec3_theta(A)			atan((A)[2]/sqrtf((A)[0]*(A)[0]+(A)[1]*(A)[1]))
#define		vec3_phi(A)				atan((A)[1]/(A)[0])
#define		vec3_phi2(A)			atan2((A)[1], (A)[0])
#define		vec3_isnan(A)			((A)[0]!=(A)[0]||(A)[1]!=(A)[1]||(A)[2]!=(A)[2])
#define		vec3_isnan_or_inf(A)	(vec3_isnan(A)||fabsf((A)[0])==infinity||fabsf((A)[1])==infinity||fabsf((A)[2])==infinity)
#define		vec3_eq(A, B)			((A)[0]==(B)[0]&&(A)[1]==(B)[1]&&(A)[2]==(B)[2])
#define		vec3_ne(A, B)			((A)[0]!=(B)[0]||(A)[1]!=(B)[1]||(A)[2]!=(B)[2])
#define		vec3_neg(DST, A)		(DST)[0]=-(A)[0], (DST)[1]=-(A)[1], (DST)[2]=-(A)[2]
#define		vec3_normalize(DST, A, TEMP_F)		TEMP_F=1/vec3_abs(A), vec3_div1(DST, A, TEMP_F)
#define		vec3_mix(DST, A, B, X)\
	(DST)[0]=(A)[0]+((B)[0]-(A)[0])*(X),\
	(DST)[1]=(A)[1]+((B)[1]-(A)[1])*(X),\
	(DST)[2]=(A)[2]+((B)[2]-(A)[2])*(X)

//column-major
#define		mat3_diag(MAT3, GAIN)	memset(MAT3, 0, 9*sizeof(float)), (MAT)[0]=(MAT)[1]=(MAT)[2]=GAIN
//#define	mat3_diag(MAT3, GAIN)	(MAT3)[0]=GAIN, (MAT3)[1]=0, (MAT3)[2]=0, (MAT3)[3]=0, (MAT3)[4]=GAIN, (MAT3)[5]=0, (MAT3)[6]=0, (MAT3)[7]=0, (MAT3)[8]=GAIN

#define		vec4_copy(DST, SRC)		(DST)[0]=(SRC)[0], (DST)[1]=(SRC)[1], (DST)[2]=(SRC)[2], (DST)[3]=(SRC)[3]
#define		vec4_dot(DST, A, B, TEMP_V1, TEMP_V2)		TEMP_V1=_mm_loadu_ps(A), TEMP_V1=_mm_mul_ps(TEMP_V1, _mm_loadu_ps(B)), TEMP_V1=_mm_hadd_ps(TEMP_V1, TEMP_V1), TEMP_V1=_mm_hadd_ps(TEMP_V1, TEMP_V1), _mm_store_ss(DST, TEMP_V1)
#define		vec4_add(DST, A, B)		(DST)[0]=(A)[0]+(B)[0], (DST)[1]=(A)[1]+(B)[1], (DST)[2]=(A)[2]+(B)[2], (DST)[3]=(A)[3]+(B)[3]
#define		vec4_sub(DST, A, B)		(DST)[0]=(A)[0]-(B)[0], (DST)[1]=(A)[1]-(B)[1], (DST)[2]=(A)[2]-(B)[2], (DST)[3]=(A)[3]-(B)[3]
#define		vec4_mul1(DST, V, S)	(DST)[0]=(V)[0]*(S), (DST)[1]=(V)[1]*(S), (DST)[2]=(V)[2]*(S), (DST)[3]=(V)[3]*(S)
//#define		vec4_add(DST, A, B)		{_mm_storeu_ps(DST, _mm_add_ps(_mm_loadu_ps(A), _mm_loadu_ps(B)));}
//#define		vec4_sub(DST, A, B)		{_mm_storeu_ps(DST, _mm_sub_ps(_mm_loadu_ps(A), _mm_loadu_ps(B)));}
//#define		vec4_mul1(DST, A, S)	{_mm_storeu_ps(DST, _mm_sub_ps(_mm_loadu_ps(A), _mm_set1_ps(S)));}

//column-major
#define		mat4_copy(DST, SRC)		memcpy(DST, SRC, 16*sizeof(float));
#define		mat4_identity(M4, GAIN)	memset(M4, 0, 16*sizeof(float)), (M4)[0]=(M4)[5]=(M4)[10]=(M4)[15]=GAIN
#define		mat4_data(M4, X, Y)		(M4)[(X)<<2|(Y)]
#define		mat4_mat3(DST, M4)\
	(DST)[0]=(M4)[0], (DST)[1]=(M4)[1], (DST)[2]=(M4)[2],\
	(DST)[3]=(M4)[4], (DST)[4]=(M4)[5], (DST)[5]=(M4)[6],\
	(DST)[6]=(M4)[8], (DST)[7]=(M4)[9], (DST)[8]=(M4)[10],
#define		mat4_transpose(DST, M4, TEMP_8V)\
	(TEMP_8V)[0]=_mm_loadu_ps(M4),\
	(TEMP_8V)[1]=_mm_loadu_ps((M4)+4),\
	(TEMP_8V)[2]=_mm_loadu_ps((M4)+8),\
	(TEMP_8V)[3]=_mm_loadu_ps((M4)+12),\
	(TEMP_8V)[4]=_mm_unpacklo_ps((TEMP_8V)[0], (TEMP_8V)[1]),\
	(TEMP_8V)[5]=_mm_unpacklo_ps((TEMP_8V)[2], (TEMP_8V)[3]),\
	(TEMP_8V)[6]=_mm_unpackhi_ps((TEMP_8V)[0], (TEMP_8V)[1]),\
	(TEMP_8V)[7]=_mm_unpackhi_ps((TEMP_8V)[2], (TEMP_8V)[3]),\
	_mm_storeu_ps(DST, _mm_movelh_ps((TEMP_8V)[4], (TEMP_8V)[5])),\
	_mm_storeu_ps((DST)+4, _mm_movehl_ps((TEMP_8V)[5], (TEMP_8V)[4])),\
	_mm_storeu_ps((DST)+8, _mm_movelh_ps((TEMP_8V)[6], (TEMP_8V)[7])),\
	_mm_storeu_ps((DST)+12, _mm_movehl_ps((TEMP_8V)[7], (TEMP_8V)[6]))
#define		mat4_mul_vec4(DST, M4, V4, TEMP_V)\
	TEMP_V=_mm_mul_ps(_mm_loadu_ps(M4), _mm_set1_ps((V4)[0])),\
	TEMP_V=_mm_add_ps(TEMP_V, _mm_mul_ps(_mm_loadu_ps(M4+4), _mm_set1_ps((V4)[1]))),\
	TEMP_V=_mm_add_ps(TEMP_V, _mm_mul_ps(_mm_loadu_ps(M4+8), _mm_set1_ps((V4)[2]))),\
	TEMP_V=_mm_add_ps(TEMP_V, _mm_mul_ps(_mm_loadu_ps(M4+12), _mm_set1_ps((V4)[3]))),\
	_mm_storeu_ps(DST, TEMP_V)
#define		mat4_mul_mat4(DST_NEW, M4A, M4B, TEMP_V)\
	mat4_mul_vec4(DST_NEW,		M4A, M4B,		TEMP_V),\
	mat4_mul_vec4(DST_NEW+4,	M4A, M4B+4,		TEMP_V),\
	mat4_mul_vec4(DST_NEW+8,	M4A, M4B+8,		TEMP_V),\
	mat4_mul_vec4(DST_NEW+12,	M4A, M4B+12,	TEMP_V)
#define		mat4_translate(M4, V3, TEMP_V)\
	TEMP_V=_mm_mul_ps(_mm_loadu_ps(M4), _mm_set1_ps((V3)[0])),\
	TEMP_V=_mm_add_ps(TEMP_V, _mm_mul_ps(_mm_loadu_ps(M4+4), _mm_set1_ps((V3)[1]))),\
	TEMP_V=_mm_add_ps(TEMP_V, _mm_mul_ps(_mm_loadu_ps(M4+8), _mm_set1_ps((V3)[2]))),\
	TEMP_V=_mm_add_ps(TEMP_V, _mm_loadu_ps(M4+12)),\
	_mm_storeu_ps(M4+12, TEMP_V)
#define		mat4_rotate(DST_NEW, M4, ANGLE, DIR, TEMP_VEC2, TEMP_VEC3A, TEMP_VEC3B)\
	vec3_normalize(TEMP_VEC3A, DIR, (TEMP_VEC2)[0]),\
	(TEMP_VEC2)[0]=cosf(ANGLE),\
	(TEMP_VEC2)[1]=1-(TEMP_VEC2)[0],\
	vec3_mul1(TEMP_VEC3B, TEMP_VEC3A, (TEMP_VEC2)[1]),\
	(TEMP_VEC2)[1]=sinf(ANGLE),\
	(DST_NEW)[0]=(TEMP_VEC3B)[0]*(TEMP_VEC3A)[0]+(TEMP_VEC2)[0],\
	(DST_NEW)[1]=(TEMP_VEC3B)[0]*(TEMP_VEC3A)[1]+(TEMP_VEC2)[1]*(TEMP_VEC3A)[2],\
	(DST_NEW)[2]=(TEMP_VEC3B)[0]*(TEMP_VEC3A)[2]-(TEMP_VEC2)[1]*(TEMP_VEC3A)[1],\
	(DST_NEW)[3]=0,\
	\
	(DST_NEW)[4]=(TEMP_VEC3B)[1]*(TEMP_VEC3A)[0]-(TEMP_VEC2)[1]*(TEMP_VEC3A)[2],\
	(DST_NEW)[5]=(TEMP_VEC3B)[1]*(TEMP_VEC3A)[1]+(TEMP_VEC2)[0],\
	(DST_NEW)[6]=(TEMP_VEC3B)[1]*(TEMP_VEC3A)[2]+(TEMP_VEC2)[1]*(TEMP_VEC3A)[0],\
	(DST_NEW)[7]=0,\
	\
	(DST_NEW)[8]=(TEMP_VEC3B)[2]*(TEMP_VEC3A)[0]+(TEMP_VEC2)[1]*(TEMP_VEC3A)[1],\
	(DST_NEW)[9]=(TEMP_VEC3B)[2]*(TEMP_VEC3A)[1]-(TEMP_VEC2)[1]*(TEMP_VEC3A)[0],\
	(DST_NEW)[10]=(TEMP_VEC3B)[2]*(TEMP_VEC3A)[2]+(TEMP_VEC2)[0],\
	(DST_NEW)[11]=0,\
	\
	(DST_NEW)[12]=0,\
	(DST_NEW)[13]=0,\
	(DST_NEW)[14]=0,\
	(DST_NEW)[15]=1
#define		mat4_scale(M4, AMMOUNT, TEMP_V0)\
		_mm_storeu_ps(M4, _mm_mul_ps(_mm_loadu_ps(M4), _mm_set1_ps((AMMOUNT)[0]))),\
		_mm_storeu_ps(M4+4, _mm_mul_ps(_mm_loadu_ps(M4+4), _mm_set1_ps((AMMOUNT)[1]))),\
		_mm_storeu_ps(M4+8, _mm_mul_ps(_mm_loadu_ps(M4+8), _mm_set1_ps((AMMOUNT)[2]))),\

void	mat4_lookAt(float *dst, const float *cam, const float *center, const float *up);
void	mat4_FPSView(float *dst, const float *campos, float yaw, float pitch);
void	mat4_perspective(float *dst, float tanfov, float w_by_h, float znear, float zfar);
void	mat4_normalmat3(float *dst, float *m4);//inverse transpose of top left 3x3 submatrix

typedef struct CameraStruct
{
	float
		x, y, z,//position
		ax, ay,//yaw/phi, pitch/theta
		tanfov,
		move_speed, turn_speed,
		cax, sax, cay, say;
} Camera;
#define		cam_copy(DSTCAM, SRCCAM)		memcpy(&(DSTCAM), &(SRCCAM), sizeof(DSTCAM))
#define		cam_moveForward(CAM, SPEED)		(CAM).x+=(SPEED)*(CAM).cax*(CAM).cay, (CAM).y+=(SPEED)*(CAM).sax*(CAM).cay, (CAM).z+=(SPEED)*(CAM).say
#define		cam_moveBack(CAM, SPEED)		(CAM).x-=(SPEED)*(CAM).cax*(CAM).cay, (CAM).y-=(SPEED)*(CAM).sax*(CAM).cay, (CAM).z-=(SPEED)*(CAM).say
#define		cam_moveLeft(CAM, SPEED)		(CAM).x-=(SPEED)*(CAM).sax, (CAM).y+=(SPEED)*(CAM).cax
#define		cam_moveRight(CAM, SPEED)		(CAM).x+=(SPEED)*(CAM).sax, (CAM).y-=(SPEED)*(CAM).cax
#define		cam_moveUp(CAM, SPEED)			(CAM).z+=SPEED
#define		cam_moveDown(CAM, SPEED)		(CAM).z-=SPEED
#define			cam_update_ax(CAM)			(CAM).ax=modulof((CAM).ax, _2pi), (CAM).cax=cosf((CAM).ax), (CAM).sax=sinf((CAM).ax)
#define			cam_update_ay(CAM)			(CAM).ay=modulof((CAM).ay, _2pi), (CAM).cay=cosf((CAM).ay), (CAM).say=sinf((CAM).ay)
#define		cam_turnUp(CAM, SPEED)			(CAM).ay+=(SPEED)*(CAM).turn_speed, cam_update_ay(CAM)
#define		cam_turnDown(CAM, SPEED)		(CAM).ay-=(SPEED)*(CAM).turn_speed, cam_update_ay(CAM)
#define		cam_turnLeft(CAM, SPEED)		(CAM).ax+=(SPEED)*(CAM).turn_speed, cam_update_ax(CAM)
#define		cam_turnRight(CAM, SPEED)		(CAM).ax-=(SPEED)*(CAM).turn_speed, cam_update_ax(CAM)
#define		cam_turnMouse(CAM, DX, DY, SENSITIVITY)\
	(CAM).ax-=(SENSITIVITY)*(CAM).turn_speed*(DX), cam_update_ax(CAM),\
	(CAM).ay-=(SENSITIVITY)*(CAM).turn_speed*(DY), cam_update_ay(CAM)
#define		cam_zoomIn(CAM, RATIO)			(CAM).tanfov/=RATIO, (CAM).turn_speed=(CAM).tanfov>1?1:(CAM).tanfov
#define		cam_zoomOut(CAM, RATIO)			(CAM).tanfov*=RATIO, (CAM).turn_speed=(CAM).tanfov>1?1:(CAM).tanfov
#define		cam_accelerate(GAIN)			(CAM).move_speed*=GAIN

#define		cam_relworld2cam(CAM, DISP, DST_CP)\
	(DST_CP)[2]=(DISP)[0]*(CAM).cax+(DISP)[1]*(CAM).sax,\
	(DST_CP)[0]=(DISP)[0]*(CAM).sax-(DISP)[1]*(CAM).cax,\
	(DST_CP)[1]=(DST_CP)[2]*(CAM).say-(DISP)[2]*(CAM).cay,\
	(DST_CP)[2]=(DST_CP)[2]*(CAM).cay+(DISP)[2]*(CAM).say
#define		cam_world2cam(CAM, P, DST_CP, TEMP_3F)\
	vec3_sub(TEMP_3F, P, &(CAM).x),\
	cam_relworld2cam(CAM, TEMP_3F, DST_CP)
#define		cam_cam2screen(CAM, CP, DST_S, X0, Y0)\
	(DST_S)[1]=(X0)/((CP)[2]*(CAM).tanfov),\
	(DST_S)[0]=(X0)+(CP)[0]*(DST_S)[1],\
	(DST_S)[1]=(Y0)+(CP)[1]*(DST_S)[1]

typedef struct GPUModelStruct
{
	unsigned VBO, EBO, txid;
	int n_elements, stride, vertices_start, normals_start, txcoord_start;
} GPUModel;
void gpubuf_send_VNT(GPUModel *dst, const float *VVVNNNTT, int n_floats, const int *indices, int n_ints);
void draw_L3D(Camera const *cam, GPUModel const *model, const float *modelpos, const float *lightpos, int lightcolor);

#endif//NO_3D


#ifdef __cplusplus
}
#endif
#endif
