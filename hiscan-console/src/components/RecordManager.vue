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
        <el-form-item label="拍摄日期">
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
      <el-table-column prop="id" label="编号" min-width="120" align="center" header-align="center" />
      <el-table-column prop="sampleName" label="样品名称" min-width="120" align="center" header-align="center" show-overflow-tooltip />
      <el-table-column prop="organization" label="组织/单位" min-width="120" align="center" header-align="center" show-overflow-tooltip />
      <el-table-column prop="type" label="类型" min-width="80" align="center" header-align="center" />
      <el-table-column prop="shootTime" label="拍摄时间" min-width="160" align="center" header-align="center" />
      <el-table-column prop="current" label="电流" min-width="80" align="center" header-align="center" />
      <el-table-column prop="voltage" label="电压" min-width="80" align="center" header-align="center" />
      <el-table-column prop="fov" label="FOV" min-width="80" align="center" header-align="center" />
      <el-table-column prop="binning" label="Binning" min-width="80" align="center" header-align="center" />
      <el-table-column prop="protocol" label="协议" min-width="80" align="center" header-align="center" show-overflow-tooltip />
      <el-table-column prop="status" label="状态" min-width="80" align="center" header-align="center">
        <template #default="scope">
          <el-tag :type="statusType(scope.row.status)">{{ scope.row.status }}</el-tag>
        </template>
      </el-table-column>
      <el-table-column prop="imagePath" label="图像状态" min-width="160" align="center" header-align="center">
        <template #default="scope">
          <span v-if="scope.row.status === '已完成'">
            <el-button size="small" @click="onOpenImage(scope.row)" :disabled="!scope.row.imageExists">打开图像</el-button>
            <span v-if="scope.row.imageExists" style="color: #67c23a; margin-left: 8px;">已存在</span>
            <span v-else style="color: #f56c6c; margin-left: 8px;">未找到</span>
          </span>
          <span v-else>--</span>
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

<script setup>
import { computed, reactive, ref, onMounted, watch, nextTick, onBeforeUnmount } from "vue";

const records = ref([]);
const tableWrapperRef = ref(null);
const tableRef = ref();
const currentPage = ref(1);
const pageSize = ref(10);

const DB_PATH = "records.db"; // relative to app working dir; change if needed

