#include"io.h"
#ifdef _MSC_VER
#include<Windows.h>
#else
#define GL_GLEXT_PROTOTYPES
#endif
#include<GL/gl.h>
#include<stdio.h>
#include<stdlib.h>
int io_init(int argc, char **argv)//return false to abort
{
	set_window_title("IO Test");
	glClearColor(0, 0, 0, 1);
	return true;
}
void io_resize()
{
}
int io_mousemove()//return true to redraw
{
	printf("Move to (%d, %d)\n", mx, my);
	return true;
}
int io_mousewheel(int forward)
{
	return true;
}
int io_keydn(IOKey key, char c)
{
	switch(key)
	{
	case KEY_LBUTTON:
	case KEY_MBUTTON:
	case KEY_RBUTTON:
		printf("Click at (%d, %d)\n", mx, my);
		break;
	default:
		printf("%02X %02X=%c down\n", key, c, c);
		if(key=='A')
			timer_start();
		break;
	}
	return true;
}
int io_keyup(IOKey key, char c)
{
	switch(key)
	{
	case KEY_LBUTTON:
	case KEY_MBUTTON:
	case KEY_RBUTTON:
		printf("Declick at (%d, %d)\n", mx, my);
		break;
	case KEY_F4:
		prof_on=!prof_on;
		break;
	default:
		printf("%02X %02X=%c up\n", key, c, c);
		if(key=='A')
			timer_stop();
		break;
	}
	return false;
}
void io_timer()
{
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
	glClear(GL_COLOR_BUFFER_BIT);

	if(!h)
		return;

	for(int k=0;k<h;k+=2)
		draw_line(w>>1, k, w*3>>2, k, 0xFFFF00FF);
	draw_ellipse(0, w>>2, 0, h>>2, 0xFF0000FF);
	draw_ellipse(w>>2, w>>1, h>>2, h>>1, 0xFFFF0000);
	for(int k=0;k<1;++k)
	{
		int color=0xFF000000|rand()<<15|rand();
		draw_rectangle_hollow((float)(rand()%w), (float)(rand()%w), (float)(rand()%h), (float)(rand()%h), color);
		draw_ellipse((float)(rand()%w), (float)(rand()%w), (float)(rand()%h), (float)(rand()%h), color);
	}
	draw_line_i(rand()%w, rand()%h, rand()%w, rand()%h, 0xFF000000|rand()<<15|rand());
	prof_add("draw");
}
int io_quit_request()//return 1 to exit
{
	return true;
}
void io_cleanup()//cleanup
{
}