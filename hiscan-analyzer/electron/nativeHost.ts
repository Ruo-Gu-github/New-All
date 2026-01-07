import { utilityProcess, type UtilityProcess } from "electron";
import path from "node:path";

export type NativeHostError = {
  message: string;
  stack?: string;
};

type PendingCall = {
  resolve: (value: any) => void;
  reject: (reason: any) => void;
  method: string;
};

export class NativeHost {
  private proc: UtilityProcess | null = null;
  private nextId = 1;
  private pending = new Map<number, PendingCall>();
  private progress = new Map<
    number,
    (progress: number, message: string) => void
  >();
  private restarting = false;
  private listeners = new Set<() => void>();

  constructor(private workerModulePath: string, private addonPath: string) {}

  onRestarted(cb: () => void) {
    this.listeners.add(cb);
    return () => this.listeners.delete(cb);
  }

  private emitRestarted() {
    for (const cb of Array.from(this.listeners)) {
      try {
        cb();
      } catch {
        // ignore
      }
    }
  }

  private spawn() {
    // UtilityProcess doesn't expose a stable "killed" flag in typings.
    // We treat a non-null proc as alive; when it exits we set it back to null.
    if (this.proc) return;

    const args = [`--addonPath=${this.addonPath}`];
    const proc = utilityProcess.fork(this.workerModulePath, args, {
      serviceName: "hiscan-native-host",
    });

    this.proc = proc;

    proc.on("message", (msg: any) => {
      if (!msg || typeof msg !== "object") return;

      if (msg.type === "result") {
        const id = Number(msg.id);
        const pending = this.pending.get(id);
        if (!pending) return;
        this.pending.delete(id);
        this.progress.delete(id);

        if (msg.ok) pending.resolve(msg.result);
        else pending.reject(msg.error ?? { message: "Native call failed" });
        return;
      }

      if (msg.type === "progress") {
        const id = Number(msg.id);
        const cb = this.progress.get(id);
        if (!cb) return;
        try {
          cb(Number(msg.progress) || 0, String(msg.message ?? ""));
        } catch {
          // ignore
        }
        return;
      }

      if (msg.type === "fatal") {
        // Worker couldn't start or load the addon.
        const error = msg.error ?? { message: "Native host fatal error" };
        this.failAllPending(error);
        this.restartSoon();
        return;
      }
    });

    const onExitOrError = (reason: any) => {
      this.proc = null;
      this.failAllPending({ message: "Native host exited", details: reason });
      this.restartSoon();
    };

    proc.on("exit", onExitOrError);
    // Some Electron versions emit an 'error' event on the underlying child
    // process, but it isn't part of the UtilityProcess typings.
    (proc as any).on?.("error", onExitOrError);
  }

  private failAllPending(error: any) {
    const entries = Array.from(this.pending.entries());
    this.pending.clear();
    this.progress.clear();
    for (const [, p] of entries) {
      try {
        p.reject(error);
      } catch {
        // ignore
      }
    }
  }

  private restartSoon() {
    if (this.restarting) return;
    this.restarting = true;
    setTimeout(() => {
      this.restarting = false;
      try {
        this.spawn();
      } finally {
        this.emitRestarted();
      }
    }, 300);
  }

  invoke(method: string, ...args: any[]) {
    this.spawn();
    if (!this.proc)
      return Promise.reject({ message: "Native host unavailable" });

    const id = this.nextId++;
    return new Promise<any>((resolve, reject) => {
      this.pending.set(id, { resolve, reject, method });
      try {
        this.proc?.postMessage({ type: "invoke", id, method, args });
      } catch (e: any) {
        this.pending.delete(id);
        reject({ message: e?.message ?? String(e) });
      }
    });
  }

  invokeWithProgress(
    method: string,
    onProgress: (progress: number, message: string) => void,
    ...args: any[]
  ) {
    this.spawn();
    if (!this.proc)
      return Promise.reject({ message: "Native host unavailable" });

    const id = this.nextId++;
    return new Promise<any>((resolve, reject) => {
      this.pending.set(id, { resolve, reject, method });
      this.progress.set(id, onProgress);
      try {
        this.proc?.postMessage({
          type: "invoke",
          id,
          method,
          args,
          wantsProgress: true,
        });
      } catch (e: any) {
        this.pending.delete(id);
        this.progress.delete(id);
        reject({ message: e?.message ?? String(e) });
      }
    });
  }

  dispose() {
    try {
      this.failAllPending({ message: "Native host disposed" });
      this.proc?.kill();
    } finally {
      this.proc = null;
    }
  }
}

export function resolveNativeHostWorkerPath(IS_DEV: boolean) {
  // We ship the worker as a static file under renderer dist (from public/).
  // - Dev: <repo>/public/native-host-worker.mjs
  // - Prod: <app>/dist/native-host-worker.mjs (dist is inside app.asar)
  const appRoot = process.env.APP_ROOT || process.cwd();
  if (IS_DEV) {
    return path.join(appRoot, "public", "native-host-worker.mjs");
  }
  return path.join(appRoot, "dist", "native-host-worker.mjs");
}
