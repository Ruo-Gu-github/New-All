const fs = require("node:fs");
const path = require("node:path");

function fail(message) {
  console.error(`[sync-console-dlls] ${message}`);
  process.exit(1);
}

const projectRoot = path.resolve(__dirname, "..");
const srcDir = path.resolve(projectRoot, "..", "ConsoleDllTest", "Dlls", "bin");
const dstDir = path.resolve(
  projectRoot,
  "native",
  "console-dll",
  "build",
  "Release"
);

if (!fs.existsSync(srcDir)) {
  fail(`Source DLL dir not found: ${srcDir}`);
}

fs.mkdirSync(dstDir, { recursive: true });

const entries = fs.readdirSync(srcDir, { withFileTypes: true });
const dlls = entries
  .filter((e) => e.isFile())
  .map((e) => e.name)
  .filter((name) => name.toLowerCase().endsWith(".dll"));

if (dlls.length === 0) {
  fail(`No .dll files found in: ${srcDir}`);
}

let copied = 0;
let skipped = 0;
const skippedFiles = [];
for (const name of dlls) {
  const from = path.join(srcDir, name);
  const to = path.join(dstDir, name);
  try {
    fs.copyFileSync(from, to);
    copied++;
  } catch (err) {
    // Electron/Node can keep DLLs locked on Windows. Don't fail the whole sync;
    // copy what we can and report what was skipped.
    if (err && (err.code === "EBUSY" || err.code === "EPERM")) {
      skipped++;
      skippedFiles.push(name);
      continue;
    }
    throw err;
  }
}

console.log(
  `[sync-console-dlls] Copied ${copied} DLL(s) from ${srcDir} -> ${dstDir}`
);
if (skipped > 0) {
  console.warn(
    `[sync-console-dlls] Skipped ${skipped} locked DLL(s): ${skippedFiles.join(
      ", "
    )}`
  );
}
