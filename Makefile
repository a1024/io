EXTENSION =
LIBS = -lGL
ifeq ($(OS),Windows_NT)
	EXTENSION = .exe
else
	LIBS += -lX11
endif

build:
	g++ -no-pie -g io.c io_stub.cpp -o io_test$(EXTENSION) $(LIBS)
