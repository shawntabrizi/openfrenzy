/*
APP.JS

The launcher page (index.html), after OpenCE's port/web/site/app.js (CC0, commit e0b581b1).
It checks that the browser can run the game, copies the player's disc image into the site's
private storage (import-worker.js), and starts the game (openfrenzy.js and openfrenzy.wasm,
port/web/CMakeLists.txt), whose canvas then fills the page.

- The game's threads need SharedArrayBuffer, so the page must be cross-origin isolated: the
  server sends COOP and COEP (scripts/web_serve.py).
- The game reads /data/game.xiso.iso and keeps its saves in /data/saves: the Origin Private
  File System, which port/web/src/web_main.c mounts at /data.
- Query parameters, for testing: ?set=NAME=value (any number) passes an environment variable
  to the game (the OPENFRENZY_* settings, docs/BUILD.md); ?play starts the game at once when
  the disc image is there; ?asyncify loads the Asyncify build even where JSPI is. For browsers
  that cannot be driven (Safari): ?image-url=/__disc imports the disc image from the local
  server (scripts/web_serve.py --disc), and ?report posts the log lines to it (--report).
- Two builds of the game: openfrenzy.js (JSPI, the default) and, when the site has it,
  openfrenzy-asyncify.js for browsers without JSPI (Safari 26; port/web/CMakeLists.txt
  OPENFRENZY_WEB_ASYNCIFY).
*/

'use strict';

const $ = (id) => document.getElementById(id);
const params = new URLSearchParams(location.search);
// the game's memory: port/web/CMakeLists.txt INITIAL_MEMORY
const WEB_MEMORY_BYTES = 0x88000000;
const MAXIMUM_LOG_LINES = 20000;
// web_shared.h
const WEB_FRAMES_SHOWN = 0, WEB_MOUSE_WANTED = 1, WEB_FPS = 2, WEB_LONGEST_FRAME_US = 3;

// ---------- the log

const log = [];

let reportQueue = params.has('report') ? [] : null;

function logLine(text) {
  for (const line of String(text).replace(/\r/g, '').split('\n')) {
    if (line === '') continue;
    log.push(line);
    if (reportQueue) reportQueue.push(`[${(performance.now() / 1000).toFixed(1)}] ${line}`);
  }
  if (log.length > MAXIMUM_LOG_LINES) log.splice(0, log.length - MAXIMUM_LOG_LINES);
}

function download(name, blob) {
  const link = document.createElement('a');
  link.href = URL.createObjectURL(blob);
  link.download = name;
  document.body.append(link);
  link.click();
  link.remove();
  setTimeout(() => URL.revokeObjectURL(link.href), 10000);
}

function downloadLog() {
  const text = `OpenFrenzy web\n${navigator.userAgent}\n\n` + log.join('\n') + '\n';
  download('openfrenzy-web-log.txt', new Blob([text], { type: 'text/plain' }));
}

window.addEventListener('error', (event) => logLine('page error: ' + (event.error && event.error.stack ? event.error.stack : event.message)));
window.addEventListener('unhandledrejection', (event) => {
  const reason = event.reason;
  logLine('page error: ' + (reason && reason.stack ? reason.stack : String(reason)));
});

// ---------- the browser's checks

const results = {};

function addCheck(name, good, text, detail) {
  results[name] = good;
  const item = document.createElement('li');
  item.className = good ? 'good' : 'bad';
  item.append(text);
  if (detail && !good) {
    const span = document.createElement('span');
    span.className = 'detail';
    span.textContent = ' ' + detail;
    item.append(span);
  }
  $('checks').append(item);
}

function workerWebGL2() {
  return new Promise((resolve) => {
    const source = "try { postMessage(!!new OffscreenCanvas(1, 1).getContext('webgl2')); } catch (e) { postMessage(false); }";
    let worker;
    try {
      worker = new Worker(URL.createObjectURL(new Blob([source], { type: 'text/javascript' })));
    } catch {
      resolve(false);
      return;
    }
    const timer = setTimeout(() => { worker.terminate(); resolve(false); }, 5000);
    worker.onmessage = (event) => { clearTimeout(timer); worker.terminate(); resolve(event.data === true); };
    worker.onerror = () => { clearTimeout(timer); worker.terminate(); resolve(false); };
  });
}

