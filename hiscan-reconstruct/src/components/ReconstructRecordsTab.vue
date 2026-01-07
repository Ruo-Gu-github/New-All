<script setup>
import { computed, reactive, ref, onMounted, nextTick, onBeforeUnmount } from "vue";
import { getNativeRecords } from "../services/nativeBridge";

const records = ref([]);
const tableWrapperRef = ref(null);
const tableRef = ref();
const currentPage = ref(1);
const pageSize = ref(10);

const DB_PATH = "records.db"; // 可根据实际路径调整

async function loadRecordsFromDb() {
  try {
  const rows = await getNativeRecords(DB_PATH);
    records.value = (rows || []).map(r => ({
      id: r.id,
      sampleName: r.sample_name,
      organization: r.organization,
      type: r.type,
      shootTime: r.shoot_time,
      current: r.current,
      voltage: r.voltage,
      fov: r.fov,
      binning: r.binning,
      protocol: r.protocol,
      status: r.status,
      dateValue: r.date_value,
      imagePath: r.image_path || '',
      imageExists: false // 可后续补充本地文件检测
    }));
    nextTick(() => {
      ensureCurrentPageWithinRange();
      recalcPageSize();
    });
  } catch (e) {
    console.error('failed to load records', e);
  }
}

const form = reactive({
  organization: "",
  type: "",
  dateRange: [],
  keyword: "",
});

const appliedFilters = reactive({
  organization: "",
  type: "",
  dateRange: [],
  keyword: "",
});

const filteredRecords = computed(() =>
  records.value.filter((item) => {
    if (appliedFilters.organization && item.organization !== appliedFilters.organization) {
      return false;
    }
    if (appliedFilters.type && item.type !== appliedFilters.type) {
      return false;
    }
    if (appliedFilters.keyword) {
      const keyword = appliedFilters.keyword.toLowerCase();
      const text = `${item.id} ${item.sampleName}`.toLowerCase();
      if (!text.includes(keyword)) {
        return false;
      }
    }
    if (appliedFilters.dateRange.length === 2) {
      const [start, end] = appliedFilters.dateRange;
      if (item.dateValue < start || item.dateValue > end) {
        return false;
      }
    }
    return true;
  })
);

const totalRecords = computed(() => filteredRecords.value.length);

const paginatedRecords = computed(() => {
  const start = (currentPage.value - 1) * pageSize.value;
  return filteredRecords.value.slice(start, start + pageSize.value);
});

const onCurrentPageChange = (page) => {
  currentPage.value = page;
};

function ensureCurrentPageWithinRange() {
  const maxPage = Math.max(1, Math.ceil(totalRecords.value / (pageSize.value || 1)));
  if (currentPage.value > maxPage) {
    currentPage.value = maxPage;
  }
}

const recalcPageSize = () => {
  nextTick(() => {
    const wrapper = tableWrapperRef.value;
    const tableComponent = tableRef.value;
    if (!wrapper || !tableComponent) {
      return;
    }
    const tableEl = tableComponent.$el;
    if (!tableEl) {
      return;
    }
    const headerEl = tableEl.querySelector('.el-table__header-wrapper');
    const bodyEl = tableEl.querySelector('.el-table__body-wrapper');
    const footerEl = wrapper.querySelector('.table-footer');
    const headerHeight = headerEl?.offsetHeight ?? 48;
    const footerHeight = footerEl?.offsetHeight ?? 0;
    const availableHeight = wrapper.clientHeight - headerHeight - footerHeight - 24;
    const firstRow = bodyEl?.querySelector('tr');
    const rowHeight = firstRow?.offsetHeight ?? 48;
    if (availableHeight <= 0 || rowHeight <= 0) {
      return;
    }
    const estimatedSize = Math.max(5, Math.floor(availableHeight / rowHeight));
    if (estimatedSize && estimatedSize !== pageSize.value) {
      pageSize.value = estimatedSize;
    } else {
      ensureCurrentPageWithinRange();
    }
  });
};

function handleQuery() {
  appliedFilters.organization = form.organization.trim();
  appliedFilters.type = form.type.trim();
  appliedFilters.keyword = form.keyword.trim();
  appliedFilters.dateRange = form.dateRange && form.dateRange.length === 2 ? [...form.dateRange] : [];
  currentPage.value = 1;
  recalcPageSize();
}

function resetFilters() {
  form.organization = "";
  form.type = "";
  form.dateRange = [];
  form.keyword = "";
  handleQuery();
}

