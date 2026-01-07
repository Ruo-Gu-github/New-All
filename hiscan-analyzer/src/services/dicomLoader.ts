import { ElMessage } from 'element-plus';

export type DicomSeriesInfo = {
  sampleName: string;
  width: number;
  height: number;
  pixelSpacing?: [number, number];
  sliceCount: number;
  thumbnail: string;
};

export type NativeDicomFilePayload = {
  name: string;
  path?: string;
  data?: Uint8Array;
};

export type NativeDicomBridge = {
  loadSeries: (payload: { files: NativeDicomFilePayload[] }) => Promise<{
    sampleName?: string;
    width?: number;
    height?: number;
    pixelSpacing?: [number, number];
    sliceCount?: number;
    thumbnail?: string;
  }>;
};

function getNativeDicomBridge(): NativeDicomBridge | null {
  const globalWindow = window as typeof window & {
    nativeBridge?: {
      dicom?: NativeDicomBridge;
    };
  };
  return globalWindow.nativeBridge?.dicom ?? null;
}

async function toNativePayload(files: File[]): Promise<NativeDicomFilePayload[]> {
  return Promise.all(files.map(async (file) => {
    const payload: NativeDicomFilePayload = { name: file.name };
    const withPath = file as File & { path?: string };
    if (withPath.path) {
      payload.path = withPath.path;
    } else {
      const buffer = await file.arrayBuffer();
      payload.data = new Uint8Array(buffer);
    }
    return payload;
  }));
}

function guessSampleName(files: File[]): string {
  if (!files.length) return '未命名样本';
  const first = files[0];
  const base = first.name.replace(/\.[^./]+$/, '');
  return base || '未命名样本';
}

function createPlaceholderThumbnail(text: string): string {
  const size = 256;
  const canvas = document.createElement('canvas');
  canvas.width = size;
  canvas.height = size;
  const ctx = canvas.getContext('2d');
  if (!ctx) return '';
  ctx.fillStyle = '#0A1C2D';
  ctx.fillRect(0, 0, size, size);
  ctx.fillStyle = '#0BCDD4';
  ctx.font = 'bold 32px "Segoe UI", sans-serif';
  ctx.textAlign = 'center';
  ctx.textBaseline = 'middle';
  ctx.fillText(text.slice(0, 6), size / 2, size / 2);
  return canvas.toDataURL('image/png');
}

export async function loadDicomSeries(files: File[]): Promise<DicomSeriesInfo> {
  if (!files.length) {
    throw new Error('未选择任何DICOM文件');
  }

  const bridge = getNativeDicomBridge();
  const sampleName = guessSampleName(files);

  if (!bridge?.loadSeries) {
    ElMessage?.warning?.('未检测到原生DICOM加载模块，使用占位数据');
    return {
      sampleName,
      width: 512,
      height: 512,
      pixelSpacing: [1, 1],
      sliceCount: files.length,
      thumbnail: createPlaceholderThumbnail(sampleName),
    };
  }

  const payload = await toNativePayload(files);
  const result = await bridge.loadSeries({ files: payload });

  const width = result.width ?? 0;
  const height = result.height ?? 0;
  const pixelSpacing = result.pixelSpacing ?? undefined;
  const sliceCount = result.sliceCount ?? files.length;
  const thumbnail = result.thumbnail ?? createPlaceholderThumbnail(sampleName);

  return {
    sampleName: result.sampleName ?? sampleName,
    width,
    height,
    pixelSpacing,
    sliceCount,
    thumbnail,
  };
}
