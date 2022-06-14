INCLUDE =
LIBS = -lGL
EXTENSION =
ifeq ($(OS),Windows_NT)
	EXTENSION = .exe
else
	INCLUDE += `pkg-config --cflags gtk+-3.0`
	LIBS += -lX11 `pkg-config --libs gtk+-3.0`
endif

build:
	g++ -no-pie -g $(INCLUDE) io.c io_stub.cpp -o io_test$(EXTENSION) $(LIBS)
