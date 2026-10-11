// DRIVE.MJS: runs the web build in a browser and records what it does (docs/WEB.md "Tests").
//
//   node port/web/tests/drive.mjs --site build-web/site --iso orig/game.xiso.iso \
//        --seconds 60 --shots 10,30,60 --out /tmp/run [--set NAME=value ...] [--keys "..."]
//        [--browser chrome|firefox|webkit] [--headed]
//
// It serves the site itself (scripts/web_serve.py on 127.0.0.1, a fixed port so that the
// browser's storage, which belongs to the origin, is found again), opens it in a browser with
// a profile kept in tools/web-test/profile-<browser> (so the disc image is copied once), copies
// the disc image in if the page has none, starts the game and waits. Every console line goes
// to <out>/console.log; screenshots of the page are <out>/shot-<seconds>.png. The browser, the
// server and this script stop after --seconds (and a hard limit of --seconds + 600 s for the
// copy and the start).
//
// Exit status: 0, or 1 if the game stopped (the page's fatal panel), or 2 on a driver error.
// Playwright is not in the repository: install it into tools/web-test (git-ignored):
//   mkdir -p tools/web-test && cd tools/web-test && npm init -y && npm install playwright@1.56.1

import { spawn } from 'node:child_process';
import { createRequire } from 'node:module';
import fs from 'node:fs';
import path from 'node:path';
import { setTimeout as sleep } from 'node:timers/promises';

const root = path.resolve(path.dirname(new URL(import.meta.url).pathname), '../../..');
const require = createRequire(path.join(root, 'tools/web-test/package.json'));
const playwright = require('playwright');

function options(argv) {
  const out = { site: 'build-web/site', seconds: 60, shots: [], set: [], keys: [], browser: 'chrome', port: 8765, headed: false };
  for (let i = 0; i < argv.length; i++) {
    const name = argv[i];
    const value = () => argv[++i];
    if (name === '--site') out.site = value();
    else if (name === '--iso') out.iso = value();
    else if (name === '--seconds') out.seconds = Number(value());
    else if (name === '--shots') out.shots = value().split(',').map(Number);
    else if (name === '--out') out.out = value();
    else if (name === '--set') out.set.push(value());
    else if (name === '--keys') out.keys.push(value());
    else if (name === '--browser') out.browser = value();
    else if (name === '--port') out.port = Number(value());
    else if (name === '--headed') out.headed = true;
    else if (name === '--expect') out.expect = value();
    else throw new Error('unknown option ' + name);
  }
  if (!out.out) throw new Error('--out is needed');
  return out;
}

const opts = options(process.argv.slice(2));
fs.mkdirSync(opts.out, { recursive: true });
const consolePath = path.join(opts.out, 'console.log');
const consoleFile = fs.createWriteStream(consolePath);
const lines = [];
function record(text) {
  const line = `[${(performance.now() / 1000).toFixed(1)}] ${text}`;
  lines.push(line);
  consoleFile.write(line + '\n');
}

const server = spawn('python3', ['-I', path.join(root, 'scripts/web_serve.py'), path.resolve(opts.site),
  '--port', String(opts.port), '--timeout', String(opts.seconds + 900)],
  { stdio: ['ignore', 'inherit', 'inherit'], env: { ...process.env, WEB_SERVE_QUIET: '1' } });

let context;
let status = 0;
const hardLimit = setTimeout(() => {
  record('driver: hard time limit reached');
  if (context) context.close().catch(() => {});
  server.kill('SIGKILL');
  process.exit(2);
}, (opts.seconds + 900) * 1000);

try {
  await sleep(800);
  const kind = opts.browser === 'firefox' ? 'firefox' : opts.browser === 'webkit' ? 'webkit' : 'chromium';
  const launch = {
    headless: !opts.headed,
    viewport: { width: 1280, height: 960 },
  };
  if (kind === 'chromium') {
    launch.channel = 'chrome';
    launch.args = ['--ignore-gpu-blocklist', '--autoplay-policy=no-user-gesture-required'];
  }
  context = await playwright[kind].launchPersistentContext(path.join(root, `tools/web-test/profile-${opts.browser}`), launch);
  const page = context.pages()[0] || await context.newPage();
  page.on('console', (message) => record(message.text()));
  page.on('pageerror', (error) => record('pageerror: ' + error.message));
  page.on('crash', () => record('driver: the page crashed'));
  const query = opts.set.map((value) => 'set=' + encodeURIComponent(value)).join('&');
  await page.goto(`http://127.0.0.1:${opts.port}/${query ? '?' + query : ''}`);
  await page.waitForFunction(() => window.openfrenzyReady === true, null, { timeout: 60000 });
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
  await page.click('#play');
  const start = Date.now();
  const shots = [...opts.shots].sort((a, b) => a - b);
  let keyIndex = 0;
  while ((Date.now() - start) / 1000 < opts.seconds) {
    const elapsed = (Date.now() - start) / 1000;
    if (shots.length && elapsed >= shots[0]) {
      const at = shots.shift();
      await page.screenshot({ path: path.join(opts.out, `shot-${String(at).padStart(4, '0')}.png`) });
      const fps = await page.textContent('#fps');
      record(`driver: screenshot at ${at} s (${fps})`);
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
    const fatal = await page.evaluate(() => window.openfrenzyFatal || null);
    if (fatal) {
      record('driver: the game stopped: ' + fatal);
      await page.screenshot({ path: path.join(opts.out, 'fatal.png') });
      status = 1;
      break;
    }
    await sleep(250);
  }
  for (const at of shots) record(`driver: screenshot at ${at} s not taken (the run ended first)`);
  if (opts.expect) {
    const pattern = new RegExp(opts.expect);
    if (!lines.some((line) => pattern.test(line))) {
      record(`driver: expected a line matching /${opts.expect}/, found none`);
      status = 1;
    } else {
      record(`driver: found /${opts.expect}/`);
    }
  }
} catch (error) {
  record('driver error: ' + (error && error.stack ? error.stack : error));
  status = 2;
} finally {
  clearTimeout(hardLimit);
  if (context) await context.close().catch(() => {});
  server.kill();
  consoleFile.end();
}
console.log(`drive: status ${status}, ${lines.length} console lines in ${consolePath}`);
process.exit(status);
