IO - A cross-platform window creation, event handling and immediate graphics library

Define the following callbacks for your application:

	int io_init(int argc, char **argv);
	void io_resize();
	int io_mousemove();
	int io_mousewheel(int forward);
	int io_keydn(IOKey key, char c);
	int io_keyup(IOKey key, char c);
	void io_timer();
	void io_render();
	int io_quit_request();
	void io_cleanup();

Features:
	Lightweight: one source and one header
	Works on Windows and Linux
	Easy to use modern OpenGL immediate drawing functions
	Easy to add new shaders

TODO:
	3D drawing functions
DONE:
	draw_ellipse()
	print_line()
