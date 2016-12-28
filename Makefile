CHIP:=rpi
CC = gcc
CFLAGS = -g -O0
FILE := a.out
SIMPLE_EGL := simple-egl
SIMPLE_EGL_O := simple-egl.o
SIMPLE_EGL_C := simple-egl.c


$(SIMPLE_EGL): $(SIMPLE_EGL_O)

ifeq ("$(CHIP)", "rpi")
	echo "CHIP is rpi"
	$(CC) $(CFLAGS) -o $(SIMPLE_EGL) $(SIMPLE_EGL_O) -lGLESv2 -lEGL -lwayland-client -lwayland-egl
else
	echo "CHIP is other"
	$(CC) $(CFLAGS) -o $(SIMPLE_EGL) $(SIMPLE_EGL_O) /root/workstation/wayland/install/lib/libwayland-client.so /root/workstation/wayland/install/lib/libMali.so
endif

$(SIMPLE_EGL_O): $(SIMPLE_EGL_C)
	$(CC) $(CFLAGS) -DENABLE_EGL -c $(SIMPLE_EGL_C)

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
