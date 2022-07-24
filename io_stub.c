#include"io.h"
#ifdef _MSC_VER
#include<Windows.h>//just to include gl.h
#else
#define GL_GLEXT_PROTOTYPES
#endif
#include<GL/gl.h>
#include<math.h>

float
	mouse_sensitivity=0.003f,
	key_turn_speed=0.03f;
Camera cam=
{
	10, 10, 10,
	225, 324.7356103172454f,
	1,
	0.04f, 2,
}, cam0;

//active keys turn on timer
#define ACTIVE_KEY_LIST\
	AK('W') AK('A') AK('S') AK('D') AK('T') AK('G')\
	AK(KEY_LEFT) AK(KEY_RIGHT) AK(KEY_UP) AK(KEY_DOWN)\
	AK(KEY_ENTER) AK(KEY_BKSP)
int active_keys_pressed=0;

//mouse
char drag=0;
int mx0=0, my0=0;

int io_init(int argc, char **argv)//return false to abort
{
	set_window_title("IO Test");
	glClearColor(1, 1, 1, 1);

	cam.ax*=torad;
	cam.ay*=torad;
	cam.turn_speed*=torad;
	cam_zoomIn(cam, 1);
	cam_turnMouse(cam, 0, 0, mouse_sensitivity);
	memcpy(&cam0, &cam, sizeof(cam));
}
void io_resize()
{
}
int io_mousemove()//return true to redraw
{
	if(drag)
	{
		int X0=w>>1, Y0=h>>1;
		cam_turnMouse(cam, mx-X0, my-Y0, mouse_sensitivity);
		set_mouse(X0, Y0);
		return !timer;
	}
	return 0;
}
int io_mousewheel(int forward)
{
	if(keyboard[KEY_SHIFT])//shift wheel		change cam speed
	{
			 if(forward>0)	cam.move_speed*=2;
		else				cam.move_speed*=0.5f;
	}
	else
	{
			 if(forward>0)	cam_zoomIn(cam, 1.1f);
		else				cam_zoomOut(cam, 1.1f);
	}
	return !timer;
}
static void count_active_keys(IOKey upkey)
{
	keyboard[upkey]=0;
	active_keys_pressed=0;
#define		AK(KEY)		active_keys_pressed+=keyboard[KEY];
	ACTIVE_KEY_LIST
#undef		AK
	if(!active_keys_pressed)
		timer_stop();
}
int io_keydn(IOKey key, char c)
{
	switch(key)
	{
	case 'S':
		if(keyboard[KEY_CTRL])
		{
		//	savedfile=dialog_save_file(file_filters, SIZEOF(file_filters), initialname);
			return 1;
		}
		break;
	}
	switch(key)
	{
	case KEY_LBUTTON:
	case KEY_ESC:
		show_mouse(drag);
		drag=!drag;
		if(drag)//enter mouse control
		{
			mx0=mx, my0=my;
			set_mouse(w>>1, h>>1);
		}
		else//leave mouse control
			set_mouse(mx0, my0);
		break;
	case KEY_MBUTTON:
	case KEY_RBUTTON:
		//printf("Click at (%d, %d)\n", mx, my);
		break;

#define		AK(KEY)		case KEY:
	ACTIVE_KEY_LIST
#undef		AK
		timer_start();
		break;

	case 'R':
		memcpy(&cam, &cam0, sizeof(cam));
		return 1;
	case 'E':
	//	wireframe=!wireframe;
		return 1;
	case 'O':
		//if(keyboard[KEY_CTRL])
		//{
		//	if(openfiles)
		//	{
		//		int temp1[2]={};
		//		FREE_ARRAY_OF_POINTERS(openfiles, temp1);
		//	}
		//	if(keyboard[KEY_SHIFT])
		//		openfiles=dialog_open_folder(1);
		//	else
		//		openfiles=dialog_open_file(file_filters, SIZEOF(file_filters), 1);
		//}
		return 1;
	case KEY_F4:
		prof_on=!prof_on;
		return 1;
	//default:
	//	printf("%02X %02X=%c down\n", key, c, c);
	//	if(key=='A')
	//		timer_start();
	//	break;
	}
	return 0;
}
int io_keyup(IOKey key, char c)
{
	switch(key)
	{
	//case KEY_LBUTTON:
	//case KEY_MBUTTON:
	//case KEY_RBUTTON:
	//	printf("Declick at (%d, %d)\n", mx, my);
	//	break;

#define		AK(KEY)		case KEY:
	ACTIVE_KEY_LIST
#undef		AK
		count_active_keys(key);
		break;

	//default:
	//	printf("%02X %02X=%c up\n", key, c, c);
	//	if(key=='A')
	//		timer_stop();
	//	break;
	}
	return 0;
}
void io_timer()
{
	float move_speed=keyboard[KEY_SHIFT]?10*cam.move_speed:cam.move_speed;
	if(keyboard['W'])		cam_moveForward(cam, move_speed);
	if(keyboard['A'])		cam_moveLeft(cam, move_speed);
	if(keyboard['S'])		cam_moveBack(cam, move_speed);
	if(keyboard['D'])		cam_moveRight(cam, move_speed);
	if(keyboard['T'])		cam_moveUp(cam, move_speed);
	if(keyboard['G'])		cam_moveDown(cam, move_speed);
	if(keyboard[KEY_UP])	cam_turnUp(cam, key_turn_speed);
	if(keyboard[KEY_DOWN])	cam_turnDown(cam, key_turn_speed);
	if(keyboard[KEY_LEFT])	cam_turnLeft(cam, key_turn_speed);
	if(keyboard[KEY_RIGHT])	cam_turnRight(cam, key_turn_speed);
	if(keyboard[KEY_ENTER])	cam_zoomIn(cam, 1.1f);
	if(keyboard[KEY_BKSP])	cam_zoomOut(cam, 1.1f);
}
static void draw_line_3d(float *p1, float *p2, int color)
{
	float cp[3], temp1[3], s1[2], s2[2];
	int X0=w>>1, Y0=h>>1;

	cam_world2cam(cam, p1, cp, temp1);
	if(cp[2]<0)
		return;
	cam_cam2screen(cam, cp, s1, X0, Y0);

	cam_world2cam(cam, p2, cp, temp1);
	if(cp[2]<0)
		return;
	cam_cam2screen(cam, cp, s2, X0, Y0);

	draw_line(s1[0], s1[1], s2[0], s2[1], color);
}
void io_render()
{
	prof_add("entry");
/*	static float level=0, delta=0.01f;
	glClearColor(level, level, level, 1);
	level+=delta;
	if(level>1)
		level=1, delta=-0.01f;
	else if(level<0)
		level=0, delta=0.01f;//*/
	glClear(GL_COLOR_BUFFER_BIT|GL_DEPTH_BUFFER_BIT);

	if(!h)
		return;

	float axes[]=
	{
		0, 0, 0,
		1, 0, 0,
		0, 1, 0,
		0, 0, 1,
	};
	draw_line_3d(axes, axes+1, 0xFF0000FF);
	draw_line_3d(axes, axes+2, 0xFF00FF00);
	draw_line_3d(axes, axes+3, 0xFFFF0000);
	//if(wireframe)
	//	glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);
	//draw_L3D(&cam, &gpu_teapot, modelPos, lightPos, 0x80C0FF);
	//if(wireframe)
	//	glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
	prof_add("model");

#ifdef DEBUG_TEAPOT
	int wireColor=0xFF80FF80;
	auto vert=cpu_teapot.VVVNNNTT;
	auto indices=cpu_teapot.indices;
	for(int kt=0;kt<cpu_teapot.n_elements;kt+=3)
	{
		float tr[]=
		{
			vert[indices[kt]<<3], vert[indices[kt]<<3|1], vert[indices[kt]<<3|2],
			vert[indices[kt+1]<<3], vert[indices[kt+1]<<3|1], vert[indices[kt+1]<<3|2],
			vert[indices[kt+2]<<3], vert[indices[kt+2]<<3|1], vert[indices[kt+2]<<3|2],
		};
		draw_line_3d(tr, tr+3, wireColor);
		draw_line_3d(tr+3, tr+6, wireColor);
		draw_line_3d(tr+6, tr, wireColor);
	}
#endif
	GUIPrint(0, 0, 0, 1, "p(%f, %f, %f) a(%f, %f) fov %f", cam.x, cam.y, cam.z, cam.ax, cam.ay, atan(cam.tanfov)*todeg*2);
	GUIPrint(0, 0, tdy, 1, "timer %d, rand %d", timer, rand());
//	float y=tdy*3;
//	if(openfiles)
//	{
//		GUIPrint(0, 0, y, 1, "Open files:");
//		y+=tdy;
//		for(int k=0, count=array_size(&openfiles);k<count;++k)
//		{
//			GUIPrint(0, 0, y, 1, "%d\t%s", k, *(char**)array_at(&openfiles, k));
//			y+=tdy;
//		}
//	}
//	if(savedfile)
//		GUIPrint(0, 0, y, 1, "Saved as: %s", savedfile);
#if 0
	for(int k=0;k<h;k+=2)
		draw_line((float)(w>>1), (float)k, (float)(w*3>>2), (float)k, 0xFFFF00FF);
	draw_ellipse(0, (float)(w>>2), 0, (float)(h>>2), 0xFF0000FF);
	draw_ellipse((float)(w>>2), (float)(w>>1), (float)(h>>2), (float)(h>>1), 0xFFFF0000);
	for(int k=0;k<1;++k)
	{
		int color=0xFF000000|rand()<<15|rand();
		draw_rectangle_hollow((float)(rand()%w), (float)(rand()%w), (float)(rand()%h), (float)(rand()%h), color);
		draw_ellipse((float)(rand()%w), (float)(rand()%w), (float)(rand()%h), (float)(rand()%h), color);
	}
	draw_line_i(rand()%w, rand()%h, rand()%w, rand()%h, 0xFF000000|rand()<<15|rand());
#endif
	prof_add("finish");
}
int io_quit_request()//return 1 to exit
{
	//int button_idx=messagebox(MBOX_OKCANCEL, "Are you sure?", "Quit application?");
	//return button_idx==0;

	return 1;
}
void io_cleanup()//cleanup
{
}