function memoryAvailable() {
  try {
    const pages = WEB_MEMORY_BYTES / 65536;
    new WebAssembly.Memory({ initial: pages, maximum: pages, shared: true });
    return true;
  } catch {
    return false;
  }
}

// the build to load: openfrenzy.js needs JSPI; openfrenzy-asyncify.js (if the site has it) not
let gameScript = 'openfrenzy.js';

async function siteHas(name) {
  try {
    return (await fetch(name, { method: 'HEAD', cache: 'no-store' })).ok;
  } catch {
    return false;
  }
}

async function runChecks() {
  $('checks').textContent = '';
  addCheck('isolated', window.crossOriginIsolated, 'Threads (cross-origin isolation)',
    'the server must send the COOP and COEP headers (scripts/web_serve.py does)');
  const jspi = typeof WebAssembly.Suspending === 'function';
  const fallback = await siteHas('openfrenzy-asyncify.js');
  if ((!jspi || params.has('asyncify')) && fallback) {
    gameScript = 'openfrenzy-asyncify.js';
    addCheck('jspi', true, jspi ? 'The Asyncify build (?asyncify)' :
      'No WebAssembly stack switching (JSPI): the Asyncify build, which is slower');
  } else {
    addCheck('jspi', jspi, 'WebAssembly stack switching (JSPI)',
      'Chrome, Edge and Firefox have it; docs/WEB.md lists the others');
  }
  addCheck('webgl', typeof OffscreenCanvas !== 'undefined' && await workerWebGL2(), 'WebGL 2 from a worker');
  addCheck('opfs', !!(navigator.storage && navigator.storage.getDirectory), "The site's private storage (OPFS)");
  addCheck('memory', window.crossOriginIsolated && memoryAvailable(), 'Memory for the game (2.1 GB)',
    'this browser could not give the game that much: a 64-bit desktop browser has it');
  return Object.values(results).every(Boolean);
}

// ---------- the disc image (import-worker.js)

let dataReady = false;

function formatBytes(bytes) {
  return bytes >= 1e9 ? (bytes / 1e9).toFixed(2) + ' GB' : Math.round(bytes / 1e6) + ' MB';
}

async function readJson(name) {
  try {
    const root = await navigator.storage.getDirectory();
    return JSON.parse(await (await (await root.getFileHandle(name)).getFile()).text());
  } catch {
    return null;
  }
}

async function refreshData() {
  const marker = await readJson('game.json');
  dataReady = !!marker;
  $('data-status').textContent = marker ?
    `Disc image ready: ${marker.name} (${formatBytes(marker.size)}), copied ${marker.copied.slice(0, 10)}.` :
    'No disc image yet: choose your disc image of Fuzion Frenzy (an .iso or .xiso file).';
  $('delete-data').disabled = !dataReady;
  await refreshStorage();
  updatePlay();
}

async function refreshStorage() {
  if (!navigator.storage || !navigator.storage.estimate) return;
  const { usage, quota } = await navigator.storage.estimate();
  const persisted = navigator.storage.persisted ? await navigator.storage.persisted() : false;
  $('storage-status').textContent = `This site keeps ${formatBytes(usage || 0)} of the ${formatBytes(quota || 0)} ` +
    `the browser allows it. ${persisted ? 'The browser keeps it.' : 'The browser may clear it when space runs low.'}`;
}

function setImporting(importing) {
  $('image-label').classList.toggle('disabled', importing);
  $('image-input').disabled = importing;
  $('delete-data').disabled = importing || !dataReady;
  $('play').disabled = importing || !canPlay();
}

if (reportQueue) {
  setInterval(() => {
    if (!reportQueue.length) return;
    const body = reportQueue.join('\n');
    reportQueue = [];
    fetch('/__report', { method: 'POST', body }).catch(() => {});
  }, 1000);
}

