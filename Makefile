CHIP:=rpi#v40 use the real lib path
CC = gcc
CFLAGS = -g -O0
BUILD_FLAGS := -DENABLE_EGL

ifeq ("$(CHIP)", "rpi")
GLESV2_LIB := -lGLESv2
EGL_LIB := -lEGL
WAYLAND_CLIENT_LIB := -lwayland-client
WAYLAND_EGL_LIB := -lwayland-egl
else
GLESV2_LIB := /root/workstation/wayland/install/lib/libMali.so
EGL_LIB := /root/workstation/wayland/install/lib/libMali.so
WAYLAND_CLIENT_LIB := /root/workstation/wayland/install/lib/libwayland-client.so
WAYLAND_EGL_LIB := /root/workstation/wayland/install/lib/libMali.so
endif


SRCS = $(wildcard *.c)
BINS = $(patsubst %.c, %, $(SRCS))

.c:
	$(CC) $(CFLAGS) $(BUILD_FLAGS) -o $@ $< $(GLESV2_LIB) $(EGL_LIB) $(WAYLAND_CLIENT_LIB) $(WAYLAND_EGL_LIB)

all: $(BINS)

clean:
	rm -f $(BINS)
