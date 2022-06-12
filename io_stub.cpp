#include"io.h"
#ifdef __linux__
#define GL_GLEXT_PROTOTYPES
#endif
#include<Windows.h>
#include<GL/gl.h>
#include<stdio.h>
int io_init(int argc, char **argv)//return false to abort
{
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
	return true;
}
void io_timer()
{
}
void io_render()
{
	prof_add("entry");
	static float level=0, delta=0.01f;
	glClearColor(level, level, level, 1);
	level+=delta;
	if(level>1)
		level=1, delta=-0.01f;
	else if(level<0)
		level=0, delta=0.01f;
	glClear(GL_COLOR_BUFFER_BIT);

	if(!h)
		return;

	for(int k=0;k<100;++k)
		draw_ellipse((float)(rand()%w), (float)(rand()%w), (float)(rand()%h), (float)(rand()%h), 0xFF000000|rand()<<15|rand());
		//draw_line_i(rand()%w, rand()%h, rand()%w, rand()%h, 0xFF000000|rand()<<15|rand());
	prof_add("draw");
}
int io_quit_request()//return 1 to exit
{
	return true;
}
void io_cleanup()//cleanup
{
}