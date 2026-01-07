'use strict';

const fs = require('node:fs');
const path = require('node:path');

function normalizeDir(candidate) {
  if (!candidate) return null;
  const resolved = path.resolve(candidate);
  return fs.existsSync(resolved) ? resolved : null;
}

function ensureVtkDllsOnPath() {
  const projectRoot = path.resolve(__dirname, '..');
  const candidates = [
    process.env.VTK_BIN_DIR,
    process.env.VTK_BIN_PATH,
    path.join(projectRoot, 'vtk-bin'),
    path.join(projectRoot, '..', 'vtk', 'VTK-9.5.2', 'build', 'bin', 'Release'),
  ];

  const delimiter = path.delimiter;
  const existing = new Set(
    (process.env.PATH || process.env.Path || '')
      .split(delimiter)
      .filter(Boolean)
      .map((entry) => path.resolve(entry).toLowerCase())
  );

  const appended = [];
  for (const raw of candidates) {
    const valid = normalizeDir(raw);
    if (!valid) continue;
    const lowered = valid.toLowerCase();
    if (!existing.has(lowered)) {
      appended.push(valid);
      existing.add(lowered);
    }
  }

  if (appended.length) {
    const updated = `${appended.join(delimiter)}${delimiter}${process.env.PATH || ''}`;
    process.env.PATH = updated;
    process.env.Path = updated;
    process.env.path = updated;
    console.info('[test-dicom-load] Added directories to PATH:', appended.join(', '));
  }
}

function collectDicomFiles(rootDir) {
  const stack = [rootDir];
  const files = [];

  while (stack.length) {
    const current = stack.pop();
    const entries = fs.readdirSync(current, { withFileTypes: true });
    for (const entry of entries) {
      const absolute = path.join(current, entry.name);
      if (entry.isDirectory()) {
        stack.push(absolute);
      } else if (entry.isFile()) {
        files.push({ name: entry.name, path: absolute });
      }
    }
  }

  return files;
}

function main() {
  const targetDir = process.argv[2] ? path.resolve(process.argv[2]) : 'D:/Scripts/Example/肺部';
  if (!fs.existsSync(targetDir) || !fs.statSync(targetDir).isDirectory()) {
    console.error('[test-dicom-load] Provided path is not a directory:', targetDir);
    process.exit(1);
  }

  const dicomFiles = collectDicomFiles(targetDir);
  if (!dicomFiles.length) {
    console.error('[test-dicom-load] No files found under directory:', targetDir);
    process.exit(1);
  }

  console.info('[test-dicom-load] Found DICOM candidates:', dicomFiles.length);
  for (const preview of dicomFiles.slice(0, 5)) {
    console.info('  -', preview.path);
  }

  ensureVtkDllsOnPath();

  let dicomModule;
  try {
    dicomModule = require('../native/dicom');
  } catch (error) {
    console.error('[test-dicom-load] Failed to load native module:', error);
    process.exit(1);
  }

  if (typeof dicomModule.loadSeries !== 'function') {
    console.error('[test-dicom-load] Native module does not expose loadSeries');
    process.exit(1);
  }

  try {
    const result = dicomModule.loadSeries({ files: dicomFiles });
  console.log('[test-dicom-load] Native loadSeries result:\n', JSON.stringify(result, null, 2));
  } catch (error) {
    console.error('[test-dicom-load] loadSeries threw an error:', error);
    process.exit(1);
  }
}

main();
