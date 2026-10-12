// DRIVE.MJS: runs the web build in a browser and records what it does (docs/WEB.md "Tests").
//
//   node port/web/tests/drive.mjs --site build-web/site --iso orig/game.xiso.iso \
//        --seconds 60 --shots 10,30,60 --out /tmp/run [--set NAME=value ...] [--keys "..."]
//        [--browser chrome|firefox|webkit] [--headed] [--pads N] [--pad "..."] [--mark RE]
//
// It serves the site itself (scripts/web_serve.py on 127.0.0.1, a fixed port so that the
// browser's storage, which belongs to the origin, is found again), opens it in a browser with
// a profile kept in tools/web-test/profile-<browser> (so the disc image is copied once), copies
// the disc image in if the page has none, starts the game and waits. Every console line goes
// to <out>/console.log; screenshots of the page are <out>/shot-<seconds>.png. The browser, the
// server and this script stop after --seconds (and a hard limit of --seconds + 600 s for the
// copy and the start).
//
// --url URL runs a site served elsewhere instead (the hosted build: docs/WEB.md "Hosting"); no
// server is started, and the driver waits until the page is cross-origin isolated (its service
// worker reloads it once on the first visit).
//
// Exit status: 0, or 1 if the game stopped (the page's fatal panel), or 2 on a driver error.
// Playwright is not in the repository: install it into tools/web-test (git-ignored):
//   mkdir -p tools/web-test && cd tools/web-test && npm init -y && npm install playwright@1.58.2
//   (and puppeteer-core@24.10.0 for --browser firefox-system; Playwright 1.62.0 in
//   tools/web-test-webkit for --browser webkit, whose WebKit build is 26.5)

import { spawn } from 'node:child_process';
import { createRequire } from 'node:module';
import fs from 'node:fs';
import path from 'node:path';
import { setTimeout as sleep } from 'node:timers/promises';

const root = path.resolve(path.dirname(new URL(import.meta.url).pathname), '../../..');
// (WebKit: tools/web-test-webkit, a Playwright whose WebKit build is installed, when it exists)
const webkitDir = path.join(root, 'tools/web-test-webkit/package.json');
const require = createRequire(process.argv.includes('webkit') && fs.existsSync(webkitDir) ? webkitDir :
  path.join(root, 'tools/web-test/package.json'));
let playwright;
try {
  playwright = require('playwright');
} catch {
  // (exit 77: ctest reports the test as skipped, docs/WEB.md "Tests")
  console.log('drive: SKIP: Playwright is not installed in tools/web-test (docs/WEB.md "Tests")');
  process.exit(77);
}

function bidiKey(key) {
  return key === 'Space' ? ' ' : key === 'Enter' ? '\uE006' : key;
}

