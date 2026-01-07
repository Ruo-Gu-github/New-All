<script setup lang="ts">
import { computed, ref } from "vue";
import {
  ElButton,
  ElCheckbox,
  ElInput,
  ElInputNumber,
  ElMessage,
} from "element-plus";

type SceneItemType = "folder" | "mask" | "shape";

interface SceneTransform {
  tx: number;
  ty: number;
  tz: number;
  rx: number;
  ry: number;
  rz: number;
}

interface SceneItem {
  id: string;
  type: SceneItemType;
  name: string;
  visible: boolean;
  parentId: string | null;
  transform: SceneTransform;
}

function makeId(prefix: string) {
  return `${prefix}_${Date.now()}_${Math.random().toString(36).slice(2, 9)}`;
}

const items = ref<SceneItem[]>([]);
const selectedId = ref<string | null>(null);

const selectedItem = computed(
  () => items.value.find((it) => it.id === selectedId.value) ?? null
);

function findById(id: string) {
  return items.value.find((it) => it.id === id) ?? null;
}

function getDepth(item: SceneItem) {
  let depth = 0;
  let cur = item;
  while (cur.parentId) {
    const parent = findById(cur.parentId);
    if (!parent) break;
    depth += 1;
    cur = parent;
    if (depth > 50) break;
  }
  return depth;
}

const displayItems = computed(() => {
  const result = items.value
    .slice()
    .sort((a, b) => {
      const da = getDepth(a);
      const db = getDepth(b);
      if (da !== db) return da - db;
      return a.name.localeCompare(b.name);
    })
    .map((it) => ({ it, depth: getDepth(it) }));
  return result;
});

function defaultTransform(): SceneTransform {
  return { tx: 0, ty: 0, tz: 0, rx: 0, ry: 0, rz: 0 };
}

function currentParentIdForCreate() {
  const cur = selectedItem.value;
  if (cur?.type === "folder") return cur.id;
  return null;
}

function addFolder() {
  const folder: SceneItem = {
    id: makeId("folder"),
    type: "folder",
    name: `文件夹 ${items.value.filter((x) => x.type === "folder").length + 1}`,
    visible: true,
    parentId: currentParentIdForCreate(),
    transform: defaultTransform(),
  };
  items.value.push(folder);
  selectedId.value = folder.id;
}

function addMask() {
  const mask: SceneItem = {
    id: makeId("mask"),
    type: "mask",
    name: `Mask ${items.value.filter((x) => x.type === "mask").length + 1}`,
    visible: true,
    parentId: currentParentIdForCreate(),
    transform: defaultTransform(),
  };
  items.value.push(mask);
  selectedId.value = mask.id;
}

function addShape() {
  const shape: SceneItem = {
    id: makeId("shape"),
    type: "shape",
    name: `形状 ${items.value.filter((x) => x.type === "shape").length + 1}`,
    visible: true,
    parentId: currentParentIdForCreate(),
    transform: defaultTransform(),
  };
  items.value.push(shape);
  selectedId.value = shape.id;
}

function collectDescendantIds(rootId: string) {
  const ids = new Set<string>();
  const stack = [rootId];
  while (stack.length) {
    const cur = stack.pop()!;
    if (ids.has(cur)) continue;
    ids.add(cur);
    for (const child of items.value) {
      if (child.parentId === cur) stack.push(child.id);
    }
  }
  return ids;
}

function removeSelected() {
  const cur = selectedItem.value;
  if (!cur) return;

  const toRemove = collectDescendantIds(cur.id);
  items.value = items.value.filter((it) => !toRemove.has(it.id));
  selectedId.value = null;
}

function n(v: unknown) {
  const x = Number(v);
  if (!Number.isFinite(x)) return 0;
  return x;
}

function normalizeSelectedTransform() {
  const cur = selectedItem.value;
  if (!cur) return;
  cur.transform.tx = n(cur.transform.tx);
  cur.transform.ty = n(cur.transform.ty);
  cur.transform.tz = n(cur.transform.tz);
  cur.transform.rx = n(cur.transform.rx);
  cur.transform.ry = n(cur.transform.ry);
  cur.transform.rz = n(cur.transform.rz);
}

function showNotImplemented(label: string) {
  ElMessage.info(`${label}：已建立数据结构，渲染/算法接口待接入 Native API`);
}
</script>

