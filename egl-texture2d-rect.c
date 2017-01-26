#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <wayland-client.h>
#include <wayland-server.h>
#include <wayland-client-protocol.h>
#include <wayland-egl.h>
#include <EGL/egl.h>
#include <GLES2/gl2.h>
#include "platform.h"
#include "weston-egl-ext.h"

#define WIDTH 200
#define HEIGHT 200
#define VIEWPORT_WIDTH 800
#define VIEWPORT_HEIGHT 800

//#define USE_PICTURE 0

struct wl_display *display = NULL;
struct wl_compositor *compositor = NULL;
struct wl_surface *surface;
struct wl_egl_window *egl_window;
struct wl_region *region;
struct wl_shell *shell;
struct wl_shell_surface *shell_surface;

EGLDisplay egl_display;
EGLConfig egl_conf;
EGLSurface egl_surface;
EGLContext egl_context;

GLuint rotation_uniform;
GLuint pos;
GLuint color;
GLuint model_uniform;
GLuint view_uniform;
GLuint projection_uniform;

GLint gvPositionHandle;
GLint gvTexCoordHandle;
GLint guTexSamplerHandle;
static GLuint TexName;
GLuint program;

static const char *vert_shader_text =
	"attribute vec4 vPosition;\n"
	"attribute vec4 aTexCoords;\n"
	"varying vec2 vTexCoords;\n"
	"uniform mat4 model;\n"
	"uniform mat4 view;\n"
	"uniform mat4 projection;\n"
	"void main() {\n"
	"	vTexCoords = aTexCoords.xy;\n"
	"	gl_Position = projection * view * model * vPosition;\n"
	"}\n";

static const char *frag_shader_text =
	"precision mediump float;\n"
	"uniform sampler2D uTexSampler;\n"
	"varying vec2 vTexCoords;\n"
	"void main() {\n"
	"	gl_FragColor = texture2D(uTexSampler, vTexCoords);\n"
	"}\n";

static checkGLError(const char *op) {
	GLint error;
	for (error = glGetError(); error; 
		error = glGetError()) {
		fprintf(stderr, "after %s glError (0x%x)\n", op, error);
	}
}

static void global_registry_handler(void *data, struct wl_registry *registry, 
uint32_t id, const char *interface, uint32_t version)
{
	printf("Got a registry event for %s id %d\n", interface, id);
	if (strcmp(interface, "wl_compositor") == 0) {
		compositor = wl_registry_bind(registry, id, &wl_compositor_interface, 1);
	} else if (strcmp(interface, "wl_shell") == 0) {
		shell = wl_registry_bind(registry, id, &wl_shell_interface, 1);
	}
}

static void global_registry_remover(void *data, struct wl_registry *registry, uint32_t id)
{
	printf("Got a registry losing event for %d\n", id);
}

static const struct wl_registry_listener registry_listener = {
	global_registry_handler,
	global_registry_remover
};

static void create_opaque_region() {
	region = wl_compositor_create_region(compositor);
	wl_region_add(region, 0, 0, VIEWPORT_WIDTH, VIEWPORT_HEIGHT);
	wl_surface_set_opaque_region(surface, region);
}

static void get_server_reference(void)
{
	display = wl_display_connect(NULL);
	if (display == NULL) {
		fprintf(stderr, "Can't connect to display\n");
	}
	printf("connect to display");

	struct wl_registry *registry = wl_display_get_registry(display);
	wl_registry_add_listener(registry, &registry_listener, NULL);

	wl_display_dispatch(display);
	wl_display_roundtrip(display);

	// hasn't found where to initialize compositor and shell
	if (compositor == NULL || shell == NULL) {
		fprintf(stderr, "Can't find compositor or shell\n");
		exit(1);
	} else {
		fprintf(stderr, "Found compositor and shell\n");
	}
}