async function loadRecordsFromDb() {
  try {
    // const rows = await core.invoke('get_records', { dbPath: DB_PATH });
    // // map rows returned from Rust to component shape
    // records.value = await Promise.all((rows || []).map(async r => {
    //   const imageExists = await checkImageExists(r.image_path);
    //   return {
    //     id: r.id,
    //     sampleName: r.sample_name,
    //     organization: r.organization,
    //     type: r.type,
    //     shootTime: r.shoot_time,
    //     current: r.current,
    //     voltage: r.voltage,
    //     fov: r.fov,
    //     binning: r.binning,
    //     protocol: r.protocol,
    //     status: r.status,
    //     dateValue: r.date_value,
    //     imagePath: r.image_path,
    //     imageExists,
    //   };
    // }));
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

handleQuery();
recalcPageSize();

watch(filteredRecords, (list) => {
  const maxPage = Math.max(1, Math.ceil(list.length / (pageSize.value || 1)));
  if (currentPage.value > maxPage) {
    currentPage.value = maxPage;
  }
  recalcPageSize();
});

watch(pageSize, () => {
  ensureCurrentPageWithinRange();
});

let resizeObserver;

onMounted(async () => {
  window.addEventListener('resize', recalcPageSize);
  try {
    await loadRecordsFromDb();
  } catch (e) {
    console.error('db load error', e);
  }
  nextTick(() => {
    if (tableWrapperRef.value) {
      resizeObserver = new ResizeObserver(() => recalcPageSize());
      resizeObserver.observe(tableWrapperRef.value);
    }
  });
});

onBeforeUnmount(() => {
  window.removeEventListener('resize', recalcPageSize);
  if (resizeObserver) {
    resizeObserver.disconnect();
    resizeObserver = undefined;
  }
});

function statusType(status) {
  switch (status) {
    case "已完成":
      return "success";
    case "进行中":
      return "warning";
    case "排队中":
      return "info";
    default:
      return "default";
  }
}

// 新增：判断图像路径是否存在
async function checkImageExists(path) {
  if (!path) return false;
  try {
    return await core.fs.exists(path);
  } catch {
    return false;
  }
}

function onOpenImage(row) {
  // 这里暂不实现打开，仅做占位
  // 以后可用 Tauri shell.open 或 Node.js child_process 调用本地看图软件
  // alert(`打开图像目录: ${row.imagePath}`);
}
</script>

<style scoped>

.tab-content {
  width: 100%;
  height: 100%;
  background: linear-gradient(180deg, #0e183e 0%, #070d26 100%);
  color: #f4f7ff;
  padding: 20px 24px 16px;
  box-sizing: border-box;
  display: flex;
  flex-direction: column;
}

.filter-bar {
  background: rgba(20, 32, 76, 0.88);
  border: 1px solid rgba(94, 120, 214, 0.38);
  border-radius: 12px;
  padding: 12px 22px 6px;
  margin-bottom: 14px;
  box-shadow: 0 12px 28px rgba(6, 14, 40, 0.45);
  backdrop-filter: blur(12px);
}

.filter-form {
  display: flex;
  flex-wrap: wrap;
  gap: 12px 16px;
  color: #f4f7ff;
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
  --el-table-bg-color: rgba(226, 236, 255, 0.95);
  border: none !important;
  min-width: 1400px;
  width: 100%;
}


.record-table :deep(.el-table__header-wrapper th) {
  background: linear-gradient(180deg, rgba(213, 226, 255, 1) 0%, rgba(193, 214, 255, 1) 100%);
  color: #16306a;
  font-weight: 600;
  border-bottom: none !important;
  border-top: none !important;
  border-left: none !important;
  border-right: none !important;
}



.record-table :deep(.el-table__body-wrapper td) {
  color: #1f2f63;
  background-color: var(--el-table-bg-color);
  border-bottom: none !important;
  border-right: none !important;
}

.record-table :deep(.el-table__body tr.el-table__row--stripe td),
.record-table :deep(.el-table__body tr.el-table__row--stripe td),
.record-table :deep(.el-table__body tr.el-table__row--striped td) {
  background-color: rgba(208, 223, 255, 0.96) !important;
  border-right: none !important;
}

.record-table :deep(.el-table__body tr:hover > td) {
  background-color: rgba(188, 210, 255, 0.94) !important;
  color: #152a59;
}

.record-table :deep(.el-table__inner-wrapper::before) {
  background-color: transparent;
}

.record-table :deep(.el-table__inner-wrapper) {
  background-color: rgba(226, 236, 255, 0.95);
}

.record-table :deep(.el-table__body-wrapper)::-webkit-scrollbar-thumb {
  background: rgba(96, 126, 214, 0.65);
}

.table-footer {
  display: flex;
  justify-content: flex-end;
  padding: 4px 0 0;
}

.table-footer :deep(.el-pagination) {
  --el-pagination-bg-color: rgba(226, 236, 255, 0.85);
}

.record-table :deep(.el-tag) {
  border: none;
  font-weight: 600;
  color: #132c62;
  background: rgba(211, 226, 255, 0.9);
}

.record-table :deep(.el-tag--success) {
  background: rgba(132, 224, 188, 0.9);
  color: #0b3a25;
}

.record-table :deep(.el-tag--warning) {
  background: rgba(248, 212, 136, 0.92);
  color: #4a2c04;
}

.record-table :deep(.el-tag--info) {
  background: rgba(169, 202, 255, 0.94);
  color: #142f6a;
}

.filter-form :deep(.el-form-item__label) {
  color: #f0f4ff;
}

.filter-bar :deep(.el-input__wrapper),
.filter-bar :deep(.el-date-editor .el-input__wrapper) {
  background: rgba(15, 27, 66, 0.85);
  border-color: rgba(108, 134, 222, 0.5);
  color: #f4f7ff;
  box-shadow: none;
}

.filter-bar :deep(.el-input__inner),
.filter-bar :deep(.el-date-editor .el-input__inner),
.filter-bar :deep(.el-range-input) {
  color: #f4f7ff;
}

.filter-bar :deep(.el-button) {
  border-radius: 8px;
}

.filter-bar :deep(.el-button--primary) {
  background: linear-gradient(90deg, #3a6bff 0%, #567dff 100%);
  border-color: transparent;
  box-shadow: 0 6px 14px rgba(40, 92, 255, 0.35);
}

.filter-bar :deep(.el-button:not(.el-button--primary)) {
  background: rgba(16, 30, 74, 0.85);
  border-color: rgba(92, 120, 214, 0.5);
  color: #dce6ff;
}

</style>
