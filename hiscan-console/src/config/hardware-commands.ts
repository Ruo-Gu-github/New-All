export type HardwareCommandArgType = 'number' | 'text' | 'select';

export interface HardwareCommandArg {
  key: string;
  label: string;
  placeholder?: string;
  type?: HardwareCommandArgType;
  required?: boolean;
  options?: Array<{ label: string; value: string }>;
  helperText?: string;
  min?: number;
  max?: number;
  step?: number;
  unit?: string;
  convert?: 'ma-from-ua';
}

export interface HardwareCommandDefinition {
  id: string;
  label: string;
  description?: string;
  groupId: string;
  args?: HardwareCommandArg[];
  tips?: string;
}

export interface HardwareCommandGroup {
  id: string;
  label: string;
  commands: HardwareCommandDefinition[];
}

export interface PresetSequenceStep {
  commandId: string;
  args?: Record<string, string>;
  note?: string;
}

export interface PresetSequenceDefinition {
  id: string;
  label: string;
  description?: string;
  steps: PresetSequenceStep[];
}

export const hardwareCommandGroups: HardwareCommandGroup[] = [
  {
    id: 'xray',
    label: '射线源',
    commands: [
      {
        id: 'hardware-initialize',
        label: '初始化系统',
        groupId: 'xray',
        description: '初始化硬件总线，准备射线源、转台等子系统。',
      },
      {
        id: 'xray-initialize',
        label: '射线源初始化',
        groupId: 'xray',
        description: '对射线源子系统进行独立初始化。',
      },
      {
        id: 'xray-enable',
        label: '开启射线源',
        groupId: 'xray',
        description: '打开高压射线源，开始发射。',
      },
      {
        id: 'xray-disable',
        label: '关闭射线源',
        groupId: 'xray',
        description: '关闭射线源，停止发射。',
      },
      {
        id: 'xray-setkv',
        label: '设置电压 (kV)',
        groupId: 'xray',
        description: '仅更新电压参数，需配合设置电流后生效。',
        args: [
          {
            key: 'voltage_kv',
            label: '电压 (kV)',
            type: 'number',
            placeholder: '$kv',
            min: 40,
            max: 90,
            step: 1,
            required: true,
            helperText: '范围示例：40 ~ 90 kV',
          },
        ],
        tips: '需与“设置电流”命令配合使用。',
      },
      {
        id: 'xray-setua',
        label: '设置电流 (µA)',
        groupId: 'xray',
        description: '设置电流并触发束流配置，需先设置电压。',
        args: [
          {
            key: 'current_ua',
            label: '电流 (µA)',
            type: 'number',
            placeholder: '$ua',
            min: 89,
            max: 200,
            step: 1,
            required: true,
            helperText: '范围示例：89 ~ 200 µA',
            convert: 'ma-from-ua',
          },
        ],
        tips: '会与当前电压组合并下发至硬件。',
      },
      {
        id: 'xray-setbeam',
        label: '设置束流参数',
        groupId: 'xray',
        description: '一次性设置电压、电流。',
        args: [
          {
            key: 'voltage_kv',
            label: '电压 (kV)',
            type: 'number',
            placeholder: '$kv',
            min: 40,
            max: 90,
            step: 1,
            required: true,
          },
          {
            key: 'current_ua',
            label: '电流 (µA)',
            type: 'number',
            placeholder: '$ua',
            min: 89,
            max: 200,
            step: 1,
            required: true,
            helperText: '单位：微安 (µA)',
          },
        ],
      },
    ],
  },
  {
    id: 'detector',
    label: '探测器',
    commands: [
      {
        id: 'detector-initialize',
        label: '初始化探测器',
        groupId: 'detector',
      },
      {
        id: 'detector-shutdown',
        label: '关闭探测器',
        groupId: 'detector',
      },
      {
        id: 'detector-set-binning',
        label: '设置 Binning',
        groupId: 'detector',
        args: [
          {
            key: 'bin_x',
            label: 'Binning X',
            type: 'number',
            placeholder: '$binX',
            min: 1,
            max: 8,
            step: 1,
            required: true,
          },
          {
            key: 'bin_y',
            label: 'Binning Y',
            type: 'number',
            placeholder: '$binY',
            min: 1,
            max: 8,
            step: 1,
            required: true,
          },
        ],
      },
      {
        id: 'detector-set-exposure',
        label: '设置曝光时间',
        groupId: 'detector',
        args: [
          {
            key: 'exposure_ms',
            label: '曝光 (ms)',
            type: 'number',
            placeholder: '$exposure',
            min: 5,
            max: 1000,
            step: 1,
            required: true,
          },
        ],
      },
      {
        id: 'detector-snapshot',
        label: '采集图像',
        groupId: 'detector',
        description: '采集探测器图像帧，可指定次数。',
        args: [
          {
            key: 'count',
            label: '采集次数',
            type: 'number',
            placeholder: '1',
            min: 1,
            max: 10,
            step: 1,
            required: true,
            helperText: '默认采集 1 次',
          },
        ],
      },
    ],
  },
  {
    id: 'stage',
    label: '转台 / 平移台',
    commands: [
      {
        id: 'stage-initialize',
        label: '初始化平台',
        groupId: 'stage',
      },
      {
        id: 'stage-shutdown',
        label: '关闭平台',
        groupId: 'stage',
      },
      {
        id: 'stage-move-by',
        label: '移动一段距离',
        groupId: 'stage',
        args: [
          {
            key: 'axis',
            label: '轴向',
            type: 'select',
            options: [
              { label: 'X 轴', value: 'X' },
              { label: 'Y 轴', value: 'Y' },
              { label: 'Z 轴', value: 'Z' },
              { label: '转台 (R)', value: 'R' },
            ],
            required: true,
            placeholder: 'X',
          },
          {
            key: 'distance',
            label: '距离 (mm / °)',
            type: 'number',
            placeholder: '$distance',
            step: 0.01,
            required: true,
          },
        ],
      },
      {
        id: 'stage-move-to',
        label: '移动到目标位置',
        groupId: 'stage',
        args: [
          {
            key: 'axis',
            label: '轴向',
            type: 'select',
            options: [
              { label: 'X 轴', value: 'X' },
              { label: 'Y 轴', value: 'Y' },
              { label: 'Z 轴', value: 'Z' },
              { label: '转台 (R)', value: 'R' },
            ],
            required: true,
            placeholder: 'X',
          },
          {
            key: 'position',
            label: '目标位置 (mm / °)',
            type: 'number',
            placeholder: '$position',
            step: 0.01,
            required: true,
          },
        ],
      },
      {
        id: 'stage-start-continuous',
        label: '连续移动(启动)',
        groupId: 'stage',
        args: [
          {
            key: 'axis',
            label: '轴向',
            type: 'select',
            options: [
              { label: 'X 轴', value: 'X' },
              { label: 'Y 轴', value: 'Y' },
              { label: 'Z 轴', value: 'Z' },
              { label: '转台 (R)', value: 'R' },
            ],
            required: true,
            placeholder: 'X',
          },
          {
            key: 'velocity',
            label: '速度 (mm/s 或 °/s)',
            type: 'number',
            placeholder: '$velocity',
            step: 0.01,
            required: true,
          },
        ],
      },
      {
        id: 'stage-stop-continuous',
        label: '连续移动(停止)',
        groupId: 'stage',
        args: [
          {
            key: 'axis',
            label: '轴向',
            type: 'select',
            options: [
              { label: 'X 轴', value: 'X' },
              { label: 'Y 轴', value: 'Y' },
              { label: 'Z 轴', value: 'Z' },
              { label: '转台 (R)', value: 'R' },
            ],
            required: true,
            placeholder: 'X',
          },
        ],
      },
      {
        id: 'stage-rotate',
        label: '转台旋转',
        groupId: 'stage',
        args: [
          {
            key: 'degrees',
            label: '角度 (°)',
            type: 'number',
            placeholder: '$deg',
            step: 0.1,
            required: true,
          },
        ],
      },
    ],
  },
  {
    id: 'camera',
    label: '摄像头',
    commands: [
      {
        id: 'camera-initialize',
        label: '初始化摄像头',
        groupId: 'camera',
      },
      {
        id: 'camera-capture',
        label: '采集摄像头图像',
        groupId: 'camera',
        args: [
          {
            key: 'count',
            label: '采集次数',
            type: 'number',
            placeholder: '1',
            min: 1,
            max: 5,
            step: 1,
            required: true,
          },
        ],
      },
      {
        id: 'camera-shutdown',
        label: '关闭摄像头',
        groupId: 'camera',
      },
    ],
  },
  {
    id: 'shoot',
    label: '拍摄命令',
    commands: [
      {
        id: 'continuous-shoot',
        label: '连续采图',
        groupId: 'shoot',
        args: [
          {
            key: 'angle_total',
            label: '旋转总角度',
            type: 'number',
            placeholder: '360',
            min: 0,
            max: 360,
            step: 0.1,
            required: true,
          },
          {
            key: 'angle_step',
            label: '角度间隔',
            type: 'number',
            placeholder: '1',
            min: 0.01,
            max: 360,
            step: 0.01,
            required: true,
          }
        ],
      },
      {
        id: 'step-shoot',
        label: '单步采图',
        groupId: 'shoot',
        args: [
          {
            key: 'angle',
            label: '旋转角度',
            type: 'number',
            placeholder: '1',
            min: 0.01,
            max: 360,
            step: 0.01,
            required: true,
          },
          {
            key: 'angle_step',
            label: '角度间隔',
            type: 'number',
            placeholder: '1',
            min: 0.01,
            max: 360,
            step: 0.01,
            required: true,
          },
          {
            key: 'shoot_count',
            label: '每个角度拍几张图',
            type: 'number',
            placeholder: '1',
            min: 1,
            max: 100,
            step: 1,
            required: true,
          },
        ],
      },
    ],
  },
];

