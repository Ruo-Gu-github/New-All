import { invokeHardware } from './nativeBridge';
import {
  createDefaultArgs,
  getCommandDefinition,
  hardwareCommandGroups,
  HardwareCommandDefinition,
  PresetSequenceDefinition,
} from '../config/hardware-commands';

export interface EditableSequenceStep {
  key: string;
  commandId: string;
  label: string;
  args: Record<string, string>;
  note?: string;
}

export interface SequenceExecutionOptions {
  onStepStart?: (index: number, definition: HardwareCommandDefinition) => void;
  onStepSuccess?: (index: number, definition: HardwareCommandDefinition) => void;
  onStepError?: (index: number, definition: HardwareCommandDefinition, error: string) => void;
}

interface ExecutionContext {
  beamVoltage?: number;
  beamCurrent?: number;
}

function parseNumber(raw: unknown, label: string): number {
  if (raw === null || raw === undefined || raw === '') {
    throw new Error(`${label} 不能为空`);
  }
  const value = Number(raw);
  if (Number.isNaN(value)) {
    throw new Error(`${label} 必须是数字`);
  }
  return value;
}

function parseInteger(raw: unknown, label: string): number {
  const value = parseNumber(raw, label);
  if (!Number.isInteger(value)) {
    throw new Error(`${label} 必须是整数`);
  }
  return value;
}

function normalizeAxis(raw: string): string {
  const value = (raw ?? '').toString().trim().toUpperCase();
  if (!value) {
    throw new Error('请选择轴向');
  }
  if (!['X', 'Y', 'Z', 'R'].includes(value)) {
    throw new Error(`不支持的轴向：${value}`);
  }
  return value;
}

function validateStep(definition: HardwareCommandDefinition, stepArgs: Record<string, string>) {
  if (!definition.args) {
    return;
  }
  for (const arg of definition.args) {
    const value = stepArgs[arg.key];
    if ((arg.required ?? true) && (!value && value !== '0')) {
      throw new Error(`请填写 ${definition.label} 的「${arg.label}」`);
    }
    if (typeof value === 'string' && value.trim().startsWith('$')) {
      throw new Error(`请替换 ${definition.label} 的占位符「${arg.label}」`);
    }
    if (arg.type === 'number' && value !== undefined && value !== '') {
      // 验证是否是数字
      const num = Number(value);
      if (Number.isNaN(num)) {
        throw new Error(`${arg.label} 必须是数字`);
      }
      if (arg.min !== undefined && num < arg.min) {
        throw new Error(`${arg.label} 不能小于 ${arg.min}`);
      }
      if (arg.max !== undefined && num > arg.max) {
        throw new Error(`${arg.label} 不能大于 ${arg.max}`);
      }
    }
  }
}

