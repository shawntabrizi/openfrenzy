// WEB_PRE.JS: runs first on the page and in each of the game's threads
// (port/web/CMakeLists.txt --pre-js). An error on one of the threads reaches the page's log
// with its stack (function names: --profiling-funcs). After OpenCE (CC0) port/web/src/web_pre.js.
if (typeof WorkerGlobalScope != 'undefined' && self instanceof WorkerGlobalScope) {
  self.addEventListener('error', (event) => {
    var error = event.error;
    var text = (error && error.stack) ? String(error.stack) : String(event.message);
    console.error('openfrenzy worker: ' + text);
  });
  self.addEventListener('unhandledrejection', (event) => {
    var reason = event.reason;
    console.error('openfrenzy worker: ' + ((reason && reason.stack) ? reason.stack : String(reason)));
  });
}