static void init_egl() {
	EGLint major, minor, count, n, size;
	EGLConfig *configs;
	int i;
	EGLint config_attribs[] = {
		EGL_SURFACE_TYPE, EGL_WINDOW_BIT,
		EGL_RED_SIZE, 8,
		EGL_GREEN_SIZE, 8,
		EGL_BLUE_SIZE, 8,
		EGL_RENDERABLE_TYPE, EGL_OPENGL_ES2_BIT,
		EGL_NONE
	};

	static const EGLint context_attribs[] = {
		EGL_CONTEXT_CLIENT_VERSION, 2,
		EGL_NONE
	};

	//egl_display = eglGetDisplay((EGLNativeDisplayType) display);
	egl_display = weston_platform_get_egl_display(EGL_PLATFORM_WAYLAND_KHR, display, NULL);
	if (egl_display == EGL_NO_DISPLAY) {
		fprintf(stderr, "Can't create egl display\n");
		exit(1);
	} else {
		fprintf(stderr, "Created egl display\n");
	}

	if (eglInitialize(egl_display, &major, &minor) != EGL_TRUE) {
		fprintf(stderr, "Can't initialize egl display\n");
		exit(1);
	}
	printf("EGL major: %d, minor %d\n", major, minor);

	eglGetConfigs(egl_display, NULL, 0, &count);
	printf("EGL has %d configs\n", count);

	configs = calloc(count, sizeof(*configs));
	eglChooseConfig(egl_display, config_attribs, configs, count, &n);

	for (i = 0; i < n; i++) {
		eglGetConfigAttrib(egl_display, configs[i], EGL_BUFFER_SIZE, &size);
		printf("Buffer size for config %d is %d\n", i, size);
		eglGetConfigAttrib(egl_display, configs[i], EGL_RED_SIZE, &size);
		printf("Red size for config %d is %d\n", i, size);

		egl_conf = configs[i];
		break;
	}

	egl_context = eglCreateContext(egl_display, egl_conf, EGL_NO_CONTEXT, context_attribs);
}

static void create_window()
{
	egl_window = wl_egl_window_create(surface, VIEWPORT_WIDTH, VIEWPORT_HEIGHT);

	if (egl_window == EGL_NO_SURFACE) {
		fprintf(stderr, "Can't create egl window\n");
		exit(1);
	} else {
		fprintf(stderr, "Create egl window\n");
	}

	egl_surface = eglCreateWindowSurface(egl_display, egl_conf, egl_window, NULL);

	if (eglMakeCurrent(egl_display, egl_surface, egl_surface, egl_context)) {
		fprintf(stderr, "Make current\n");
	} else {
		fprintf(stderr, "Make current failed\n");
	}
	
	if (eglSwapBuffers(egl_display, egl_surface)) {
		fprintf(stderr, "Swapped buffers\n");
	} else {
		fprintf(stderr, "Swapped buffers failed\n");
	}
}


static GLuint create_shader(const char *source, GLenum shader_type)
{
	GLuint shader;
	GLint status;

	shader = glCreateShader(shader_type);

	glShaderSource(shader, 1, (const char **) &source, NULL);
	glCompileShader(shader);

	glGetShaderiv(shader, GL_COMPILE_STATUS, &status);
	if (!status) {
		char log[1000];
		GLsizei len;
		glGetShaderInfoLog(shader, 1000, &len, log);
		fprintf(stderr, "Error: compiling %s: %*s\n",
			shader_type == GL_VERTEX_SHADER ? "vertex" : "fragment",
			len, log);
		exit(1);
	}

	return shader;
}

static void init_gl()
{
	char *buf = (char *)malloc(WIDTH*HEIGHT*4);

#ifndef USE_PICTURE
	int blockWidth = WIDTH/16;
	int blockHeight = HEIGHT/16;
	int x = 0, y = 0;
	for (x = 0; x < WIDTH; x++) {
		for (y = 0; y < HEIGHT; y++) {
			int parityX = (x / blockWidth) & 1;
			int parityY = (y / blockHeight) & 1;
			unsigned char intensity = (parityX ^ parityY) ? 63 :191;
			buf[((y * WIDTH) + x) * 4] = intensity;
			buf[((y * WIDTH) + x) * 4 + 1] = 0x00;
			buf[((y * WIDTH) + x) * 4 + 2] = 0x00;
			buf[((y * WIDTH) + x) * 4 + 3] = 0xff;
		}
	}
#else
	int ret_size = 0;
	FILE *fIn = NULL;
	fIn = fopen("./pic_source/argb8888_bike.bin", "r");
	if (fIn == NULL) {
		fprintf(stderr, "Could not open bin file\n");
	}
	
	ret_size = fread(buf, 1, WIDTH*HEIGHT*4, fIn);
	if (ret_size <= 0) {
		fprintf(stderr, "Could not open bin file\n");
	}
	fclose(fIn);
#endif

	glGenTextures(1, &TexName);
	checkGLError("glGenTextures");
	glBindTexture(GL_TEXTURE_2D, TexName);
	checkGLError("glBindTexture");
	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, WIDTH, HEIGHT,
		0, GL_RGBA, GL_UNSIGNED_BYTE, buf);
	checkGLError("glTexImage2D");

	free(buf);


	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);


	GLuint frag, vert;
	GLint status;

	frag = create_shader(frag_shader_text, GL_FRAGMENT_SHADER);
	vert = create_shader(vert_shader_text, GL_VERTEX_SHADER);

	program = glCreateProgram();
	glAttachShader(program, frag);
	glAttachShader(program, vert);
	glLinkProgram(program);

	glGetProgramiv(program, GL_LINK_STATUS, &status);
	if (!status) {
		char log[1000];
		GLsizei len;
		glGetProgramInfoLog(program, 1000, &len, log);
		fprintf(stderr, "Error: linking:\n%*s\n", len, log);
	}

	gvPositionHandle = glGetAttribLocation(program, "vPosition");
	checkGLError("glGetAttribLocation");
	fprintf(stderr, "glGetAttribLocation vPosition = %d\n", gvPositionHandle);
	gvTexCoordHandle = glGetAttribLocation(program, "aTexCoords");
	checkGLError("glGetAttribLocation");
	fprintf(stderr, "glGetAttribLocation aTexCoords = %d\n", gvTexCoordHandle);
	
	guTexSamplerHandle = glGetUniformLocation(program, "uTexSampler");
	checkGLError("glGetUniformLocation");
	fprintf(stderr, "glGetAttribLocation guTexSamplerHandle = %d\n", guTexSamplerHandle);

	model_uniform = glGetUniformLocation(program, "model")
	view_uniform = glGetUniformLocation(program, "view")
	projection_uniform = glGetUniformLocation(program, "projection")
	
	glViewport(0, 0, VIEWPORT_WIDTH, VIEWPORT_HEIGHT);
	checkGLError("glViewport");
	
}