function importImage(file, url) {
  const worker = new Worker('import-worker.js');
  $('import-progress').hidden = false;
  $('import-bar').style.width = '0';
  $('import-text').textContent = 'Reading…';
  setImporting(true);
  // (asked before the copy: the image is 2 GB the browser should not drop)
  if (navigator.storage && navigator.storage.persist) navigator.storage.persist().catch(() => {});
  const started = performance.now();
  worker.onmessage = async (event) => {
    const data = event.data;
    if (data.type === 'progress') {
      $('import-bar').style.width = (100 * data.done / data.total).toFixed(1) + '%';
      $('import-text').textContent = `Copying: ${formatBytes(data.done)} of ${formatBytes(data.total)}`;
      return;
    }
    worker.terminate();
    if (data.type === 'done') {
      const seconds = ((performance.now() - started) / 1000).toFixed(1);
      $('import-text').textContent = `Copied in ${seconds} s.`;
      logLine(`import: ${data.bytes} bytes in ${seconds} s`);
    } else {
      $('import-text').textContent = data.message;
      logLine('import failed: ' + data.message);
    }
    setImporting(false);
    await refreshData();
  };
  worker.onerror = (event) => {
    worker.terminate();
    setImporting(false);
    $('import-text').textContent = 'The copy failed: ' + event.message;
  };
  worker.postMessage(url ? { url } : { file });
}

$('image-input').addEventListener('change', (event) => {
  const file = event.target.files[0];
  event.target.value = '';
  if (file) importImage(file);
});

$('delete-data').addEventListener('click', async () => {
  if (!confirm('Delete the disc image copied into this browser? Saves stay.')) return;
  const root = await navigator.storage.getDirectory();
  await root.removeEntry('game.json').catch(() => {});
  await root.removeEntry('game.xiso.iso').catch(() => {});
  await refreshData();
});

// ---------- saves out, as a zip (stored, not compressed)

const crcTable = (() => {
  const table = new Uint32Array(256);
  for (let n = 0; n < 256; n++) {
    let c = n;
    for (let k = 0; k < 8; k++) c = c & 1 ? 0xEDB88320 ^ (c >>> 1) : c >>> 1;
    table[n] = c >>> 0;
  }
  return table;
})();

function crc32(bytes) {
  let crc = 0xFFFFFFFF;
  for (let i = 0; i < bytes.length; i++) crc = crcTable[(crc ^ bytes[i]) & 0xFF] ^ (crc >>> 8);
  return (crc ^ 0xFFFFFFFF) >>> 0;
}

function zip(entries) {
  const encoder = new TextEncoder();
  const parts = [];
  const central = [];
  let offset = 0;
  for (const { name, data } of entries) {
    const nameBytes = encoder.encode(name);
    const crc = crc32(data);
    const local = new DataView(new ArrayBuffer(30));
    local.setUint32(0, 0x04034b50, true);
    local.setUint16(4, 20, true);
    local.setUint16(6, 0x0800, true);
    local.setUint32(14, crc, true);
    local.setUint32(18, data.length, true);
    local.setUint32(22, data.length, true);
    local.setUint16(26, nameBytes.length, true);
    const header = new DataView(new ArrayBuffer(46));
    header.setUint32(0, 0x02014b50, true);
    header.setUint16(4, 20, true);
    header.setUint16(6, 20, true);
    header.setUint16(8, 0x0800, true);
    header.setUint32(16, crc, true);
    header.setUint32(20, data.length, true);
    header.setUint32(24, data.length, true);
    header.setUint16(28, nameBytes.length, true);
    header.setUint32(42, offset, true);
    parts.push(local.buffer, nameBytes, data);
    central.push(header.buffer, nameBytes);
    offset += 30 + nameBytes.length + data.length;
  }
  const centralSize = central.reduce((sum, part) => sum + part.byteLength, 0);
  const end = new DataView(new ArrayBuffer(22));
  end.setUint32(0, 0x06054b50, true);
  end.setUint16(8, entries.length, true);
  end.setUint16(10, entries.length, true);
  end.setUint32(12, centralSize, true);
  end.setUint32(16, offset, true);
  return new Blob([...parts, ...central, end.buffer], { type: 'application/zip' });
}

