const { spawnSync } = require("node:child_process");
const path = require("node:path");

function readVsInstallPath() {
  const vswhere = path.join(
    process.env["ProgramFiles(x86)"] || "C:\\Program Files (x86)",
    "Microsoft Visual Studio",
    "Installer",
    "vswhere.exe"
  );

  const r = spawnSync(vswhere, ["-all", "-products", "*", "-format", "json"], {
    encoding: "utf8",
    stdio: ["ignore", "pipe", "ignore"],
  });

  if (r.status !== 0 || !r.stdout) return null;
  try {
    const list = JSON.parse(r.stdout);
    if (!Array.isArray(list)) return null;
    const vs = list.find(
      (x) =>
        x &&
        typeof x.installationPath === "string" &&
        String(x.productId || "").includes("Microsoft.VisualStudio.Product")
    );
    return vs?.installationPath ?? null;
  } catch {
    return null;
  }
}

function fail(msg) {
  console.error(msg);
  process.exit(1);
}

let electronVersion;
try {
  electronVersion = require("electron/package.json").version;
} catch (e) {
  fail("[build-opengl-addon] Cannot resolve electron. Run npm install first.");
}

let nodeGyp;
try {
  nodeGyp = require.resolve("node-gyp/bin/node-gyp.js");
} catch (e) {
  fail(
    "[build-opengl-addon] Cannot resolve node-gyp. Ensure it is in devDependencies."
  );
}

const projectRoot = path.resolve(__dirname, "..");
const addonDir = path.join(projectRoot, "native", "opengl-child");
const buildDir = path.join(addonDir, "build");

const outputNodePath = path.join(buildDir, "Release", "opengl_child.node");

function buildWithMsvsVersion(msvsVersion) {
  const args = [
    nodeGyp,
    "rebuild",
    "-C",
    addonDir,
    `--msvs_version=${msvsVersion}`,
    `--target=${electronVersion}`,
    "--runtime=electron",
    "--dist-url=https://electronjs.org/headers",
    "--arch=x64",
  ];

  const env = { ...process.env };

  // Prefer a stable, explicit MSVC toolset. Some environments only have newer
  // toolsets installed; allow opt-in override and/or fallback below.
  env.GYP_MSVS_VERSION = String(msvsVersion);
  env.npm_config_msvs_version = String(msvsVersion);

  // Avoid Python default-encoding issues on Windows locales (GBK) when gyp reads
  // existing XML files.
  env.PYTHONUTF8 = "1";

  // Clean previous build outputs to avoid decoding existing .filters with a
  // different encoding.
  try {
    const fs = require("node:fs");
    if (fs.existsSync(buildDir)) {
      fs.rmSync(buildDir, { recursive: true, force: true });
      console.log("[build-opengl-addon] Cleaned:", buildDir);
    }

    // If the previous .node is still present, it is likely locked by a running
    // Electron/Node process. Fail early with a helpful message.
    if (fs.existsSync(outputNodePath)) {
      try {
        fs.unlinkSync(outputNodePath);
      } catch (e) {
        fail(
          `[build-opengl-addon] Cannot delete ${outputNodePath}. ` +
            "It is likely locked by a running Electron/Node process. " +
            "Close the app/dev server that loads the addon and retry."
        );
      }
    }
  } catch (e) {
    console.warn(
      "[build-opengl-addon] Warning: cleanup failed:",
      e?.message || e
    );
  }

  // Use the installed Visual Studio via override path. We patch node-gyp in
  // postinstall to recognize VS2026 (18.x) and map it to the v180 toolset.
  const vsPath = readVsInstallPath();
  if (vsPath) {
    env.GYP_MSVS_OVERRIDE_PATH = vsPath;
    console.log("[build-opengl-addon] Using VS override path:", vsPath);
  }

  console.log("[build-opengl-addon] Electron:", electronVersion);
  console.log(
    `[build-opengl-addon] Building with MSVC/VS version: ${msvsVersion}`
  );
  console.log(
    "[build-opengl-addon] Command:",
    process.execPath,
    args.join(" ")
  );

  return spawnSync(process.execPath, args, {
    cwd: projectRoot,
    stdio: "inherit",
    env,
  });
}

// Default to VS2019 (v142) for compatibility with prebuilt libs, but allow
// override via env and fall back automatically when v142 isn't installed.
//
// Notes:
// - Some machines may only have VS2026 (18.x) installed; our postinstall patch
//   makes node-gyp accept msvs_version=2026 and map to toolset v180.
// - Some machines also have VS2022 BuildTools, but without a Windows SDK.
const preferredVersion = process.env.HISCAN_MSVS_VERSION || "2019";
let r = buildWithMsvsVersion(preferredVersion);

if ((r.status ?? 1) !== 0 && preferredVersion === "2019") {
  console.warn(
    "[build-opengl-addon] VS2019 (v142) build failed. If you don't have v142 installed, install 'MSVC v142 build tools' via Visual Studio Installer. Attempting fallback to 2026 (VS 18.x) now..."
  );
  r = buildWithMsvsVersion("2026");
}

if ((r.status ?? 1) !== 0 && preferredVersion === "2019") {
  console.warn(
    "[build-opengl-addon] VS2026 fallback failed. Attempting fallback to 2022 now (requires Windows SDK installed in VS2022 Build Tools)..."
  );
  r = buildWithMsvsVersion("2022");
}

process.exit(r.status ?? 1);