static void redraw()
{
	const GLfloat gTriangleVertices[] = {
		0.0f, 0.5f, 1.0f,
		0.5f, 0.5f, 0.5f,
		0.5f, 0.0f, 0.5f,
		0.0f, 0.0f, 1.0f,
	};

	const GLfloat gTexCoords[] = {
		0.0f, 0.2f,
		0.0f, 0.7f,
		0.5f, 0.7f,
		0.5f, 0.2f,
	};

	GLfloat model[4][4] = {
		{ 1.00, 0.00, 0.00, 0.00 },
		{ 0.00, 1.00, 0.00, 0.00 },
		{ 0.00, 0.00, 1.00, 0.00 },
		{ 0.50, 0.00, 0.00, 1.00 }
	};
	GLfloat view[4][4] = {
		{ 1.00, 0.00, 0.00, 0.00 },
		{ 0.00, 1.00, 0.00, 0.00 },
		{ 0.00, 0.00, 1.00, 0.00 },
		{ 0.00, 0.00, 0.00, 1.00 }
	};
	GLfloat projection[4][4] = {
		{ 1.00, 0.00, 0.00, 0.00 },
		{ 0.00, 1.00, 0.00, 0.00 },
		{ 0.00, 0.00, 1.00, 0.00 },
		{ 0.00, 0.00, 0.00, 1.00 }
	};

	glUniformMatrix4fv(model_uniform, 1, GL_FALSE,
		(GLfloat *)model);
	glUniformMatrix4fv(view_uniform, 1, GL_FALSE,
		(GLfloat *)view);
	glUniformMatrix4fv(projection_uniform, 1, GL_FALSE,
		(GLfloat *)projection);

	glClearColor(0.0, 0.0, 1.0, 1.0);
	checkGLError("glClearColor");
	glClear(GL_DEPTH_BUFFER_BIT | GL_COLOR_BUFFER_BIT);
	checkGLError("glClear");

	glUseProgram(program);

	glVertexAttribPointer(gvPositionHandle, 3, GL_FLOAT, GL_FALSE, 0, gTriangleVertices);
	glEnableVertexAttribArray(gvPositionHandle);

	glVertexAttribPointer(gvTexCoordHandle, 2, GL_FLOAT, GL_FALSE, 0, gTexCoords);
	glEnableVertexAttribArray(gvTexCoordHandle);

	glUniform1i(guTexSamplerHandle, 0);

	glBindTexture(GL_TEXTURE_2D, TexName);

	glDrawArrays(GL_TRIANGLE_FAN, 0, 4);

	eglSwapBuffers(egl_display, egl_surface);
}

int main(int argc, char **argv)
{
	get_server_reference();

	surface = wl_compositor_create_surface(compositor);
	if (surface == NULL) {
		fprintf(stderr, "Can't create surface\n");
		exit(1);
	} else {
		fprintf(stderr, "Create surface\n");
	}

	shell_surface = wl_shell_get_shell_surface(shell, surface);
	wl_shell_surface_set_toplevel(shell_surface);

	create_opaque_region();
	init_egl();
	create_window();
	init_gl();

	while (wl_display_dispatch(display) != -1) {
		redraw();
	}

	wl_display_disconnect(display);
	printf("disconnected from display\n");

	exit(0);
}
