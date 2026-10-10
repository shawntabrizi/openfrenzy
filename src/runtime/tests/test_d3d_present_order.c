/*
TEST_D3D_PRESENT_ORDER.C

The game's exact frame order, with nothing added: it creates the device
(bStartDisplay, 0x00042020) and then presents frames. It never calls
D3DDevice_SetVerticalBlankCallback or D3DDevice_BlockUntilVerticalBlank
(src/runtime/entry/xdk_entry_points.inc: 0 game callers).

d3d8_gl.c's D3DDevice_Present waits while two flips are pending, and only the
vertical blank thread retires flips; that thread starts only from those two
calls. So the third Present blocks for ever. This test fails (watchdog) until
the d3d8 module starts the thread itself (for example in Direct3D_CreateDevice).
It is listed in known_failures.txt.
*/

#include "platform.h"
#include "of_test.h"

#include <string.h>

int main(void)
{
	static struct of_watchdog watchdog = { 5, "D3DDevice_Present (the vertical blank thread never started)" };
	D3DPRESENT_PARAMETERS parameters;
	D3DDevice *device = NULL;
	unsigned long frame;

	setvbuf(stdout, NULL, _IONBF, 0);
	memset(&parameters, 0, sizeof(parameters));
	parameters.BackBufferWidth = 640;
	parameters.BackBufferHeight = 480;
	parameters.BackBufferFormat = (D3DFORMAT)7;
	parameters.BackBufferCount = 1;
	parameters.SwapEffect = 1;
	parameters.EnableAutoDepthStencil = TRUE;
	parameters.AutoDepthStencilFormat = (D3DFORMAT)0x2a;
	parameters.FullScreen_PresentationInterval = 1;
	Direct3DCreate8(D3D_SDK_VERSION);
	OF_CHECK(Direct3D_CreateDevice(0, D3DDEVTYPE_HAL, NULL, D3DCREATE_HARDWARE_VERTEXPROCESSING, &parameters,
		&device) == S_OK, "Direct3D_CreateDevice");
	of_watchdog_start(&watchdog);
	for (frame = 0; frame < 10; frame++)
	{
		D3DDevice_Clear(0, NULL, D3DCLEAR_TARGET, 0xff000000, 1.0f, 0);
		D3DDevice_Present(NULL, NULL, NULL, NULL);
		printf("  frame %lu presented\n", frame);
	}
	OF_CHECK(1, "10 frames presented in the game's order");
	return of_test_result("test_d3d_present_order");
}
