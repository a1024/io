INCLUDE =
LIBS =
EXTENSION =
ifeq ($(OS),Windows_NT)
	EXTENSION = .exe
	LIBS += -lopengl32 -lgdi32 -lole32 -lcomdlg32 -lcomctl32 -loleaut32 -luuid
else
	INCLUDE += `pkg-config --cflags gtk+-3.0`
	LIBS += -lGL -lX11 `pkg-config --libs gtk+-3.0`
endif

build-cpp:
	g++ -no-pie -g $(INCLUDE) io.c io_stub.cpp -o io_test$(EXTENSION) $(LIBS)

build-c:
	gcc -no-pie -g $(INCLUDE) io.c io_stub.c -o io_test$(EXTENSION) $(LIBS)
