/*
TEST_D3D_WINDOW.C

The real game's display start-up through the XDK layer, which is what makes
sdl_platform.c open the window: bStartDisplay (0x00042020, BDISPLAY/xbDisplay.cpp)
calls Direct3D_CreateDevice(0, D3DDEVTYPE_HAL, 0, 0x40, &parameters, &device) with
the parameters below, then D3DDevice_GetBackBuffer and GetDepthStencilSurface.
Each frame then clears and presents (D3DDevice_Present pumps the SDL events
through platform_pump_events and swaps through platform_video_swap).

Checks:
- the device is created and has a back buffer and a depth buffer;
- with a real video driver: a window with a GLES context exists
  (platform_video_drawable_size > 0, a GL renderer string) and no GL error is left;
- headless (SDL_VIDEO_DRIVER=dummy): device creation still succeeds, without a window;
- OF_TEST_FRAMES frames (default 120) present without blocking (watchdog 20 s).

The vertical blank thread is started first (D3DDevice_BlockUntilVerticalBlank),
which the game itself never does: without it the third D3DDevice_Present blocks
for ever (d3d8 module; test_d3d_present_order.c shows it). OF_TEST_START_VBLANK=0
leaves it out.
*/

#include "platform.h"
#include "sdl_platform.h"
#include "gl.h"
#include "of_test.h"

#include <string.h>

int main(void)
{
	static struct of_watchdog watchdog = { 20, "the frame loop (D3DDevice_Present)" };
	const char *driver = getenv("SDL_VIDEO_DRIVER");
	const char *frames_text = getenv("OF_TEST_FRAMES");
	const char *vblank = getenv("OF_TEST_START_VBLANK");
	BOOL headless = driver && !strcmp(driver, "dummy");
	unsigned long frames = frames_text ? strtoul(frames_text, NULL, 10) : 120;
	D3DPRESENT_PARAMETERS parameters;
	D3DDevice *device = NULL;
	D3DSurface *back_buffer = NULL, *depth_buffer = NULL;
	Direct3D *direct3d;
	HRESULT result;
	int width = 0, height = 0;
	unsigned long frame;

	setvbuf(stdout, NULL, _IONBF, 0);
	printf("test_d3d_window: %s, %lu frames\n", headless ? "headless" : "real window", frames);

	/* as bStartDisplay fills them (local_15c in export/funcs/00042020.c) */
	memset(&parameters, 0, sizeof(parameters));
	parameters.BackBufferWidth = 640;
	parameters.BackBufferHeight = 480;
	parameters.BackBufferFormat = (D3DFORMAT)7;        /* D3DFMT_X8R8G8B8 on the Xbox */
	parameters.BackBufferCount = 1;
	parameters.MultiSampleType = 0;
	parameters.SwapEffect = 1;         /* D3DSWAPEFFECT_DISCARD */
	parameters.EnableAutoDepthStencil = TRUE;
	parameters.AutoDepthStencilFormat = (D3DFORMAT)0x2a; /* D3DFMT_D24S8 on the Xbox */
	parameters.Flags = 0;
	parameters.FullScreen_PresentationInterval = 1;    /* D3DPRESENT_INTERVAL_ONE */

	direct3d = Direct3DCreate8(D3D_SDK_VERSION);
	OF_CHECK(direct3d != NULL, "Direct3DCreate8");
	result = Direct3D_CreateDevice(0, D3DDEVTYPE_HAL, NULL, D3DCREATE_HARDWARE_VERTEXPROCESSING, &parameters, &device);
	OF_CHECK(result == S_OK && device, "Direct3D_CreateDevice (result %08lx)", (unsigned long)result);
	D3DDevice_GetBackBuffer(0, D3DBACKBUFFER_TYPE_MONO, &back_buffer);
	OF_CHECK(back_buffer != NULL, "D3DDevice_GetBackBuffer");
	OF_CHECK(D3DDevice_GetDepthStencilSurface(&depth_buffer) == S_OK && depth_buffer,
		"D3DDevice_GetDepthStencilSurface");

	platform_video_drawable_size(&width, &height);
	if (headless)
	{
		OF_CHECK(width == 0 && height == 0, "no window without a video driver (%dx%d)", width, height);
	}
	else
	{
		const char *renderer = (const char *)glGetString(GL_RENDERER);

		OF_CHECK(width > 0 && height > 0, "the window has a drawable (%dx%d pixels)", width, height);
		OF_CHECK(renderer && *renderer, "a GLES context is current (%s)", renderer ? renderer : "none");
	}

	if (!vblank || strcmp(vblank, "0"))
		D3DDevice_BlockUntilVerticalBlank();
	of_watchdog_start(&watchdog);
	for (frame = 0; frame < frames; frame++)
	{
		/* a colour that changes each frame: red up, blue down */
		DWORD shade = (DWORD)(frame * 255 / (frames ? frames : 1));

		D3DDevice_Clear(0, NULL, D3DCLEAR_TARGET | D3DCLEAR_ZBUFFER | D3DCLEAR_STENCIL,
			0xff000000 | shade << 16 | 0x40 << 8 | (255 - shade), 1.0f, 0);
		D3DDevice_Present(NULL, NULL, NULL, NULL);
	}
	OF_CHECK(1, "%lu frames presented", frames);
	if (!headless)
	{
		GLenum error = glGetError();

		OF_CHECK(error == GL_NO_ERROR, "no GL error after the frames (0x%04x)", error);
		platform_video_drawable_size(&width, &height);
		OF_CHECK(width > 0 && height > 0, "the window still has a drawable (%dx%d)", width, height);
	}
	return of_test_result("test_d3d_window");
}