// The browser and its page, behind the few calls this driver makes. chrome (the installed
// Chrome), firefox and webkit go through Playwright (firefox and webkit: Playwright's own
// builds); firefox-system is the installed Firefox, through Puppeteer and WebDriver BiDi (a
// stock Firefox has no Playwright protocol).
async function openBrowser(kind, profile, headless, viewport) {
  if (kind === 'firefox-system') {
    const puppeteer = require('puppeteer-core');
    const browser = await puppeteer.launch({
      browser: 'firefox', headless, userDataDir: profile,
      executablePath: process.env.OPENFRENZY_FIREFOX || '/Applications/Firefox.app/Contents/MacOS/firefox',
      defaultViewport: viewport,
      extraPrefsFirefox: { 'media.autoplay.default': 0, 'media.autoplay.blocking_policy': 0 },
    });
    const page = (await browser.pages())[0] || await browser.newPage();
    return {
      version: await browser.version(),
      close: () => browser.close(),
      addInitScript: (fn) => page.evaluateOnNewDocument(fn),
      on: (event, handler) => page.on(event === 'crash' ? 'error' : event, handler),
      goto: (url) => page.goto(url),
      waitForFunction: (fn, arg, options) => page.waitForFunction(fn, options || {}, arg),
      $$eval: (selector, fn) => page.$$eval(selector, fn),
      textContent: (selector) => page.$eval(selector, (element) => element.textContent),
      setInputFiles: async (selector, file) => (await page.$(selector)).uploadFile(file),
      click: (selector) => page.click(selector),
      screenshot: (options) => page.screenshot(options),
      // (BiDi names the space bar by its character, and Puppeteer's "Enter" is the keypad's
      // Enter there: WebDriver's Return key, U+E006, is the main one, SDL's RETURN)
      keyboard: {
        down: (key) => page.keyboard.down(bidiKey(key)),
        up: (key) => page.keyboard.up(bidiKey(key)),
      },
      evaluate: (fn, arg) => page.evaluate(fn, arg),
    };
  }
  const type = kind === 'firefox' ? 'firefox' : kind === 'webkit' ? 'webkit' : 'chromium';
  const launch = { headless, viewport };
  if (type === 'chromium') {
    launch.channel = 'chrome';
    launch.args = ['--ignore-gpu-blocklist', '--autoplay-policy=no-user-gesture-required'];
  }
  const context = await playwright[type].launchPersistentContext(profile, launch);
  const page = context.pages()[0] || await context.newPage();
  return {
    version: context.browser() ? context.browser().version() : kind,
    close: () => context.close(),
    addInitScript: (fn) => page.addInitScript(fn),
    on: (event, handler) => page.on(event, handler),
    goto: (url) => page.goto(url),
    waitForFunction: (fn, arg, options) => page.waitForFunction(fn, arg, options),
    $$eval: (selector, fn) => page.$$eval(selector, fn),
    textContent: (selector) => page.textContent(selector),
    setInputFiles: (selector, file) => page.setInputFiles(selector, file),
    click: (selector) => page.click(selector),
    screenshot: (options) => page.screenshot(options),
    keyboard: page.keyboard,
    evaluate: (fn, arg) => page.evaluate(fn, arg),
  };
}

function options(argv) {
  const out = { site: 'build-web/site', seconds: 60, shots: [], set: [], keys: [], pad: [], pads: 0, browser: 'chrome', port: 8765,
    headed: false, expect: [], forbid: [], query: [], viewport: { width: 1280, height: 960 } };
  for (let i = 0; i < argv.length; i++) {
    const name = argv[i];
    const value = () => argv[++i];
    if (name === '--site') out.site = value();
    else if (name === '--url') out.url = value();
    else if (name === '--iso') out.iso = value();
    else if (name === '--seconds') out.seconds = Number(value());
    else if (name === '--shots') out.shots = value().split(',');
    else if (name === '--mark') out.mark = value();
    else if (name === '--out') out.out = value();
    else if (name === '--set') out.set.push(value());
    else if (name === '--keys') out.keys.push(value());
    else if (name === '--pad') out.pad.push(value());
    else if (name === '--pads') out.pads = Number(value());
    else if (name === '--pull') out.pull = value();
    else if (name === '--browser') out.browser = value();
    else if (name === '--port') out.port = Number(value());
    else if (name === '--headed') out.headed = true;
    else if (name === '--expect') out.expect.push(value());
    else if (name === '--forbid') out.forbid.push(value());
    else if (name === '--profile') out.profile = value();
    else if (name === '--min-fps') out.minFps = Number(value());
    else if (name === '--query') out.query.push(value());
    else if (name === '--viewport') {
      const [width, height] = value().split('x').map(Number);
      out.viewport = { width, height };
    }
    else throw new Error('unknown option ' + name);
  }
  if (!out.out) throw new Error('--out is needed');
  // (--pad alone: one pad, as before --pads)
  if (out.pad.length && !out.pads) out.pads = 1;
  if (!(out.pads >= 0 && out.pads <= 4)) throw new Error('--pads takes 0 to 4');
  return out;
}

