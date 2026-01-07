// Electron utilityProcess provides a MessagePort at process.parentPort.
// Keep a fallback for cases where this file is launched as a worker thread.
import { parentPort as workerThreadsParentPort } from "node:worker_threads";
const parentPort = process.parentPort ?? workerThreadsParentPort;
import path from "node:path";
import { createRequire } from "node:module";

function parseArgs(argv) {
  const out = {};
  for (const a of argv) {
    if (!a.startsWith("--")) continue;
    const eq = a.indexOf("=");
    if (eq === -1) out[a.slice(2)] = true;
    else out[a.slice(2, eq)] = a.slice(eq + 1);
  }
  return out;
}

const args = parseArgs(process.argv);
const addonPath = typeof args.addonPath === "string" ? args.addonPath : null;

if (!parentPort) {
  // If launched outside utilityProcess, we cannot communicate with the parent.
  // Exit fast to avoid a hung process.
  console.error(
    "[native-host-worker] parentPort missing (not a utilityProcess context)"
  );
  process.exit(1);
}

if (!addonPath) {
  parentPort.postMessage({
    type: "fatal",
    error: { message: "Missing --addonPath" },
  });
  // Hard-exit so the parent can restart/recover.
  process.exit(2);
}

// Ensure dependent DLLs can be resolved (Windows)
try {
  const dllDir = path.dirname(addonPath);
  const currentPath = process.env.PATH || "";
  if (!currentPath.split(";").includes(dllDir)) {
    process.env.PATH = `${dllDir};${currentPath}`;
  }
} catch {
  // ignore
}

let addon = null;
try {
  const require = createRequire(import.meta.url);
  addon = require(addonPath);
  parentPort.postMessage({ type: "ready" });
} catch (e) {
  parentPort.postMessage({
    type: "fatal",
    error: { message: e?.message ?? String(e), stack: e?.stack },
  });
  process.exit(3);
}

function serializeError(e) {
  return { message: e?.message ?? String(e), stack: e?.stack };
}

parentPort.on("message", async (evtOrMsg) => {
  let msg = null;
  try {
    msg = evtOrMsg?.data ?? evtOrMsg;
    if (!msg || typeof msg !== "object") return;
    if (msg.type !== "invoke") return;

    const { id, method, args: callArgs } = msg;
    if (!id) return;

    if (!addon || typeof addon[method] !== "function") {
      parentPort.postMessage({
        type: "result",
        id,
        ok: false,
        error: { message: `Function not found: ${String(method)}` },
      });
      return;
    }

    const args = Array.isArray(callArgs) ? callArgs : [];

    // Special-case long-running create* calls: progress callback cannot cross processes.
    if (
      msg.wantsProgress &&
      (method === "createAPRViews" || method === "createMPRViews")
    ) {
      if (args.length < 2) {
        parentPort.postMessage({
          type: "result",
          id,
          ok: false,
          error: { message: `Invalid args for ${method}` },
        });
        return;
      }
      const [sessionId, folderPath] = args;
      const result = addon[method](
        sessionId,
        folderPath,
        (progress, message) => {
          try {
            parentPort.postMessage({ type: "progress", id, progress, message });
          } catch {
            // ignore
          }
        }
      );
      const awaited =
        result && typeof result.then === "function" ? await result : result;
      parentPort.postMessage({ type: "result", id, ok: true, result: awaited });
      return;
    }

    // Many N-API exports are sync; allow both sync and promise return.
    const result = addon[method](...args);
    const awaited =
      result && typeof result.then === "function" ? await result : result;

    parentPort.postMessage({ type: "result", id, ok: true, result: awaited });
  } catch (e) {
    try {
      parentPort.postMessage({
        type: "result",
        id: msg?.id,
        ok: false,
        error: serializeError(e),
      });
    } finally {
      // If an exception happened inside the addon call, staying alive is still useful.
    }
  }
});
