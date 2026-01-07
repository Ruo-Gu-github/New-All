<template>
  <el-dialog
    v-model="visible"
    title="命令执行器"
    width="1080px"
    class="command-dialog"
    :close-on-click-modal="false"
  >
    <div class="command-dialog-body">
      <div class="command-list-panel">
        <div class="command-panel-header">
          <div class="command-panel-title">命令集合</div>
        </div>
        <div class="command-list-scroll">
          <el-collapse v-model="expandedCommandGroups" class="command-collapse">
            <el-collapse-item
              v-for="group in commandGroups"
              :key="group.id"
              :name="group.id"
            >
              <template #title>
                <span class="command-group-title">{{ group.label }}</span>
              </template>
              <div
                v-for="command in group.commands"
                :key="command.id"
                class="command-item command-item-simple"
                @dblclick="addCommandToSequence(command)"
              >
                <div class="command-item-label">{{ command.label }}</div>
              </div>
            </el-collapse-item>
            <!-- 组合命令作为一个 collapse-item -->
            <el-collapse-item name="presets">
              <template #title>
                <span class="command-group-title">组合命令</span>
              </template>
              <div v-if="commandPresets.length" class="preset-list">
                <div
                  v-for="preset in commandPresets"
                  :key="preset.id"
                  class="preset-card preset-card-simple"
                  @dblclick="applyPresetSequence(preset)"
                >
                  <div class="preset-title">{{ preset.label }}</div>
                </div>
              </div>
              <el-empty v-else description="暂无组合命令" />
            </el-collapse-item>
          </el-collapse>
        </div>
      </div>

      <div class="command-editor-panel">
        <el-form label-width="90px" class="command-editor-form">
          <el-form-item label="序列名称">
            <el-input v-model="commandEditorName" placeholder="可选：命令序列名称" clearable />
          </el-form-item>
        </el-form>

        <div class="command-editor-alert">
          左侧选择命令，右侧编辑参数。占位符请替换为实际值。
        </div>

        <div class="command-step-list">
          <el-empty v-if="!commandEditorSteps.length" description="请先选择命令" />
          <div
            v-else
            v-for="(step, index) in commandEditorSteps"
            :key="step.key"
            class="command-step-card"
          >
            <div class="command-step-row">
              <span class="command-step-index">{{ index + 1 }}.</span>
              <span class="command-step-label">{{ step.label }}</span>
              <template v-for="arg in getDefinitionArgs(step.commandId)">
                <span class="command-arg-label">
                  {{ arg.label }}为
                  <input
                    type="number"
                    v-model="step.args[arg.key]"
                    class="command-arg-input narrow"
                    style="width: 60px; display: inline-block; margin: 0 2px;"
                  />
                  {{ arg.unit ? arg.unit : (arg.label.includes('角度') ? '度' : arg.label.includes('张数') ? '张' : '') }}
                </span>
              </template>
              <div class="command-step-actions">
                <el-button size="small" type="danger" text @click="removeCommandStep(index)">删除</el-button>
              </div>
            </div>
          </div>
        </div>

        <div class="command-editor-footer fixed-footer">
          <div class="command-editor-buttons">
            <el-button size="small" plain @click="clearCommandSequence" :disabled="!commandEditorSteps.length">清空</el-button>
            <el-button size="small" type="primary" :loading="executingCommandSequence" @click="executeCommandSequence">执行</el-button>
          </div>
        </div>
      </div>
    </div>
  </el-dialog>
</template>

<script setup>
import { computed, ref } from 'vue'
import {
  hardwareCommandGroups,
  presetSequences,
  getCommandDefinition,
  createDefaultArgs,
} from '../config/hardware-commands'
import { executeSequence, preparePresetSequence } from '../services/hardwareSequence'
import { ElMessage } from 'element-plus'

const props = defineProps({ modelValue: { type: Boolean, default: false } })
const emit = defineEmits(['update:modelValue'])

const visible = computed({
  get: () => props.modelValue,
  set: (v) => emit('update:modelValue', v),
})

const commandGroups = hardwareCommandGroups
const commandPresets = presetSequences
const expandedCommandGroups = ref([])
const commandEditorName = ref('自定义序列')
const commandEditorSteps = ref([])
const commandEditorStatus = ref('')
const executingCommandSequence = ref(false)

