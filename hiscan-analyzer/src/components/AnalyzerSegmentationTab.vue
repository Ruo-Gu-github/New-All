<script setup>
import { ref } from "vue";

const segmentationTasks = ref([
  { name: "肝脏", status: "已完成", accuracy: "0.94 Dice" },
  { name: "胰腺", status: "运行中", accuracy: "0.82 Dice" },
  { name: "脾脏", status: "等待", accuracy: "—" },
]);
</script>

<template>
  <div class="analyzer-tab-page">
    <section class="panel-grid">
      <article class="panel-card accent">
        <header>
          <h2>分割任务</h2>
          <p>按照解剖部位排序</p>
        </header>
        <ul class="task-list">
          <li v-for="item in segmentationTasks" :key="item.name">
            <div class="task-name">{{ item.name }}</div>
            <div class="task-meta">
              <span class="task-status">{{ item.status }}</span>
              <span class="task-accuracy">{{ item.accuracy }}</span>
            </div>
          </li>
        </ul>
      </article>
      <article class="panel-card">
        <header>
          <h2>模型管理</h2>
          <p>可切换的预训练模型</p>
        </header>
        <div class="form-stack">
          <label
            >当前模型
            <select>
              <option>UNet v3</option>
              <option>SegFormer v2</option>
              <option>Swin-UNet</option>
            </select>
          </label>
          <label
            >后处理
            <select>
              <option>形态学闭操作</option>
              <option>CRF 精化</option>
              <option>无</option>
            </select>
          </label>
          <button class="btn primary block">应用设置</button>
        </div>
      </article>
    </section>
  </div>
</template>
