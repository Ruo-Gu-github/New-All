const fs = require("node:fs");
const path = require("node:path");

function log(...args) {
  console.log("[patch-node-gyp-vs2026]", ...args);
}

function patchFile(filePath) {
  let text = fs.readFileSync(filePath, "utf8");

  // Defensive cleanup: some earlier patch attempts could leave stray lines that
  // break JS parsing (e.g. `ret.versionYear = 2026` outside any block).
  text = text.replace(
    /\nret\.versionYear = 2026\s*\n\s*return ret\s*\n\s*}\s*\n/g,
    "\n"
  );

  // 1) Ensure VS2026 mapping exists (18.x).
  //    PLUS: if user requested msvs_version=2019, treat VS18 as 2019 so
  //    node-gyp generates v142 projects (compatible with VS2019-built libs).

  // Remove any existing VS18 mapping block first (so we can insert a clean one).
  text = text.replace(
    /\n\s*\/\/ Visual Studio 2026[\s\S]*?\n\s*if \(ret\.versionMajor === 18\) \{[\s\S]*?\n\s*}\s*\n/g,
    "\n"
  );
  text = text.replace(
    /\n\s*if \(ret\.versionMajor === 18\) \{[\s\S]*?\n\s*}\s*\n/g,
    "\n"
  );

  // Insert our clean VS18 mapping right after VS17 mapping.
  text = text.replace(
    /if \(ret\.versionMajor === 17\) \{\s*ret\.versionYear = 2022\s*return ret\s*}\s*/,
    (m) =>
      `${m}\n\n    // Visual Studio 2026 reports major version 18.x.\n    // If user asks for VS2019 (msvs_version=2019), force v142 toolset generation\n    // even when the host IDE is VS18.x (as long as v142 toolset is installed).\n    if (ret.versionMajor === 18) {\n      if (this.configVersionYear === 2019) {\n        ret.versionYear = 2019\n        return ret\n      }\n      ret.versionYear = 2026\n      return ret\n    }\n`
  );

  // 2) Allow supportedYears to include 2026 (Node>=22 path uses [2019, 2022])
  text = text.replace(/\[2019, 2022\]/g, "[2019, 2022, 2026]");

  // 3) Toolset mapping for VS2026
  //
  // IMPORTANT:
  // "PlatformToolset" names (v142/v143/...) do NOT necessarily match the
  // Visual Studio major version (18.x). On current installations, the MSVC
  // compiler versions 14.4x/14.5x are still built via the v143 toolset.
  // Mapping VS2026 to a non-existent toolset (e.g. v180) will trigger MSB8020.
  // If an older patch already injected a VS2026 mapping, normalize it.
  text = text.replace(
    /else\s+if\s*\(versionYear\s*===\s*2026\)\s*\{[\s\S]*?return\s*['"]v180['"][\s\S]*?\}/g,
    "else if (versionYear === 2026) {\n      return 'v143'\n    }"
  );

  if (!text.includes("versionYear === 2026")) {
    text = text.replace(
      /} else if \(versionYear === 2022\) \{\s*return 'v143'\s*}\s*/,
      (m) =>
        `${m}    } else if (versionYear === 2026) {\n      return 'v143'\n    }\n`
    );
  }

  // Final cleanup: ensure we didn't introduce the known stray pattern again.
  text = text.replace(
    /\nret\.versionYear = 2026\s*\n\s*return ret\s*\n\s*}\s*\n/g,
    "\n"
  );

  fs.writeFileSync(filePath, text, "utf8");
}

const root = path.resolve(__dirname, "..");
const target = path.join(
  root,
  "node_modules",
  "node-gyp",
  "lib",
  "find-visualstudio.js"
);

if (!fs.existsSync(target)) {
  log("node-gyp not found at", target);
  process.exit(0);
}

try {
  patchFile(target);
  log("patched", target);
} catch (e) {
  log("failed:", e && e.message ? e.message : String(e));
  process.exit(1);
}
