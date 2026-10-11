/*
WEB_SHARED.H

The words of the game's memory the page reads itself (port/web/site/app.js): the
WebAssembly memory is shared, so the page's thread sees them as the game's threads do,
without a message. The page finds them with web_state (web_main.c) and knows them by these
indices, 32-bit words each: change them together. After OpenCE (CC0) port/web/src/web_shared.h
at commit e0b581b1 (the touch control words left out).
*/

#ifndef WEB_SHARED_H
#define WEB_SHARED_H

#include <stdint.h>

enum
{
	/* the frames shown so far (the page says it is loading while they stand still) */
	_web_frames_shown = 0,
	/* the game wants the mouse (relative mode, platform_mouse_capture) */
	_web_mouse_wanted = 1,
	/* frame time statistics (web_frame.c): frames in the last whole second, and the
	longest frame in it in microseconds */
	_web_fps = 2,
	_web_longest_frame_us = 3,
	WEB_SHARED_WORDS = 8,
};

extern volatile int32_t web_shared[WEB_SHARED_WORDS];

#endif