onMounted(() => {
  window.addEventListener('resize', recalcPageSize);
  loadRecordsFromDb();
  nextTick(() => {
    if (tableWrapperRef.value) {
      let resizeObserver = new ResizeObserver(() => recalcPageSize());
      resizeObserver.observe(tableWrapperRef.value);
    }
  });
});

onBeforeUnmount(() => {
  window.removeEventListener('resize', recalcPageSize);
});

function statusType(status) {
  switch (status) {
    case "已完成":
      return "success";
    case "进行中":
      return "warning";
    case "排队中":
      return "info";
    case "未开始":
    case "未完成":
      return "danger";
    default:
      return "default";
  }
}

function onOpenImage(row) {
  // 可补充本地文件打开逻辑
}
</script>

<template>
  <div class="tab-content">
    <section class="filter-bar">
      <el-form :inline="true" :model="form" class="filter-form">
        <el-form-item label="组织/单位">
          <el-input
            v-model="form.organization"
            placeholder="全部"
            clearable
            class="filter-input"
          />
        </el-form-item>
        <el-form-item label="样品类型">
          <el-input
            v-model="form.type"
            placeholder="全部"
            clearable
            class="filter-input"
          />
        </el-form-item>
        <el-form-item label="扫描日期">
          <el-date-picker
            v-model="form.dateRange"
            type="daterange"
            range-separator="至"
            start-placeholder="开始日期"
            end-placeholder="结束日期"
            value-format="YYYY-MM-DD"
            unlink-panels
            clearable
            class="filter-date"
          />
        </el-form-item>
        <el-form-item label="关键词">
          <el-input
            v-model="form.keyword"
            placeholder="编号 / 样品名称"
            clearable
            class="filter-input keyword"
          />
        </el-form-item>
        <el-form-item>
          <el-button type="primary" @click="handleQuery">查询</el-button>
          <el-button @click="resetFilters">重置</el-button>
        </el-form-item>
      </el-form>
    </section>

    <div ref="tableWrapperRef" class="table-wrapper">
      <el-table
        ref="tableRef"
        :data="paginatedRecords"
        stripe
        class="record-table"
        table-layout="fixed"
      >
      <el-table-column prop="id" label="编号" min-width="140" align="center" header-align="center" />
      <el-table-column prop="sampleName" label="名称" min-width="140" align="center" header-align="center" show-overflow-tooltip />
      <el-table-column prop="organization" label="单位" min-width="140" align="center" header-align="center" show-overflow-tooltip />
      <el-table-column prop="type" label="类型" min-width="100" align="center" header-align="center" />
      <el-table-column prop="current" label="电流" min-width="100" align="center" header-align="center" />
      <el-table-column prop="voltage" label="电压" min-width="100" align="center" header-align="center" />
      <el-table-column prop="fov" label="FOV" min-width="100" align="center" header-align="center" />
      <el-table-column prop="correction" label="矫正" min-width="110" align="center" header-align="center">
        <template #default="scope">
          <el-tag :type="statusType(scope.row.correction)">{{ scope.row.correction }}</el-tag>
        </template>
      </el-table-column>
      <el-table-column prop="reconstruction" label="重建" min-width="110" align="center" header-align="center">
        <template #default="scope">
          <el-tag :type="statusType(scope.row.reconstruction)">{{ scope.row.reconstruction }}</el-tag>
        </template>
      </el-table-column>
      <el-table-column prop="denoise" label="降噪" min-width="110" align="center" header-align="center">
        <template #default="scope">
          <el-tag :type="statusType(scope.row.denoise)">{{ scope.row.denoise }}</el-tag>
        </template>
      </el-table-column>
      <el-table-column prop="imagePath" label="图像位置" min-width="200" align="center" header-align="center">
        <template #default="scope">
          <div class="image-cell">
            <el-button size="small" @click="onOpenImage(scope.row)">打开图像</el-button>
            <span :class="{ exists: scope.row.imageExists, missing: !scope.row.imageExists }">
              {{ scope.row.imageExists ? "已生成" : "未找到" }}
            </span>
          </div>
        </template>
      </el-table-column>
      </el-table>
      <div class="table-footer">
        <el-pagination
          background
          layout="total, prev, pager, next, jumper"
          :total="totalRecords"
          :current-page="currentPage"
          :page-size="pageSize"
          @current-change="onCurrentPageChange"
        />
      </div>
    </div>
  </div>
</template>

<style scoped>
.tab-content {
  width: 100%;
  height: 100%;
  background: linear-gradient(180deg, rgba(46, 32, 0, 0.96) 0%, rgba(22, 14, 0, 0.96) 100%);
  color: #ffefc6;
  padding: 20px 24px 16px;
  box-sizing: border-box;
  display: flex;
  flex-direction: column;
}

