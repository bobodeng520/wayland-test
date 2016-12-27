CHIP:=rpi
CC = gcc
CFLAGS = -g -O0
FILE := a.out
SIMPLE_EGL := simple-egl
SIMPLE_EGL_O := simple-egl.o
SIMPLE_EGL: simple-egl.o

ifeq ("$(CHIP)", "rpi")
	echo "CHIP is rpi"
	$(CC) $(CFLAGS) -o simple-egl simple-egl.o -lGLESv2 -lEGL -lwayland-client -lwayland-egl
else
	echo "CHIP is other"
	$(CC) $(CFLAGS) -o simple-egl simple-egl.o /root/workstation/wayland/install/lib/libwayland-client.so /root/workstation/wayland/install/lib/libMali.so
endif

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