<template>
  <div class="three-d-manager">
    <div class="toolbar">
      <ElButton size="small" @click="addFolder">添加文件夹</ElButton>
      <ElButton size="small" @click="addMask">添加 Mask</ElButton>
      <ElButton size="small" @click="addShape">添加形状</ElButton>
      <ElButton
        size="small"
        type="danger"
        :disabled="!selectedItem"
        @click="removeSelected"
      >
        删除
      </ElButton>
    </div>

    <div class="content">
      <div class="list">
        <div
          v-for="row in displayItems"
          :key="row.it.id"
          class="row"
          :class="{ selected: row.it.id === selectedId }"
          @click="selectedId = row.it.id"
        >
          <span class="indent" :style="{ width: `${row.depth * 12}px` }" />
          <ElCheckbox v-model="row.it.visible" @click.stop />
          <span class="type">{{ row.it.type }}</span>
          <span class="name">{{ row.it.name }}</span>
        </div>

        <div v-if="items.length === 0" class="empty">
          还没有 3D 对象（可添加 Mask/形状/文件夹）
        </div>
      </div>

      <div class="editor" v-if="selectedItem">
        <div class="editor-title">对象属性</div>

        <div class="editor-row">
          <div class="label">名称</div>
          <ElInput v-model="selectedItem.name" size="small" />
        </div>

        <div class="editor-row">
          <div class="label">显示</div>
          <ElCheckbox v-model="selectedItem.visible">可见</ElCheckbox>
        </div>

        <div class="editor-title">平移 (T)</div>
        <div class="grid">
          <div class="cell">
            <div class="label">X</div>
            <ElInputNumber
              v-model="selectedItem.transform.tx"
              size="small"
              :step="1"
              @change="normalizeSelectedTransform"
            />
          </div>
          <div class="cell">
            <div class="label">Y</div>
            <ElInputNumber
              v-model="selectedItem.transform.ty"
              size="small"
              :step="1"
              @change="normalizeSelectedTransform"
            />
          </div>
          <div class="cell">
            <div class="label">Z</div>
            <ElInputNumber
              v-model="selectedItem.transform.tz"
              size="small"
              :step="1"
              @change="normalizeSelectedTransform"
            />
          </div>
        </div>

        <div class="editor-title">旋转 (R, deg)</div>
        <div class="grid">
          <div class="cell">
            <div class="label">X</div>
            <ElInputNumber
              v-model="selectedItem.transform.rx"
              size="small"
              :step="1"
              @change="normalizeSelectedTransform"
            />
          </div>
          <div class="cell">
            <div class="label">Y</div>
            <ElInputNumber
              v-model="selectedItem.transform.ry"
              size="small"
              :step="1"
              @change="normalizeSelectedTransform"
            />
          </div>
          <div class="cell">
            <div class="label">Z</div>
            <ElInputNumber
              v-model="selectedItem.transform.rz"
              size="small"
              :step="1"
              @change="normalizeSelectedTransform"
            />
          </div>
        </div>

        <div class="editor-title">布尔/显示（预留）</div>
        <div class="toolbar">
          <ElButton size="small" @click="showNotImplemented('重叠显示')"
            >重叠显示</ElButton
          >
          <ElButton size="small" @click="showNotImplemented('交集')"
            >交集</ElButton
          >
          <ElButton size="small" @click="showNotImplemented('差集')"
            >差集</ElButton
          >
        </div>
      </div>
    </div>
  </div>
</template>

<style scoped>
.three-d-manager {
  display: flex;
  flex-direction: column;
  gap: 8px;
}

.toolbar {
  display: flex;
  gap: 8px;
  flex-wrap: wrap;
}

.content {
  display: grid;
  grid-template-columns: 1fr 1fr;
  gap: 10px;
}

.list {
  border: 1px solid var(--el-border-color);
  border-radius: 6px;
  overflow: hidden;
  min-height: 140px;
}

.row {
  display: flex;
  align-items: center;
  gap: 8px;
  padding: 6px 8px;
  cursor: pointer;
  border-bottom: 1px solid var(--el-border-color);
}

.row:last-child {
  border-bottom: none;
}

.row.selected {
  background: var(--el-fill-color-light);
}

.indent {
  display: inline-block;
}

.type {
  color: var(--el-text-color-secondary);
  width: 56px;
}

.name {
  flex: 1;
  overflow: hidden;
  text-overflow: ellipsis;
  white-space: nowrap;
}

.empty {
  padding: 10px;
  color: var(--el-text-color-secondary);
}

.editor {
  border: 1px solid var(--el-border-color);
  border-radius: 6px;
  padding: 10px;
}

.editor-title {
  font-weight: 600;
  margin: 6px 0;
}

.editor-row {
  display: grid;
  grid-template-columns: 54px 1fr;
  gap: 8px;
  align-items: center;
  margin-bottom: 8px;
}

.grid {
  display: grid;
  grid-template-columns: repeat(3, 1fr);
  gap: 8px;
}

.cell {
  display: grid;
  grid-template-columns: 16px 1fr;
  gap: 6px;
  align-items: center;
}

.label {
  color: var(--el-text-color-secondary);
  font-size: 12px;
}
</style>
