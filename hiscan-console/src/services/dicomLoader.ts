import { ElMessage } from 'element-plus';

export type DicomSeriesInfo = {
  sampleName: string;
  width: number;
  height: number;
  sliceCount: number;
  pixelSpacing?: [number, number];
  thumbnail: string;
};

export type NativeDicomResult = {
  sampleName?: string;
  width?: number;
  height?: number;
  sliceCount?: number;
  pixelSpacing?: [number, number];
  thumbnail?: string;
};

type NativeDicomBridge = {
  loadSeries: (payload: { files: Array<{ path?: string; name?: string }> }) => Promise<NativeDicomResult | null>;
};

function resolveNativeDicomBridge(): NativeDicomBridge | null {
  if (typeof window === 'undefined') return null;
  const bridge = (window as typeof window & { nativeBridge?: { dicom?: NativeDicomBridge } }).nativeBridge;
  return bridge?.dicom ?? null;
}

function toPlaceholder(sampleName: string, sliceCount: number): DicomSeriesInfo {
  const text = sampleName.slice(0, 12) || '未命名样本';
  const canvas = document.createElement('canvas');
  canvas.width = 256;
  canvas.height = 256;
  const ctx = canvas.getContext('2d');
  if (ctx) {
    ctx.fillStyle = '#0A1C2D';
    ctx.fillRect(0, 0, canvas.width, canvas.height);
    ctx.fillStyle = '#0BCDD4';
    ctx.font = '600 28px "Segoe UI", sans-serif';
    ctx.textAlign = 'center';
    ctx.textBaseline = 'middle';
    ctx.fillText(text, canvas.width / 2, canvas.height / 2 - 20);
    ctx.fillStyle = '#8BD2FFCC';
    ctx.font = '22px "Segoe UI", sans-serif';
    ctx.fillText(`Slices: ${sliceCount}`, canvas.width / 2, canvas.height / 2 + 24);
  }
  return {
    sampleName,
    width: 0,
    height: 0,
    sliceCount,
    pixelSpacing: undefined,
    thumbnail: canvas.toDataURL('image/png'),
  };
}

export async function loadDicomSeries(files: File[]): Promise<DicomSeriesInfo> {
  if (!Array.isArray(files) || !files.length) {
    throw new Error('请先选择至少一个 DICOM 文件');
  }

  const withPath = files.filter((file) => Boolean((file as File & { path?: string }).path));
  if (!withPath.length) {
    throw new Error('选择的文件缺少路径信息，无法通过原生模块加载');
  }

  const sampleName = (() => {
    const first = withPath[0];
    const name = first.name.replace(/\.[^./]+$/, '');
    return name || '未命名样本';
  })();

  const bridge = resolveNativeDicomBridge();
  if (!bridge?.loadSeries) {
    ElMessage?.warning?.('未检测到原生 DICOM 模块，使用占位信息');
    return toPlaceholder(sampleName, withPath.length);
  }

  const payload = {
    files: withPath.map((file) => {
      const enriched = file as File & { path?: string };
      return {
        path: enriched.path,
        name: file.name,
      };
    }),
  };

  try {
    const result = await bridge.loadSeries(payload);
    if (!result) {
      throw new Error('原生模块未返回结果');
    }
    const width = result.width ?? 0;
    const height = result.height ?? 0;
    const sliceCount = result.sliceCount ?? withPath.length;
    const thumbnail = result.thumbnail && result.thumbnail.length > 0
      ? result.thumbnail
      : toPlaceholder(sampleName, sliceCount).thumbnail;

    return {
      sampleName: result.sampleName ?? sampleName,
      width,
      height,
      sliceCount,
      pixelSpacing: result.pixelSpacing,
      thumbnail,
    };
  } catch (error) {
    console.error('[dicomLoader] loadSeries failed', error);
    ElMessage?.error?.('加载 DICOM 序列失败');
    throw error instanceof Error ? error : new Error('加载 DICOM 序列失败');
  }
}