function addCommandToSequence(definition) {
  const step = {
    key: `${definition.id}-${Date.now()}-${Math.random().toString(16).slice(2, 8)}`,
    commandId: definition.id,
    label: definition.label,
    args: createDefaultArgs(definition),
    note: definition.tips ?? '',
  }
  commandEditorSteps.value = [...commandEditorSteps.value, step]
  commandEditorStatus.value = `已添加命令：${definition.label}`
}

function removeCommandStep(index) {
  commandEditorSteps.value = commandEditorSteps.value.filter((_, idx) => idx !== index)
}

function duplicateCommandStep(step, index) {
  const copy = {
    key: `${step.commandId}-${Date.now()}-${Math.random().toString(16).slice(2, 8)}`,
    commandId: step.commandId,
    label: step.label,
    args: { ...step.args },
    note: step.note,
  }
  const next = [...commandEditorSteps.value]
  next.splice(index + 1, 0, copy)
  commandEditorSteps.value = next
  commandEditorStatus.value = `已复制命令：${step.label}`
}

function applyPresetSequence(preset) {
  const steps = preparePresetSequence(preset)
  if (!steps.length) {
    ElMessage.warning('预设序列为空或命令不可用')
    return
  }
  commandEditorSteps.value = steps
  commandEditorName.value = preset.label
  commandEditorStatus.value = `已载入组合命令：${preset.label}`
}

function clearCommandSequence() {
  commandEditorSteps.value = []
  commandEditorStatus.value = '已清空命令序列'
}

function getDefinitionArgs(commandId) {
  const definition = getCommandDefinition(commandId)
  return definition?.args ?? []
}

function getDefinitionTips(commandId) {
  const definition = getCommandDefinition(commandId)
  return definition?.tips ?? ''
}

async function executeCommandSequence() {
  if (!commandEditorSteps.value.length) {
    ElMessage.warning('请先添加要执行的命令')
    return
  }

  executingCommandSequence.value = true
  try {
    const payload = commandEditorSteps.value.map((step) => ({
      commandId: step.commandId,
      args: { ...step.args },
    }))
    await executeSequence(payload, {
      onStepStart(index, definition) {
        commandEditorStatus.value = `执行第 ${index + 1} 步：${definition.label}`
      },
      onStepSuccess(index, definition) {
        commandEditorStatus.value = `完成第 ${index + 1} 步：${definition.label}`
      },
      onStepError(index, definition, error) {
        commandEditorStatus.value = `第 ${index + 1} 步失败（${definition.label}）：${error}`
      },
    })
    commandEditorStatus.value = '命令序列执行完成'
    ElMessage.success(`命令序列执行完成：${commandEditorName.value || '未命名序列'}`)
  } catch (error) {
    const message = error instanceof Error ? error.message : String(error)
    commandEditorStatus.value = message
    ElMessage.error(message)
  } finally {
    executingCommandSequence.value = false
  }
}
</script>

<style scoped>
/* 右侧命令区滚动条风格与左侧一致 */
.command-step-list::-webkit-scrollbar {
  width: 8px;
  background: #232a3b;
  border-radius: 8px;
}
.command-step-list::-webkit-scrollbar-thumb {
  background: #31406e;
  border-radius: 8px;
}
.command-step-list::-webkit-scrollbar-thumb:hover {
  background: #4661a6;
}
/* 右侧panel高度固定为720px，内容滚动，按钮固定底部 */
.command-editor-panel {
  position: relative;
  height: 720px;
  min-height: 720px;
  max-height: 720px;
  display: flex;
  flex-direction: column;
}

