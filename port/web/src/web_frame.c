/*
WEB_FRAME.C

One yield to the browser per frame (docs/WEB.md, docs/ARCHITECTURE.md section 18).

The game's main runs on a worker (PROXY_TO_PTHREAD) and draws into the page's canvas as
an OffscreenCanvas. A worker's canvas shows what was drawn only when the worker returns to
its event loop. OpenCE (Halo) changed its main loop to run one iteration per browser frame;
this game presents from many loops (the mode loop, loading screens, the Bink player), so
instead platform_video_swap (src/runtime/sdl_platform.c) calls web_frame_yield, which
suspends the game's WebAssembly stack with JSPI (WebAssembly.Suspending) until the worker's
next animation frame, and resumes it there. The stack and every local stay as they are.

Only the thread that runs main may yield (JSPI suspends a stack that a promising export
started: main's). Present is called only from that thread.
*/

#include <emscripten.h>
#include <pthread.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>

#include "web_shared.h"

/* resolves at the worker's next animation frame (Chrome and Firefox give workers
requestAnimationFrame for OffscreenCanvas), else at the next turn of its event loop */
EM_ASYNC_JS(void, web_wait_animation_frame, (void), {
	await new Promise((resolve) => {
		if (typeof requestAnimationFrame === 'function')
			requestAnimationFrame(() => resolve());
		else
			setTimeout(resolve, 0);
	});
});

static pthread_t main_thread;
static int main_thread_known;

void web_frame_set_main_thread(void)
{
	main_thread = pthread_self();
	main_thread_known = 1;
}

void web_frame_yield(void)
{
	static double second_start, previous, waited, worked, yield_end;
	static int frames, log = -1;
	static double longest;
	double now, before;

	__atomic_add_fetch(&web_shared[_web_frames_shown], 1, __ATOMIC_RELAXED);
	if (!main_thread_known || !pthread_equal(pthread_self(), main_thread))
		return;
	before = emscripten_get_now();
	if (yield_end > 0.0)
		worked += before - yield_end;
	web_wait_animation_frame();
	now = emscripten_get_now();
	yield_end = now;
	waited += now - before;
	if (previous > 0.0 && now - previous > longest)
		longest = now - previous;
	previous = now;
	frames++;
	if (second_start == 0.0)
		second_start = now;
	if (now - second_start >= 1000.0)
	{
		__atomic_store_n(&web_shared[_web_fps], frames, __ATOMIC_RELAXED);
		__atomic_store_n(&web_shared[_web_longest_frame_us], (int32_t)(longest * 1000.0), __ATOMIC_RELAXED);
		/* OPENFRENZY_WEB_FRAME_LOG=1: once a second, the frames and where their time went */
		if (log < 0)
			log = getenv("OPENFRENZY_WEB_FRAME_LOG") != NULL;
		if (log)
			fprintf(stderr, "web frame: %d fps, longest %.1f ms, game %.0f ms, waiting for the browser %.0f ms\n",
				frames, longest, worked, waited);
		worked = 0.0;
		waited = 0.0;
		frames = 0;
		longest = 0.0;
		second_start = now;
	}
}
