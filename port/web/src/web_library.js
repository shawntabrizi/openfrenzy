/*
WEB_LIBRARY.JS

The JavaScript half of the web runtime (port/web/CMakeLists.txt --js-library). The game runs
on a worker (PROXY_TO_PTHREAD); web_js_post runs on the page's thread, where Emscripten
proxies it, and hands the text to the page (port/web/site/app.js, Module.openfrenzyMessage).
After OpenCE (CC0) port/web/src/web_library.js at commit e0b581b1.
*/

addToLibrary({
  // Emscripten's clock (clock_gettime, the game's GetTickCount and
  // QueryPerformanceCounter) with this thread's time origin read once (OpenCE: reading
  // performance.timeOrigin each time made a clock read 1.7 times as slow in Chrome).
  $webTimeOrigin: 0,
  emscripten_get_now__deps: ['$webTimeOrigin'],
  emscripten_get_now: () => (webTimeOrigin ||= performance.timeOrigin) + performance.now(),

  // kind: 0 a status, 1 a notice, 3 a fatal error (the game stops)
  web_js_post__proxy: 'sync',
  web_js_post: (kind, text) => {
    var message = UTF8ToString(text);
    if (Module.openfrenzyMessage) Module.openfrenzyMessage(kind, message);
    else if (kind == 3) console.error(message);
    else console.log(message);
  },
});