.fixed-footer {
  position: absolute;
  left: 0;
  right: 0;
  bottom: 0;
  background: rgba(20,30,60,0.98);
  padding: 12px 0 8px 0;
  z-index: 2;
}
.command-editor-buttons {
  display: flex;
  gap: 8px;
  justify-content: flex-end;
}
/* 命令区最大高度720px，超出滚动 */
.command-step-list {
  max-height: 570px;
  overflow-y: auto;
  padding-right: 8px;
}
/* 命令行全部同行显示，按钮在最右 */
.command-step-row {
  display: flex;
  align-items: center;
  gap: 8px;
  margin-bottom: 2px;
  background: none;
  border: none;
  min-height: 24px;
}
.command-step-index {
  color: #8ca7d5;
  font-size: 13px;
  margin-right: 2px;
}
.command-step-label {
  color: #dbe4ff;
  font-size: 14px;
  font-weight: 500;
  margin-right: 8px;
}
.command-step-actions {
  margin-left: auto;
  display: flex;
  gap: 4px;
}
.command-arg-label {
  min-width: 60px;
  color: #dbe4ff;
  font-size: 13px;
}
.command-arg-input {
  width: 120px !important;
  min-width: 80px;
  max-width: 120px;
  font-size: 13px;
}
.command-arg-input.narrow {
  width: 80px !important;
  min-width: 60px;
  max-width: 80px;
}
.command-arg-helper {
  color: #8ca7d5;
  font-size: 12px;
  margin-left: 4px;
}
.command-step-note {
  color: #8ca7d5;
  font-size: 12px;
  margin-left: 8px;
}
.command-step-tips {
  color: #8ca7d5;
  font-size: 12px;
  margin-left: 8px;
}
/* 简短提示信息一行且浅蓝色 */
.command-editor-alert {
  background: none;
  color: #8ca7d5;
  font-size: 14px;
  margin-bottom: 12px;
  padding: 0;
  border: none;
}
/* 命令参数一行显示，label和input同行，input更窄 */
.command-arg-row {
  display: flex;
  align-items: center;
  gap: 8px;
  margin-bottom: 6px;
}
.command-arg-label {
  min-width: 80px;
  color: #dbe4ff;
  font-size: 13px;
}
.command-arg-input {
  width: 220px !important;
  min-width: 120px;
  max-width: 220px;
  font-size: 13px;
}
.command-arg-helper {
  color: #8ca7d5;
  font-size: 12px;
  margin-left: 8px;
}
/* 优化命令行卡片间距和样式 */
.command-step-card {
  margin-bottom: 2px;
  padding: 2px 0 0 0;
  background: none;
  border: none;
}
.command-dialog-body {
  display: grid;
  grid-template-columns: 260px 1fr;
  gap: 16px;
}

.command-list-panel {
  display: flex;
  flex-direction: column;
  height: 720px;
  background: #232a3b !important;
  border-radius: 6px;
}

.command-panel-header {
  height: 36px;
  display: flex;
  align-items: center;
  padding: 0 4px;
  box-sizing: border-box;
  font-size: 20px;
}

.command-list-scroll {
  flex: 1;
  height: 100%;
  overflow-y: auto;
  overflow-x: hidden;
  margin-bottom: 0;
}
/* 自定义滚动条风格 */
.command-list-panel ::-webkit-scrollbar,
.command-collapse ::-webkit-scrollbar {
  width: 8px;
  background: #232a3b;
  border-radius: 8px;
}
.command-list-panel ::-webkit-scrollbar-thumb,
.command-collapse ::-webkit-scrollbar-thumb {
  background: #31406e;
  border-radius: 8px;
}
.command-list-panel ::-webkit-scrollbar-thumb:hover,
.command-collapse ::-webkit-scrollbar-thumb:hover {
  background: #4661a6;
}
/* collapse整体背景色和header色，使用:deep全局样式 */
.command-collapse {
    border: none;
}

.command-collapse :deep(.el-collapse-item__header) {
  background: #232a3b !important;
  color: #fff !important;
  border: none !important;
  padding-left: 12px !important;
  box-sizing: border-box;
}
.command-collapse :deep(.el-collapse-item__wrap) {
  background: #232a3b !important;
  border: none !important;
}

.command-item.command-item-simple {
  padding: 4px 24px;
  cursor: pointer;
  border-radius: 4px;
  margin-bottom: 0px;
  background: #232a3b;
  color: #fff;
}

.command-item.command-item-simple:hover {
  background: #31406e;
}
.preset-card.preset-card-simple {
  padding: 4px 12px;
  cursor: pointer;
  border-radius: 4px;
  margin-bottom: 0px;
  background: #232a3b;
  color: #fff;
}
.preset-card.preset-card-simple:hover {
  background: #31406e;
}
.command-editor-buttons { display: flex; gap: 8px; }

@media (max-width: 1200px) {
  .command-dialog-body { grid-template-columns: 1fr; }
}
</style>
