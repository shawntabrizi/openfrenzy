/*
IMPORT-WORKER.JS

Copies the player's disc image into the site's Origin Private File System as
game.xiso.iso, where the game reads it (/data/game.xiso.iso: port/web/src/web_main.c).
It runs on a worker for createSyncAccessHandle, which writes at any offset without a copy
of the file in memory.

- The image is checked first: an XDVDFS volume ("MICROSOFT*XBOX*MEDIA" at 0x10000 of the game
  partition, at offset 0 or at one of the whole-disc offsets, as src/vfs/xiso_fs.c reads it).
- Blocks of zeros (the disc's padding files, about half of a full image) are not written:
  the copy leaves them to the file's growth, which reads back as zeros.
- game.json is written last, so a copy that stopped part way is not taken for a whole one.

Message in: { file }. Messages out: { type: 'progress', done, total }, { type: 'done', bytes },
{ type: 'error', message }.
*/

'use strict';

const CHUNK = 8 * 1024 * 1024;
const MAGIC = 'MICROSOFT*XBOX*MEDIA';
// a plain image, and whole-disc images (extract-xiso's XGD1, XGD2, XGD3 offsets)
const PARTITIONS = [0, 0x18300000, 0x0FD90000, 0x02080000];

async function findPartition(file) {
  for (const offset of PARTITIONS) {
    const at = offset + 0x10000;
    if (at + MAGIC.length > file.size) continue;
    const bytes = new Uint8Array(await file.slice(at, at + MAGIC.length).arrayBuffer());
    if (String.fromCharCode(...bytes) === MAGIC) return offset;
  }
  return -1;
}

function allZero(bytes) {
  const words = new Uint32Array(bytes.buffer, bytes.byteOffset, bytes.byteLength >>> 2);
  for (let i = 0; i < words.length; i++) if (words[i]) return false;
  for (let i = words.length << 2; i < bytes.length; i++) if (bytes[i]) return false;
  return true;
}

async function copy(file) {
  const partition = await findPartition(file);
  if (partition < 0) {
    throw new Error('This file is not an Xbox disc image (no XDVDFS volume found). ' +
      'Choose an .iso or .xiso image of Fuzion Frenzy.');
  }
  const root = await navigator.storage.getDirectory();
  await root.removeEntry('game.json').catch(() => {});
  await root.removeEntry('game.xiso.iso').catch(() => {});
  const handle = await root.getFileHandle('game.xiso.iso', { create: true });
  const access = await handle.createSyncAccessHandle();
  let skipped = 0;
  try {
    for (let offset = 0; offset < file.size; offset += CHUNK) {
      const bytes = new Uint8Array(await file.slice(offset, Math.min(offset + CHUNK, file.size)).arrayBuffer());
      if (allZero(bytes)) {
        skipped += bytes.length;
      } else {
        const written = access.write(bytes, { at: offset });
        if (written !== bytes.length) throw new Error('The browser storage is full (wrote ' + written + ' of ' + bytes.length + ' bytes).');
      }
      postMessage({ type: 'progress', done: Math.min(offset + CHUNK, file.size), total: file.size });
    }
    access.truncate(file.size);
    access.flush();
  } finally {
    access.close();
  }
  const marker = await (await root.getFileHandle('game.json', { create: true })).createSyncAccessHandle();
  try {
    const text = new TextEncoder().encode(JSON.stringify({
      name: file.name, size: file.size, partition, zero_bytes_skipped: skipped, copied: new Date().toISOString(),
    }));
    marker.truncate(0);
    marker.write(text, { at: 0 });
    marker.flush();
  } finally {
    marker.close();
  }
  return file.size;
}

onmessage = async (event) => {
  try {
    const bytes = await copy(event.data.file);
    postMessage({ type: 'done', bytes });
  } catch (error) {
    postMessage({ type: 'error', message: String(error && error.message ? error.message : error) });
  }
};