async function executeCommand(
  definition: HardwareCommandDefinition,
  args: Record<string, string>,
  context: ExecutionContext,
) {
  switch (definition.id) {
    case 'hardware-initialize':
      await invokeHardware('hardware_initialize');
      return;
    case 'xray-initialize':
      await invokeHardware('hardware_xray_initialize');
      return;
    case 'xray-enable':
      await invokeHardware('hardware_enable_xray');
      return;
    case 'xray-disable':
      await invokeHardware('hardware_disable_xray');
      return;
    case 'xray-setkv': {
      const voltage = parseNumber(args.voltage_kv, '电压 (kV)');
      context.beamVoltage = voltage;
      return;
    }
    case 'xray-setua': {
      const raw = parseNumber(args.current_ua, '电流 (µA)');
      const convertSpec = definition.args?.find((item) => item.key === 'current_ua');
      const currentMa = convertSpec?.convert === 'ma-from-ua' ? raw / 1000 : raw;
      context.beamCurrent = currentMa;
      if (context.beamVoltage === undefined) {
        throw new Error('请先执行“设置电压 (kV)”命令');
      }
      await invokeHardware('hardware_configure_beam', {
        voltage_kv: context.beamVoltage,
        current_ma: currentMa,
      });
      return;
    }
    case 'xray-setbeam': {
      const voltage = parseNumber(args.voltage_kv, '电压 (kV)');
      const current = parseNumber(args.current_ma, '电流 (mA)');
      context.beamVoltage = voltage;
      context.beamCurrent = current;
      await invokeHardware('hardware_configure_beam', {
        voltage_kv: voltage,
        current_ma: current,
      });
      return;
    }
    case 'detector-initialize':
      await invokeHardware('hardware_detector_initialize');
      return;
    case 'detector-shutdown':
      await invokeHardware('hardware_detector_shutdown');
      return;
    case 'detector-set-binning': {
      const binX = parseInteger(args.bin_x, 'Binning X');
      const binY = parseInteger(args.bin_y, 'Binning Y');
      await invokeHardware('hardware_detector_set_binning', { bin_x: binX, bin_y: binY });
      return;
    }
    case 'detector-set-exposure': {
      const exposure = parseInteger(args.exposure_ms, '曝光时间');
      await invokeHardware('hardware_detector_set_exposure', { exposure_ms: exposure });
      return;
    }
    case 'detector-snapshot': {
      const count = parseInteger(args.count ?? '1', '采集次数');
      for (let i = 0; i < count; i += 1) {
        await invokeHardware('hardware_detector_capture');
      }
      return;
    }
    case 'stage-initialize':
      await invokeHardware('hardware_stage_initialize');
      return;
    case 'stage-shutdown':
      await invokeHardware('hardware_stage_shutdown');
      return;
    case 'stage-move-by': {
      const axis = normalizeAxis(args.axis);
      const distance = parseNumber(args.distance, '移动距离');
      await invokeHardware('hardware_stage_move_by', { axis: axis[0], distance_mm: distance });
      return;
    }
    case 'stage-move-to': {
      const axis = normalizeAxis(args.axis);
      const position = parseNumber(args.position, '目标位置');
      await invokeHardware('hardware_stage_move_to', { axis: axis[0], position });
      return;
    }
    case 'stage-start-continuous': {
      const axis = normalizeAxis(args.axis);
      const velocity = parseNumber(args.velocity, '速度');
      await invokeHardware('hardware_stage_start_continuous', { axis: axis[0], velocity });
      return;
    }
    case 'stage-stop-continuous': {
      const axis = normalizeAxis(args.axis);
      await invokeHardware('hardware_stage_stop_continuous', { axis: axis[0] });
      return;
    }
    case 'stage-rotate': {
      const degrees = parseNumber(args.degrees, '角度');
      await invokeHardware('hardware_rotate_turntable', { degrees });
      return;
    }
    case 'camera-initialize':
      await invokeHardware('hardware_camera_initialize');
      return;
    case 'camera-capture': {
      const count = parseInteger(args.count ?? '1', '采集次数');
      for (let i = 0; i < count; i += 1) {
        await invokeHardware('hardware_camera_capture');
      }
      return;
    }
    case 'camera-shutdown':
      await invokeHardware('hardware_camera_shutdown');
      return;
    default:
      throw new Error(`未实现的命令：${definition.label}`);
  }
}

export async function executeSequence(
  steps: Array<{ commandId: string; args: Record<string, string> }>,
  options?: SequenceExecutionOptions,
) {
  if (!steps.length) {
    throw new Error('请先添加至少一个命令');
  }

  const context: ExecutionContext = {};

  for (let index = 0; index < steps.length; index += 1) {
    const step = steps[index];
    const definition = getCommandDefinition(step.commandId);
    if (!definition) {
      throw new Error(`未找到命令：${step.commandId}`);
    }

    validateStep(definition, step.args ?? {});

    options?.onStepStart?.(index, definition);
    try {
      await executeCommand(definition, step.args ?? {}, context);
      options?.onStepSuccess?.(index, definition);
    } catch (error) {
      const message = error instanceof Error ? error.message : String(error);
      options?.onStepError?.(index, definition, message);
      throw new Error(message);
    }
  }
}

export function createEmptySequenceStep(commandId: string, seed = Date.now()) {
  const definition = getCommandDefinition(commandId);
  if (!definition) {
    throw new Error(`未找到命令：${commandId}`);
  }
  return {
    key: `${commandId}-${seed}-${Math.random().toString(16).slice(2, 8)}`,
    commandId,
    label: definition.label,
    args: createDefaultArgs(definition),
  };
}

export function flattenCommandCatalog() {
  return hardwareCommandGroups.flatMap((group) => group.commands);
}

export function preparePresetSequence(preset: PresetSequenceDefinition) {
  return preset.steps.map((step, index) => {
    const definition = getCommandDefinition(step.commandId);
    if (!definition) {
      return null;
    }
    return {
      key: `${step.commandId}-${index}-${Date.now()}`,
      commandId: step.commandId,
      label: definition.label,
      args: createDefaultArgs(definition, step.args),
      note: step.note ?? '',
    } as EditableSequenceStep;
  }).filter(Boolean) as EditableSequenceStep[];
}