.filter-bar {
  background: rgba(60, 38, 0, 0.88);
  border: 1px solid rgba(255, 204, 64, 0.4);
  border-radius: 12px;
  padding: 12px 22px 6px;
  margin-bottom: 14px;
  box-shadow: 0 12px 28px rgba(32, 20, 0, 0.5);
  backdrop-filter: blur(12px);
}

.filter-form {
  display: flex;
  flex-wrap: wrap;
  gap: 12px 16px;
  color: #ffefc6;
}

.filter-form :deep(.el-form-item) {
  margin-bottom: 10px;
}

.filter-input,
.filter-date {
  width: 150px;
}

.filter-date {
  width: 240px;
}

.filter-input.keyword {
  width: 220px;
}

.table-wrapper {
  flex: 1;
  display: flex;
  flex-direction: column;
  gap: 12px;
}

.record-table {
  flex: 1;
  border-radius: 12px;
  overflow: auto;
  --el-table-bg-color: rgba(255, 245, 219, 0.92);
  border: none !important;
  min-width: 1400px;
  width: 100%;
}

.record-table :deep(.el-table__header-wrapper th) {
  background: linear-gradient(180deg, rgba(255, 236, 196, 1) 0%, rgba(255, 225, 170, 1) 100%);
  color: #604100;
  font-weight: 600;
  border: none !important;
}

.record-table :deep(.el-table__body-wrapper td) {
  color: #5b3d00;
  background-color: var(--el-table-bg-color);
  border-bottom: none !important;
  border-right: none !important;
}

.record-table :deep(.el-table__body tr.el-table__row--striped td) {
  background-color: rgba(255, 236, 198, 0.96) !important;
}

.record-table :deep(.el-table__body tr:hover > td) {
  background-color: rgba(255, 228, 170, 0.92) !important;
  color: #4a2f00;
}

.record-table :deep(.el-table__inner-wrapper::before) {
  background-color: transparent;
}

.record-table :deep(.el-table__inner-wrapper) {
  background-color: rgba(255, 245, 219, 0.92);
}

.table-footer {
  display: flex;
  justify-content: flex-end;
  padding: 4px 0 0;
}

.table-footer :deep(.el-pagination) {
  --el-pagination-bg-color: rgba(255, 245, 219, 0.8);
}

.record-table :deep(.el-tag) {
  border: none;
  font-weight: 600;
  color: #5a3b00;
  background: rgba(255, 231, 175, 0.9);
}

.record-table :deep(.el-tag--success) {
  background: rgba(255, 210, 140, 0.92);
  color: #4d3200;
}

.record-table :deep(.el-tag--warning) {
  background: rgba(255, 190, 120, 0.92);
  color: #4a1f00;
}

.record-table :deep(.el-tag--info) {
  background: rgba(255, 223, 174, 0.92);
  color: #422400;
}

.record-table :deep(.el-tag--danger) {
  background: rgba(255, 168, 132, 0.9);
  color: #4a1300;
}

.filter-form :deep(.el-form-item__label) {
  color: #ffedba;
}

.filter-bar :deep(.el-input__wrapper),
.filter-bar :deep(.el-date-editor .el-input__wrapper) {
  background: rgba(32, 22, 0, 0.85);
  border-color: rgba(255, 213, 79, 0.4);
  color: #ffe9b0;
  box-shadow: none;
}

.filter-bar :deep(.el-input__inner),
.filter-bar :deep(.el-range-input) {
  color: #ffe9b0;
}

.filter-bar :deep(.el-button) {
  border-radius: 8px;
}

.filter-bar :deep(.el-button--primary) {
  background: linear-gradient(90deg, #ffb74d 0%, #ffcc80 100%);
  border-color: transparent;
  color: #4a2b00;
  box-shadow: 0 6px 14px rgba(255, 193, 7, 0.35);
}

.filter-bar :deep(.el-button:not(.el-button--primary)) {
  background: rgba(36, 24, 0, 0.85);
  border-color: rgba(255, 213, 79, 0.35);
  color: #ffefc6;
}

.record-table :deep(.el-table__body-wrapper)::-webkit-scrollbar-thumb {
  background: rgba(255, 200, 87, 0.65);
}

.image-cell {
  display: flex;
  align-items: center;
  justify-content: center;
  gap: 8px;
}

.image-cell .exists {
  color: #8bc34a;
}

.image-cell .missing {
  color: #ff7043;
}
</style>