async function collect(directory, prefix, entries) {
  for await (const [name, handle] of directory.entries()) {
    if (handle.kind === 'directory') await collect(handle, prefix + name + '/', entries);
    else entries.push({ name: prefix + name, data: new Uint8Array(await (await handle.getFile()).arrayBuffer()) });
  }
}

$('export-saves').addEventListener('click', async () => {
  const entries = [];
  try {
    const root = await navigator.storage.getDirectory();
    await collect(await root.getDirectoryHandle('saves'), 'saves/', entries);
  } catch {
    // no saves yet
  }
  if (!entries.length) {
    alert('There are no saves yet.');
    return;
  }
  download('openfrenzy-saves.zip', zip(entries));
});

$('download-log').addEventListener('click', downloadLog);
$('fatal-log-download').addEventListener('click', downloadLog);
$('fatal-reload').addEventListener('click', () => location.reload());

// ---------- settings (kept in this browser, passed to the game as OPENFRENZY_* variables)

const SETTINGS = [
  { id: 'set-resolution', name: 'OPENFRENZY_DISPLAY_RESOLUTION_SCALING' },
  { id: 'set-aa', name: 'OPENFRENZY_DISPLAY_ANTI_ALIASING' },
  { id: 'set-aspect', name: 'OPENFRENZY_DISPLAY_ASPECT_RATIO' },
  { id: 'set-videos', name: 'OPENFRENZY_DEBUG_SKIP_VIDEOS', value: (v) => (v === 'off' ? 'true' : 'false') },
];

function loadSettings() {
  for (const setting of SETTINGS) {
    try {
      const stored = localStorage.getItem('openfrenzy.' + setting.id);
      if (stored) $(setting.id).value = stored;
    } catch {
      // no localStorage: the defaults
    }
    $(setting.id).addEventListener('change', () => {
      try {
        localStorage.setItem('openfrenzy.' + setting.id, $(setting.id).value);
      } catch {
        // as above
      }
    });
  }
}

function gameArguments() {
  const args = [];
  for (const setting of SETTINGS) {
    const value = $(setting.id).value;
    args.push(`--${setting.name}=${setting.value ? setting.value(value) : value}`);
  }
  // ?set= comes last, so that it wins
  for (const value of params.getAll('set')) {
    if (/^[A-Z0-9_]+=/.test(value)) args.push('--' + value);
  }
  return args;
}

// ---------- the game

let checksPassed = false;
let gameStarted = false;
let gameStopped = false;
let game = null;

function canPlay() {
  return checksPassed && dataReady && !gameStarted;
}

function updatePlay() {
  $('play').disabled = !canPlay();
  $('play-note').textContent = !checksPassed ? 'This browser cannot run the game (above).' :
    !dataReady ? 'The disc image is needed first.' : '';
}

function showFatal(text) {
  if (gameStopped) return;
  gameStopped = true;
  $('fatal-text').textContent = text;
  $('fatal-log').textContent = log.slice(-80).join('\n');
  $('fatal').hidden = false;
  $('starting').hidden = true;
  if (document.exitPointerLock) document.exitPointerLock();
  // (for the automated test: tools/web-test, docs/WEB.md)
  window.openfrenzyFatal = text;
}

function loadScript(src) {
  return new Promise((resolve, reject) => {
    const script = document.createElement('script');
    script.src = src;
    script.onload = resolve;
    script.onerror = () => reject(new Error('could not load ' + src));
    document.head.append(script);
  });
}

// the canvas's drawing buffer follows its size on the page (device pixels)
function sizeCanvas() {
  const canvas = $('canvas');
  const ratio = window.devicePixelRatio || 1;
  return [Math.max(1, Math.round(canvas.clientWidth * ratio)), Math.max(1, Math.round(canvas.clientHeight * ratio))];
}

async function play() {
  if (!canPlay()) return;
  gameStarted = true;
  $('launcher').hidden = true;
  $('game').hidden = false;
  const canvas = $('canvas');
  [canvas.width, canvas.height] = sizeCanvas();
  canvas.focus();
  watchFrames();
  try {
    logLine('page: loading ' + gameScript);
    await loadScript(gameScript);
    const options = {
      canvas,
      arguments: gameArguments(),
      print: (text) => { logLine(text); console.log(text); },
      printErr: (text) => { logLine(text); console.warn(text); },
      openfrenzyMessage: (kind, text) => {
        logLine(text);
        if (kind === 3) showFatal(text);
      },
      onAbort: (what) => showFatal('The game stopped: ' + what),
      onExit: (status) => showFatal(`The game ended (status ${status}).`),
    };
    game = await window.createOpenFrenzy(options);
    window.openfrenzyGame = game;
  } catch (error) {
    showFatal(String(error && error.message ? error.message : error));
  }
}

