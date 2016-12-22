CC = gcc
CFLAGS = -g -O0
FILE := a.out
SIMPLE_EGL := simple-egl
SIMPLE_EGL_O := simple-egl.o
SIMPLE_EGL: simple-egl.o
	$(CC) $(CFLAGS) -o simple-egl simple-egl.o -lEGL -lGLESv2 -lwayland-client

simple-egl.o: simple-egl.c
	$(CC) $(CFLAGS) -DENABLE_EGL -c simple-egl.c

clean:
ifeq ($(FILE), $(wildcard $(FILE)))
	rm $(FILE)
endif

ifeq ($(SIMPLE_EGL), $(wildcard $(SIMPLE_EGL)))
	rm $(SIMPLE_EGL)
endif

ifeq ($(SIMPLE_EGL_O), $(wildcard $(SIMPLE_EGL_O)))
	rm $(SIMPLE_EGL_O)
endif
