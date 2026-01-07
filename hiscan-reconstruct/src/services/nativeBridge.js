const bridge = typeof window !== 'undefined' ? window.nativeBridge ?? {} : {};

const noopAsync = () => Promise.resolve();
const noop = () => {};

export function useWindowControls() {
  const controls = bridge.windowControls ?? {};
  return {
    minimize: () => (controls.minimize?.() ?? noopAsync()),
    toggleMaximize: () => (controls.toggleMaximize?.() ?? noopAsync()),
    close: () => (controls.close?.() ?? noopAsync()),
  };
}

export async function getNativeRecords(dbPath) {
  if (!bridge.native?.getRecords) {
    console.warn('[nativeBridge] getRecords unavailable, returning empty list');
    return [];
  }
  try {
    return await bridge.native.getRecords(dbPath);
  } catch (error) {
    console.error('[nativeBridge] Failed to load records', error);
    return [];
  }
}

export function runNativeJob(payload, onProgress = noop) {
  if (!bridge.native?.runJob) {
    console.warn('[nativeBridge] runJob unavailable, skipping');
    return Promise.resolve({ status: 'unavailable' });
  }
  return bridge.native.runJob(payload, onProgress);
}