export const presetSequences: PresetSequenceDefinition[] = [
  {
    id: 'lightground',
    label: '单次取像 (lightground)',
    description: '射线源打开 -> 设置电压/电流 -> 采集1帧 -> 关闭射线源',
    steps: [
      { commandId: 'xray-initialize' },
      { commandId: 'xray-enable' },
      { commandId: 'xray-setkv', args: { voltage_kv: '$kv' } },
      { commandId: 'xray-setua', args: { current_ua: '$ua' } },
      { commandId: 'detector-snapshot', args: { count: '1' }, note: '采集一帧探测器图像' },
      { commandId: 'xray-disable' },
    ],
  },
  {
    id: 'detect-init',
    label: '探测器启动流程',
    description: '初始化硬件 -> 初始化射线源和探测器 -> 设置曝光',
    steps: [
      { commandId: 'hardware-initialize' },
      { commandId: 'xray-initialize' },
      { commandId: 'detector-initialize' },
      { commandId: 'detector-set-exposure', args: { exposure_ms: '50' } },
    ],
  },
  {
    id: 'stage-home',
    label: '转台复位 + 准备',
    description: '初始化平台并复位关键轴',
    steps: [
      { commandId: 'stage-initialize' },
      { commandId: 'stage-move-to', args: { axis: 'R', position: '0' } },
      { commandId: 'stage-move-to', args: { axis: 'X', position: '0' } },
      { commandId: 'stage-move-to', args: { axis: 'Y', position: '0' } },
    ],
  },
];

const commandMap = new Map<string, HardwareCommandDefinition>();
for (const group of hardwareCommandGroups) {
  for (const command of group.commands) {
    commandMap.set(command.id, command);
  }
}

export function getCommandDefinition(commandId: string): HardwareCommandDefinition | undefined {
  return commandMap.get(commandId);
}

export function createDefaultArgs(
  definition: HardwareCommandDefinition,
  initialValues?: Record<string, string>,
): Record<string, string> {
  const result: Record<string, string> = {};
  if (definition.args) {
    for (const arg of definition.args) {
      const key = arg.key;
      const provided = initialValues?.[key];
      result[key] = provided ?? '';
    }
  }
  return result;
}

export function clonePresetSteps(preset: PresetSequenceDefinition) {
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
    };
  }).filter(Boolean) as Array<{
    key: string;
    commandId: string;
    label: string;
    args: Record<string, string>;
    note: string;
  }>;
}
