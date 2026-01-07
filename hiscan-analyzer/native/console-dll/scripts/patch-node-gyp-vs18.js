const fs = require("fs");
const path = require("path");

function patchFindVisualStudio(filePath) {
  if (!fs.existsSync(filePath)) {
    console.log(`[patch-node-gyp-vs18] skip: not found: ${filePath}`);
    return;
  }

  const original = fs.readFileSync(filePath, "utf8");

  // Already patched?
  if (original.includes("ret.versionMajor === 18")) {
    console.log(`[patch-node-gyp-vs18] ok: already patched: ${filePath}`);
    return;
  }

  const needle =
    "if (ret.versionMajor === 17) {\n      ret.versionYear = 2022\n      return ret\n    }";
  const replacement =
    needle +
    "\n    // VS 2026 reports major version 18.x. node-gyp 10.x doesn't recognize it\n" +
    "    // yet; treat it as VS2022-compatible so we can use the installed MSVC.\n" +
    "    if (ret.versionMajor === 18) {\n" +
    "      ret.versionYear = 2022\n" +
    "      return ret\n" +
    "    }";

  if (!original.includes(needle)) {
    console.warn(
      `[patch-node-gyp-vs18] warn: signature not found; no patch applied: ${filePath}`
    );
    return;
  }

  const updated = original.replace(needle, replacement);
  fs.writeFileSync(filePath, updated, "utf8");
  console.log(`[patch-node-gyp-vs18] patched: ${filePath}`);
}

const repoRoot = __dirname; // scripts/
const target = path.resolve(
  repoRoot,
  "..",
  "node_modules",
  "node-gyp",
  "lib",
  "find-visualstudio.js"
);

patchFindVisualStudio(target);