const opts = options(process.argv.slice(2));
if (opts.iso && !fs.existsSync(opts.iso)) {
  console.log(`drive: SKIP: no disc image at ${opts.iso} (the player's own; never in the repository)`);
  process.exit(77);
}
fs.mkdirSync(opts.out, { recursive: true });
const consolePath = path.join(opts.out, 'console.log');
const consoleFile = fs.createWriteStream(consolePath);
const lines = [];
function record(text) {
  const line = `[${(performance.now() / 1000).toFixed(1)}] ${text}`;
  lines.push(line);
  consoleFile.write(line + '\n');
}

const server = opts.url ? null : spawn('python3', ['-I', path.join(root, 'scripts/web_serve.py'), path.resolve(opts.site),
  '--port', String(opts.port), '--timeout', String(opts.seconds + 900)],
  { stdio: ['ignore', 'inherit', 'inherit'], env: { ...process.env, WEB_SERVE_QUIET: '1' } });

let context;
let status = 0;
const hardLimit = setTimeout(() => {
  record('driver: hard time limit reached');
  if (context) context.close().catch(() => {});
  if (server) server.kill('SIGKILL');
  process.exit(2);
}, (opts.seconds + 900) * 1000);

try {
  await sleep(800);
  context = await openBrowser(opts.browser, path.resolve(opts.profile || path.join(root, `tools/web-test/profile-${opts.browser}`)),
    !opts.headed, opts.viewport);
  const page = context;
  record('driver: browser ' + opts.browser + ' ' + context.version);
  // Sound check: every Web Audio node connected to the speakers is also connected to an
  // analyser, whose level the driver logs ("audio: peak P rms R, context C"). Muted headless
  // output does not change what the graph produces.
  await page.addInitScript(() => {
    const original = AudioNode.prototype.connect;
    window.openfrenzyAudioTaps = [];
    AudioNode.prototype.connect = function (target, ...rest) {
      const result = original.call(this, target, ...rest);
      if (target instanceof AudioDestinationNode) {
        const analyser = this.context.createAnalyser();
        analyser.fftSize = 2048;
        original.call(this, analyser);
        window.openfrenzyAudioTaps.push(analyser);
      }
      return result;
    };
    // --pad / --pads: scripted gamepads of the standard mapping (the Gamepad API has no pad in a
    // headless browser). Pads 0 to count - 1 are connected, each with its own id, index and
    // gamepadconnected event; navigator.getGamepads returns them (null for the free indices).
    // window.openfrenzyPad(pad, button, down) presses standard button indices (0 A, 1 B, 2 X,
    // 3 Y, 9 Start, 12-15 the d-pad) and returns false for a pad that is not connected;
    // window.openfrenzyPadPlug(pad, connected) unplugs a pad (gamepaddisconnected) or plugs in a
    // new one at that index (gamepadconnected) and returns false when there is nothing to do.
    window.openfrenzyPadInstall = (count) => {
      const pads = [null, null, null, null];
      const announce = (pad, type) => {
        const event = new Event(type);
        Object.defineProperty(event, 'gamepad', { value: pad });
        window.dispatchEvent(event);
      };
      window.openfrenzyPadPlug = (index, connected) => {
        if (!!pads[index] === connected) return false;
        if (!connected) {
          const pad = pads[index];
          pads[index] = null;
          pad.connected = false;
          announce(pad, 'gamepaddisconnected');
          return true;
        }
        pads[index] = {
          id: `OpenFrenzy scripted pad ${index} (STANDARD GAMEPAD)`, index, connected: true, mapping: 'standard',
          timestamp: performance.now(), axes: [0, 0, 0, 0],
          buttons: Array.from({ length: 17 }, () => ({ pressed: false, touched: false, value: 0 })),
          vibrationActuator: null,
        };
        announce(pads[index], 'gamepadconnected');
        return true;
      };
      navigator.getGamepads = () => [...pads];
      for (let index = 0; index < count; index++) window.openfrenzyPadPlug(index, true);
      window.openfrenzyPad = (index, button, down) => {
        const pad = pads[index];
        if (!pad) return false;
        pad.buttons[button] = { pressed: down, touched: down, value: down ? 1 : 0 };
        pad.timestamp = performance.now();
        return true;
      };
    };
    // the key events the page receives (to tell a driver problem from a game one)
    window.addEventListener('keydown', (event) => console.log(`page: keydown ${event.code} on ${event.target.id || event.target.nodeName}`), true);
    window.openfrenzyAudioLevel = () => {
      let peak = 0, sum = 0, count = 0, state = 'none';
      for (const analyser of window.openfrenzyAudioTaps) {
        const data = new Float32Array(analyser.fftSize);
        analyser.getFloatTimeDomainData(data);
        state = analyser.context.state;
        for (const v of data) { peak = Math.max(peak, Math.abs(v)); sum += v * v; count++; }
      }
      return { peak, rms: count ? Math.sqrt(sum / count) : 0, state, taps: window.openfrenzyAudioTaps.length };
    };
  });
  page.on('console', (message) => record(message.text()));
  page.on('pageerror', (error) => record('pageerror: ' + error.message));
  page.on('crash', () => record('driver: the page crashed'));
  // --set NAME=value: ?set=; --query WORD: a page parameter of its own (asyncify, for example)
  const query = [...opts.set.map((value) => 'set=' + encodeURIComponent(value)), ...opts.query].join('&');
  const base = opts.url || `http://127.0.0.1:${opts.port}/`;
  await page.goto(query ? base + (base.includes('?') ? '&' : '?') + query : base);
  // (a hosted page may reload itself once to become isolated: wait across that navigation)
  const readyBy = Date.now() + 60000;
  for (;;) {
    try {
      await page.waitForFunction(() => window.openfrenzyReady === true && window.crossOriginIsolated !== false,
        null, { timeout: Math.max(1000, readyBy - Date.now()) });
      await sleep(1000);
      if (await page.evaluate(() => window.openfrenzyReady === true && window.crossOriginIsolated !== false)) break;
    } catch (error) {
      if (Date.now() >= readyBy) throw error;
      await sleep(500);
    }
  }
  record('driver: crossOriginIsolated ' + await page.evaluate(() => window.crossOriginIsolated));
  const checks = await page.$$eval('#checks li', (items) => items.map((item) => `${item.className}: ${item.textContent}`));
  record('checks: ' + checks.join(' | '));
  let data = await page.textContent('#data-status');
  record('data: ' + data);
  if (!/ready/.test(data)) {
    if (!opts.iso) throw new Error('the page has no disc image and --iso is not given');
    const started = Date.now();
    await page.setInputFiles('#image-input', path.resolve(opts.iso));
    await page.waitForFunction(() => /ready/.test(document.querySelector('#data-status').textContent) ||
      /not an Xbox|failed|full/.test(document.querySelector('#import-text').textContent), null, { timeout: 600000 });
    record(`import: ${await page.textContent('#import-text')} (${((Date.now() - started) / 1000).toFixed(1)} s)`);
    data = await page.textContent('#data-status');
    record('data: ' + data);
    if (!/ready/.test(data)) throw new Error('the disc image was not copied');
  }
  if (opts.pads) {
    await page.evaluate((count) => window.openfrenzyPadInstall(count), opts.pads);
    record(`driver: ${opts.pads} scripted pads connected`);
  }
  await page.click('#play');
  const start = Date.now();
  // --mark RE: the time of the first console line that matches RE is the mark; a --shots or
  // --pad time "mSECONDS" is SECONDS after the mark (never, if no line matches)
  let markAt = null;
  let markScanned = 0;
  const parseTime = (text) => text.startsWith('m') ? { at: Number(text.slice(1)), relative: true } : { at: Number(text), relative: false };
  const due = (item) => item.relative ? (markAt === null ? Infinity : markAt + item.at) : item.at;
  // --pad "SECONDS:[PAD/]BUTTON[+HOLD_MS],..." (any number): PAD 0-3 (default 0); a press is
  // released HOLD_MS later (default 150) without holding up the rest. BUTTON "unplug" / "plug"
  // unplugs the pad / plugs it in again.
  const padItems = opts.pad.flatMap((script) => script.split(',')).map((item) => {
    const [when, what] = item.split(':');
    const [target, hold] = what.split('+');
    const [pad, button] = target.includes('/') ? target.split('/') : ['0', target];
    return { ...parseTime(when), pad: Number(pad), button: /plug/.test(button) ? button : Number(button), hold: Number(hold || 150) };
  });
  const padReleases = [];
  const shots = opts.shots.map(parseTime);
  let keyIndex = 0;
  while ((Date.now() - start) / 1000 < opts.seconds) {
    const elapsed = (Date.now() - start) / 1000;
    if (opts.mark && markAt === null) {
      const pattern = new RegExp(opts.mark);
      for (; markScanned < lines.length && markAt === null; markScanned++) {
        if (pattern.test(lines[markScanned]) && !lines[markScanned].includes('driver:')) markAt = elapsed;
      }
      if (markAt !== null) record(`driver: mark /${opts.mark}/ at ${elapsed.toFixed(1)} s`);
    }
    for (const shot of shots.filter((item) => due(item) <= elapsed).sort((a, b) => due(a) - due(b))) {
      shots.splice(shots.indexOf(shot), 1);
      const name = (shot.relative ? 'm' : '') + String(shot.at).padStart(shot.relative ? 3 : 4, '0');
      await page.screenshot({ path: path.join(opts.out, `shot-${name}.png`) });
      const fps = await page.textContent('#fps');
      record(`driver: screenshot ${name} at ${elapsed.toFixed(1)} s (${fps})`);
    }
    // --keys "SECONDS:KEY[+HOLD_MS],..." presses keys through the page (real DOM events)
    for (const script of opts.keys) {
      const items = script.split(',');
      while (keyIndex < items.length) {
        const [when, what] = items[keyIndex].split(':');
        if (Number(when) > elapsed) break;
        const [key, hold] = what.split('+');
        await page.keyboard.down(key);
        await sleep(Number(hold || 120));
        await page.keyboard.up(key);
        record(`driver: key ${key} at ${elapsed.toFixed(1)} s`);
        keyIndex++;
      }
    }
    if (Math.floor(elapsed / 5) !== Math.floor((elapsed - 0.3) / 5)) {
      const level = await page.evaluate(() => window.openfrenzyAudioLevel());
      record(`audio: peak ${level.peak.toFixed(3)} rms ${level.rms.toFixed(4)}, context ${level.state}, ${level.taps} taps`);
    }
    // --pad: standard button indices on the scripted pads
    for (const item of padItems.filter((entry) => due(entry) <= elapsed).sort((a, b) => due(a) - due(b))) {
      padItems.splice(padItems.indexOf(item), 1);
      const { pad, button, hold } = item;
      if (typeof button === 'string') {
        const done = await page.evaluate(([p, c]) => window.openfrenzyPadPlug(p, c), [pad, button === 'plug']);
        record(`driver: pad ${pad} ${button} at ${elapsed.toFixed(1)} s${done ? '' : ' (nothing to do)'}`);
      } else if (await page.evaluate(([p, b]) => window.openfrenzyPad(p, b, true), [pad, button])) {
        padReleases.push({ at: Date.now() + hold, pad, button });
        record(`driver: pad ${pad} button ${button} at ${elapsed.toFixed(1)} s`);
      } else {
        record(`driver: pad ${pad} button ${button} at ${elapsed.toFixed(1)} s: pad ${pad} is not connected`);
      }
    }
    for (let i = padReleases.length - 1; i >= 0; i--) {
      if (padReleases[i].at > Date.now()) continue;
      const { pad, button } = padReleases.splice(i, 1)[0];
      await page.evaluate(([p, b]) => window.openfrenzyPad(p, b, false), [pad, button]);
    }
    const fatal = await page.evaluate(() => window.openfrenzyFatal || null);
    if (fatal) {
      record('driver: the game stopped: ' + fatal);
      await page.screenshot({ path: path.join(opts.out, 'fatal.png') });
      status = 1;
      break;
    }
    // (sooner when a pad press is due for release)
    await sleep(Math.max(10, Math.min(250, ...padReleases.map((r) => r.at - Date.now()))));
  }
  for (const shot of shots) record(`driver: screenshot at ${shot.relative ? 'm' : ''}${shot.at} s not taken (the run ended first)`);
  for (const item of padItems) record(`driver: pad ${item.pad} ${item.button} at ${item.relative ? 'm' : ''}${item.at} s not done (the run ended first)`);
  // what the site keeps in its private storage (OPFS), for the saves check
  const files = await page.evaluate(async () => {
    const out = [];
    async function walk(directory, prefix) {
      for await (const [name, handle] of directory.entries()) {
        if (handle.kind === 'directory') await walk(handle, prefix + name + '/');
        else {
          let size = '?';
          try { size = (await handle.getFile()).size; } catch (e) { size = 'open'; }
          out.push(prefix + name + ' ' + size);
        }
      }
    }
    await walk(await navigator.storage.getDirectory(), '');
    return out;
  });
  record('storage: ' + files.join(', '));
  // --pull DIR: copy the files of an OPFS folder (the game's own screenshots, for example
  // OPENFRENZY_DEBUG_SCREENSHOT_DIRECTORY=/data/saves/shots) into the run's folder
  if (opts.pull) {
    const pulled = await page.evaluate(async (folder) => {
      let directory = await navigator.storage.getDirectory();
      for (const part of folder.split('/').filter(Boolean)) directory = await directory.getDirectoryHandle(part);
      const out = [];
      for await (const [name, handle] of directory.entries()) {
        if (handle.kind !== 'file') continue;
        const bytes = new Uint8Array(await (await handle.getFile()).arrayBuffer());
        let text = '';
        for (let i = 0; i < bytes.length; i += 0x8000) text += String.fromCharCode(...bytes.subarray(i, i + 0x8000));
        out.push([name, btoa(text)]);
      }
      return out;
    }, opts.pull).catch((error) => { record('driver: pull failed: ' + error); return []; });
    for (const [name, data] of pulled) fs.writeFileSync(path.join(opts.out, name), Buffer.from(data, 'base64'));
    record(`driver: pulled ${pulled.length} files from ${opts.pull}`);
  }
  // --expect PATTERN (any number): each must match a line, in the order given;
  // --forbid PATTERN (any number): none may match a line of the game's
  let from = 0;
  for (const expected of opts.expect) {
    const pattern = new RegExp(expected);
    const index = lines.findIndex((line, i) => i >= from && pattern.test(line));
    if (index < 0) {
      record(`driver: expected a line matching /${expected}/ (after line ${from}), found none`);
      status = 1;
    } else {
      record(`driver: found /${expected}/ at line ${index}`);
      from = index + 1;
    }
  }
  // --min-fps N: the seconds the game reports (web_frame.c, OPENFRENZY_WEB_FRAME_LOG=1), less
  // the first 5 (start-up), are at N fps or more for 90 % of them
  if (opts.minFps) {
    const seconds = lines.map((line) => /web frame: (\d+) fps/.exec(line)).filter(Boolean).map((m) => Number(m[1])).slice(5);
    const good = seconds.filter((fps) => fps >= opts.minFps).length;
    const text = `${good} of ${seconds.length} seconds at ${opts.minFps} fps or more`;
    if (!seconds.length || good < 0.9 * seconds.length) {
      record('driver: too slow: ' + text);
      status = 1;
    } else {
      record('driver: frame rate: ' + text);
    }
  }
  for (const forbidden of opts.forbid) {
    const pattern = new RegExp(forbidden);
    const hit = lines.find((line) => pattern.test(line) && !line.includes('driver:'));
    if (hit) {
      record(`driver: forbidden /${forbidden}/: ${hit}`);
      status = 1;
    }
  }
} catch (error) {
  record('driver error: ' + (error && error.stack ? error.stack : error));
  status = 2;
} finally {
  clearTimeout(hardLimit);
  if (context) await context.close().catch(() => {});
  if (server) server.kill();
  consoleFile.end();
}
console.log(`drive: status ${status}, ${lines.length} console lines in ${consolePath}`);
process.exit(status);
