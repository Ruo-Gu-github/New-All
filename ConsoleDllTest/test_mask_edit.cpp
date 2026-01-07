// Mask 编辑功能测试
// 编译后将此代码添加到 ConsoleDllTest.cpp 的 main 函数中

#include "../Common/NativeInterfaces.h"
#include "../DllDicom/DicomApi.h"
#include "../DllImageProcessing/ImageProcessingApi.h"
#include "../DllVisualization/VisualizationApi.h"
#include <iostream>
#include <thread>
#include <chrono>

void TestMaskEdit() {
    printf("\n========== Mask 编辑功能测试 ==========\n\n");

    // ==================== 步骤1：加载 DICOM 数据 ====================
    printf("[1/7] 加载 DICOM 数据...\n");
    const char* dicomFolder = "D:/DicomData/CT_Chest";  // 替换为你的 DICOM 文件夹路径
    
    VolumeHandle volume = Dicom_LoadVolume(dicomFolder);
    if (!volume) {
        printf("❌ 加载失败！请检查路径: %s\n", dicomFolder);
        return;
    }
    
    int width, height, depth;
    Dicom_Volume_GetDimensions(volume, &width, &height, &depth);
    printf("✅ 加载成功：%d x %d x %d\n\n", width, height, depth);

    // ==================== 步骤2：创建 MaskManager ====================
    printf("[2/7] 创建 MaskManager...\n");
    MaskManagerHandle maskMgr = MaskManager_Create();
    printf("✅ MaskManager 已创建\n\n");

    // ==================== 步骤3：创建 Mask ====================
    printf("[3/7] 创建 Mask（三种方式）...\n");
    
    // 方式1：从阈值创建（提取骨头）
    int boneMask = MaskManager_CreateFromThreshold(
        maskMgr, volume,
        200.0f, 3000.0f,  // HU 值范围
        "Bone Mask"
    );
    MaskManager_SetColor(maskMgr, boneMask, 1.0f, 1.0f, 1.0f, 0.7f);  // 白色
    printf("✅ Mask #%d: Bone (阈值 200-3000 HU)\n", boneMask);
    
    // 方式2：创建空白 Mask（手动编辑）
    int customMask = MaskManager_CreateEmpty(
        maskMgr,
        width, height, depth,
        "Custom Mask"
    );
    MaskManager_SetColor(maskMgr, customMask, 0.0f, 1.0f, 0.0f, 0.5f);  // 绿色
    printf("✅ Mask #%d: Custom (空白，待手动编辑)\n", customMask);
    
    // 方式3：克隆 Bone Mask
    int boneCopy = MaskManager_Clone(maskMgr, boneMask);
    MaskManager_SetName(maskMgr, boneCopy, "Bone Copy");
    MaskManager_SetColor(maskMgr, boneCopy, 1.0f, 0.0f, 0.0f, 0.5f);  // 红色
    printf("✅ Mask #%d: Bone Copy (克隆)\n\n", boneCopy);

    // ==================== 步骤4：初始化可视化 ====================
    printf("[4/7] 初始化可视化窗口...\n");
    if (Viz_Init() != NATIVE_OK) {
        printf("❌ 可视化初始化失败\n");
        return;
    }
    
    MPRHandle mprHandle = MPR_Create(800, 800, "MPR Mask 编辑测试");
    if (!mprHandle) {
        printf("❌ MPR 窗口创建失败\n");
        return;
    }
    
    MPR_SetVolume(mprHandle, volume);
    MPR_SetSliceDirection(mprHandle, 1);  // 冠状面
    MPR_SetCenter(mprHandle, width/2.0f, height/2.0f, depth/2.0f);
    printf("✅ MPR 窗口已创建 (800x800)\n\n");

    // ==================== 步骤5：添加 Mask Overlay ====================
    printf("[5/7] 添加 Mask 叠加层...\n");
    
    // 添加 Bone Mask（白色，半透明）
    MPR_AddMaskOverlay(mprHandle, maskMgr, boneMask, 
        1.0f, 1.0f, 1.0f, 0.7f);
    printf("✅ Overlay #0: Bone Mask (白色)\n");
    
    // 添加 Custom Mask（绿色，半透明）
    MPR_AddMaskOverlay(mprHandle, maskMgr, customMask, 
        0.0f, 1.0f, 0.0f, 0.5f);
    printf("✅ Overlay #1: Custom Mask (绿色)\n\n");

    // ==================== 步骤6：设置编辑工具 ====================
    printf("[6/7] 配置 Mask 编辑工具...\n");
    
    // 设置当前要编辑的 Mask（编辑 Custom Mask）
    Mask_SetCurrentMask(maskMgr, customMask);
    
    // 设置画笔大小
    Mask_SetBrushRadius(5.0f);
    
    printf("✅ 当前编辑: Mask #%d (Custom Mask)\n", customMask);
    printf("✅ 画笔半径: %.1f 像素\n\n", Mask_GetBrushRadius());

    // ==================== 步骤7：交互操作说明 ====================
    printf("[7/7] 开始交互测试！\n");
    printf("\n========== 操作指南 ==========\n");
    printf("【视图控制】\n");
    printf("  鼠标滚轮      - 切换切片\n");
    printf("  Ctrl+滚轮     - 缩放视图\n");
    printf("  X/Y/Z 键      - 切换轴向/冠状/矢状面\n");
    printf("\n【Mask 编辑】\n");
    printf("  7 键          - 进入 Mask 编辑模式\n");
    printf("  B 键          - 画笔工具（添加 mask）\n");
    printf("  E 键          - 橡皮擦工具（删除 mask）\n");
    printf("  [ / ] 键      - 减小/增大画笔\n");
    printf("  Shift+[ ]     - 微调画笔（0.5px）\n");
    printf("  左键拖拽      - 绘制/擦除\n");
    printf("\n【测试步骤】\n");
    printf("  1. 按 7 进入 Mask 编辑模式\n");
    printf("  2. 按 B 选择画笔工具（绿色圆圈）\n");
    printf("  3. 左键拖拽在绿色 Custom Mask 上绘制\n");
    printf("  4. 按 E 切换橡皮擦（红色圆圈）\n");
    printf("  5. 左键拖拽擦除刚才绘制的内容\n");
    printf("  6. 滚轮切换切片查看不同层的 mask\n");
    printf("  7. 按 ESC 退出\n");
    printf("==============================\n\n");

    // ==================== 渲染循环 ====================
    printf("🎨 渲染中... 请操作窗口\n");
    
    while (!MPR_ShouldClose(mprHandle)) {
        MPR_Render(mprHandle);
        std::this_thread::sleep_for(std::chrono::milliseconds(16));  // ~60 FPS
    }

    // ==================== 清理 ====================
    printf("\n清理资源...\n");
    MPR_Destroy(mprHandle);
    MaskManager_Destroy(maskMgr);
    Dicom_Volume_Destroy(volume);
    Viz_Shutdown();
    
    printf("✅ 测试完成！\n");
}

int main() {
    TestMaskEdit();
    return 0;
}