function sharedWord(index) {
  if (!game || !game._web_state) return 0;
  return game.HEAPU32[(game._web_state() >>> 2) + index];
}

// The game counts the frames it shows (web_shared.h): while the count stands still, the game
// is starting or loading, and the page says so.
function watchFrames() {
  let shown = -1;
  let changed = performance.now();
  const timer = setInterval(() => {
    if (gameStopped) {
      clearInterval(timer);
      return;
    }
    const now = sharedWord(WEB_FRAMES_SHOWN);
    $('fps').textContent = game ? `${sharedWord(WEB_FPS)} fps` : '';
    if (now !== shown) {
      if (shown < 0 && now === 0) return;
      shown = now;
      changed = performance.now();
      $('starting').hidden = true;
    } else if (performance.now() - changed > 1000 && $('starting').hidden) {
      $('starting-text').textContent = 'Loading…';
      $('starting').hidden = false;
    }
  }, 250);
}

// the game wants the mouse (web_shared.h): a click takes the pointer lock (a page may ask only
// from a click)
$('canvas').addEventListener('pointerdown', (event) => {
  const canvas = event.currentTarget;
  if (event.pointerType !== 'mouse' || document.pointerLockElement === canvas) return;
  if (!sharedWord(WEB_MOUSE_WANTED)) return;
  const request = canvas.requestPointerLock();
  if (request && request.catch) request.catch(() => {});
});

$('play').addEventListener('click', play);

$('fullscreen').addEventListener('click', async () => {
  try {
    if (document.fullscreenElement) await document.exitFullscreen();
    else await $('game').requestFullscreen({ navigationUI: 'hide' });
    if (navigator.keyboard && navigator.keyboard.lock && document.fullscreenElement) {
      await navigator.keyboard.lock(['Escape']).catch(() => {});
    }
  } catch (error) {
    logLine('fullscreen: ' + error);
  }
});

// keys the game uses must not scroll the page or move the focus
window.addEventListener('keydown', (event) => {
  if (gameStarted && !gameStopped && ['Tab', 'Space', 'ArrowUp', 'ArrowDown', 'ArrowLeft', 'ArrowRight', 'Backspace'].includes(event.code)) {
    event.preventDefault();
  }
}, true);
$('canvas').addEventListener('click', () => $('canvas').focus());
$('canvas').addEventListener('contextmenu', (event) => event.preventDefault());

// ---------- start

(async () => {
  loadSettings();
  checksPassed = await runChecks();
  if (!checksPassed) {
    $('checks-note').textContent = 'Desktop Chrome or Edge (version 137 or later) run the game. docs/WEB.md lists the state of the other browsers.';
    $('checks-note').hidden = false;
  }
  if (results.opfs) await refreshData();
  else updatePlay();
  logLine('page: checks: ' + Object.entries(results).map(([k, v]) => `${k} ${v ? 'good' : 'bad'}`).join(', '));
  // ?wipe (automated runs): remove everything this site keeps in this browser
  if (params.has('wipe')) {
    const root = await navigator.storage.getDirectory();
    for await (const [name] of root.entries()) await root.removeEntry(name, { recursive: true }).catch(() => {});
    logLine('page: wiped the storage');
    await refreshData();
    return;
  }
  window.openfrenzyReady = true;
  if (params.has('image-url') && !dataReady && /^\/__disc$/.test(params.get('image-url'))) {
    const started = performance.now();
    importImage(null, params.get('image-url'));
    while (!dataReady) await new Promise((resolve) => setTimeout(resolve, 500));
    logLine(`page: image imported in ${((performance.now() - started) / 1000).toFixed(1)} s`);
  }
  if (params.has('play')) play();
})();
