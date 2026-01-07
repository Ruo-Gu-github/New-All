// ConsoleDllTest.cpp : ���ļ����� "main" ����������ִ�н��ڴ˴���ʼ��������
//

#include <iostream>
#include <string>
#include <map>
#include <functional>
#include <algorithm>
#include <fstream>

// ��ֹ Windows.h ���� min/max ��
#define NOMINMAX
#include <Windows.h>

#include "../Common/NativeInterfaces.h"

// �������� DLL ��ͷ�ļ�
#include "../DllCore/CoreApi.h"
#include "../DllDicom/DicomApi.h"
#include "../DllVisualization/VisualizationApi.h"
#include "../DllImageProcessing/ImageProcessingApi.h"

// ��̬���ӿ�
#pragma comment(lib, "DllCore.lib")
#pragma comment(lib, "DllDicom.lib")
#pragma comment(lib, "DllVisualization.lib")
#pragma comment(lib, "DllImageProcessing.lib")

using namespace std;

void ShowHelp() {
    cout << "\n=== Medical Image Processing DLL Test Program ===\n";
    cout << "\n��Core Module��\n";
    cout << "  core         - Test Core module\n";
    cout << "  memory       - Test memory management\n";
    cout << "  log          - Test logging system\n";
    cout << "  thread       - Test thread pool\n";
    cout << "  timer        - Test performance timer\n";
    cout << "\n��DICOM Module��- �ɹ�ͷ���� (501��)\n";
    cout << "  dicom-read   - Read single DICOM file (0.dcm)\n";
    cout << "  dicom-series - Read DICOM series (all 501 files)\n";
    cout << "  dicom-volume - Create volume from series\n";
    cout << "  dicom-thumbnail - Generate DICOM thumbnail\n";
    cout << "\n��Visualization Module��\n";
    cout << "  viz-mpr      - MPR (Multi-Planar Reconstruction) ��ƽ���ؽ�\n";
    cout << "  viz-mpr-axial    - MPR Axial view only (����/�����)\n";
    cout << "  viz-mpr-coronal  - MPR Coronal view only (��״��)\n";
    cout << "  viz-mpr-sagittal - MPR Sagittal view only (ʸ״��)\n";
    cout << "  viz-mpr3     - MPR Triple View (Axial+Coronal+Sagittal) ����ͼ\n";
    cout << "  viz-apr      - APR (Arbitrary Plane Reconstruction) ����ƽ���ؽ�\n";
    cout << "  viz-apr3     - APR Triple View (Axial+Coronal+Sagittal) ����ͼ + ��ת\n";
    cout << "  viz-3d       - 3D Volume Rendering ��ά�����\n";
    cout << "  viz-window   - Test empty OpenGL window\n";
    cout << "  viz-host-mpr - Host-style embedding test (parent HWND + child HWND)\n";
    cout << "  viz-offscreen- Test offscreen rendering (for Vue)\n";
    cout << "  viz-tools    - Test measurement tools (�������߲���)\n";
    cout << "\n��Image Processing Module - Mask Operations��\n";
    cout << "  mask-create     - Create basic masks ��������Mask\n";
    cout << "  mask-threshold  - Create mask from threshold ��ֵ�ָ�\n";
    cout << "  mask-boolean    - Boolean operations (Union/Intersection/etc) ��������\n";
    cout << "  mask-morphology - Morphology operations (Dilate/Erode/etc) ��̬ѧ����\n";
    cout << "  mask-connected  - Connected components analysis ��ͨ�����\n";
    cout << "  mask-draw       - Draw ROI (Rectangle/Circle/Polygon) ����ROI\n";
    cout << "  mask-measure    - Measurement (Volume/Centroid/BoundingBox) ��������\n";
    cout << "  mask-io         - Save/Load masks �������\n";
    cout << "  mask-all        - Run all mask tests �������в���\n";
    cout << "  mask-edit       - Interactive mask editing (Brush/Eraser) ?����ʽ�༭?\n";
    cout << "  mpr-measure     - MPR interactive measurement ����ʽ��������\n";
    cout << "\n��General��\n";
    cout << "  help/h       - Show this help message\n";
    cout << "  exit/quit/q  - Exit program\n";
    cout << "\n";
}

void TestCore() {
    cout << "\n=== Testing Core Module ===\n";
    
    // ���԰汾
    const char* version = Core_GetVersion();
    cout << "Core Version: " << version << "\n";
    
    // �����ڴ�ͳ��
    size_t total, peak;
    if (Core_GetMemoryStats(&total, &peak) == NATIVE_OK) {
        cout << "Memory - Total: " << total << " bytes, Peak: " << peak << " bytes\n";
    } else {
        cout << "Failed to get memory stats: " << Core_GetLastError() << "\n";
    }
    
    cout << "Core test completed!\n";
}

void TestMemory() {
    cout << "\n=== Testing Memory Management ===\n";
    
    // �����ڴ�
    size_t size = 1024 * 1024; // 1MB
    void* ptr = Core_Malloc(size);
    
    if (ptr) {
        cout << "Allocated " << size << " bytes\n";
        
        // ��ȡͳ��
        size_t total, peak;
        if (Core_GetMemoryStats(&total, &peak) == NATIVE_OK) {
            cout << "Total allocated: " << total << " bytes\n";
            cout << "Peak usage: " << peak << " bytes\n";
        }
        
        // �ͷ��ڴ�
        Core_Free(ptr);
        cout << "Memory freed\n";
    } else {
        cout << "Failed to allocate memory\n";
    }
}

void TestLogger() {
    cout << "\n=== Testing Logger ===\n";
    
    // ��ʼ����־
    const char* logFile = "test.log";
    if (Core_InitLogger(logFile, LOG_LEVEL_DEBUG) == NATIVE_OK) {
        cout << "Logger initialized: " << logFile << "\n";
        
        // д�벻ͬ�������־
        Core_Log(LOG_LEVEL_DEBUG, "This is a debug message");
        Core_Log(LOG_LEVEL_INFO, "This is an info message");
        Core_Log(LOG_LEVEL_WARNING, "This is a warning message");
        Core_Log(LOG_LEVEL_ERROR, "This is an error message");
        
        cout << "Log messages written to " << logFile << "\n";
        
        // �ر���־
        Core_ShutdownLogger();
        cout << "Logger shutdown\n";
    } else {
        cout << "Failed to initialize logger: " << Core_GetLastError() << "\n";
    }
}

// �߳�����ص�����
void TestTask(void* userData) {
    int* taskId = static_cast<int*>(userData);
    cout << "  Task " << *taskId << " is running on thread " << GetCurrentThreadId() << "\n";
    Sleep(100); // ģ�⹤��
}

void TestThreadPool() {
    cout << "\n=== Testing Thread Pool ===\n";
    
    // �����̳߳أ�4���̣߳�
    ThreadPoolHandle pool = Core_CreateThreadPool(4);
    if (!pool) {
        cout << "Failed to create thread pool\n";
        return;
    }
    
    cout << "Thread pool created with 4 threads\n";
    
    // �ύ10������
    int taskIds[10];
    for (int i = 0; i < 10; ++i) {
        taskIds[i] = i + 1;
        Core_SubmitTask(pool, TestTask, &taskIds[i]);
    }
    
    cout << "Submitted 10 tasks\n";
    
    // ��ȡͳ����Ϣ
    int queued, active;
    if (Core_GetThreadPoolStats(pool, &queued, &active) == NATIVE_OK) {
        cout << "Queued tasks: " << queued << ", Active tasks: " << active << "\n";
    }
    
    // �ȴ������������
    cout << "Waiting for all tasks to complete...\n";
    Core_WaitAllTasks(pool);
    cout << "All tasks completed\n";
    
    // �����̳߳�
    Core_DestroyThreadPool(pool);
    cout << "Thread pool destroyed\n";
}

void TestTimer() {
    cout << "\n=== Testing Performance Timer ===\n";
    
    // ������ʱ��
    TimerHandle timer = Core_CreateTimer();
    if (!timer) {
        cout << "Failed to create timer\n";
        return;
    }
    
    cout << "Timer created\n";
    
    // ����1�����ٲ���
    Core_StartTimer(timer);
    Sleep(100);
    double elapsed = Core_StopTimer(timer);
    cout << "Test 1 (Sleep 100ms): " << elapsed << " ms\n";
    
    // ����2���ϳ�����
    Core_StartTimer(timer);
    Sleep(500);
    elapsed = Core_StopTimer(timer);
    cout << "Test 2 (Sleep 500ms): " << elapsed << " ms\n";
    
    // ���ټ�ʱ��
    Core_DestroyTimer(timer);
    cout << "Timer destroyed\n";
}

// ==================== DICOM ���Ժ��� ====================
void TestDicomRead() {
    cout << "\n=== Testing DICOM File Read ===\n";
    
    // Ĭ��ʹ�ùɹ�ͷ����
    string filepath = "D:\\Scripts\\Example\\�β�\\1.2.3.20210810.133939_0001.dcm";
    cout << "Reading DICOM file: " << filepath << "\n";
    
    // ���� DICOM Reader
    DicomReaderHandle reader = Dicom_CreateReader();
    if (!reader) {
        cout << "Failed to create DICOM reader\n";
        return;
    }
    
    // ��ȡ�ļ�
    NativeResult result = Dicom_ReadFile(reader, filepath.c_str());
    if (result != NATIVE_OK) {
        cout << "Failed to read DICOM file: " << Dicom_GetLastError() << "\n";
        Dicom_DestroyReader(reader);
        return;
    }
    
    cout << "DICOM file read successfully!\n\n";
    
    // ��ȡ���� Tags
    char buffer[512];
    
    cout << "=== Basic Information ===\n";
    if (Dicom_GetTag(reader, 0x0010, 0x0010, buffer, sizeof(buffer)) == NATIVE_OK) {
        cout << "  Patient Name: " << buffer << "\n";
    }
    if (Dicom_GetTag(reader, 0x0010, 0x0020, buffer, sizeof(buffer)) == NATIVE_OK) {
        cout << "  Patient ID: " << buffer << "\n";
    }
    if (Dicom_GetTag(reader, 0x0008, 0x0060, buffer, sizeof(buffer)) == NATIVE_OK) {
        cout << "  Modality: " << buffer << "\n";
    }
    if (Dicom_GetTag(reader, 0x0008, 0x0070, buffer, sizeof(buffer)) == NATIVE_OK) {
        cout << "  Manufacturer: " << buffer << "\n";
    }
    
    cout << "\n=== Acquisition Parameters ===\n";
    if (Dicom_GetTag(reader, 0x0018, 0x0060, buffer, sizeof(buffer)) == NATIVE_OK) {
        cout << "  KVP (��ѹ): " << buffer << " kV\n";
    }
    if (Dicom_GetTag(reader, 0x0018, 0x1151, buffer, sizeof(buffer)) == NATIVE_OK) {
        cout << "  X-Ray Tube Current (����): " << buffer << " mA\n";
    }
    if (Dicom_GetTag(reader, 0x0018, 0x0050, buffer, sizeof(buffer)) == NATIVE_OK) {
        cout << "  Slice Thickness (���): " << buffer << " mm\n";
    }
    if (Dicom_GetTag(reader, 0x0018, 0x0088, buffer, sizeof(buffer)) == NATIVE_OK) {
        cout << "  Spacing Between Slices (����): " << buffer << " mm\n";
    }
    
    cout << "\n=== Image Information ===\n";
    // ��ȡͼ��ߴ�
    int width, height;
    if (Dicom_GetImageSize(reader, &width, &height) == NATIVE_OK) {
        cout << "  Image Size (�ֱ���): " << width << " x " << height << "\n";
    }
    
    if (Dicom_GetTag(reader, 0x0028, 0x0030, buffer, sizeof(buffer)) == NATIVE_OK) {
        cout << "  Pixel Spacing (���ؼ��): " << buffer << " mm\n";
    }
    if (Dicom_GetTag(reader, 0x0028, 0x0100, buffer, sizeof(buffer)) == NATIVE_OK) {
        cout << "  Bits Allocated: " << buffer << "\n";
    }
    if (Dicom_GetTag(reader, 0x0020, 0x0013, buffer, sizeof(buffer)) == NATIVE_OK) {
        cout << "  Instance Number (ͼ����): " << buffer << "\n";
    }
    
    Dicom_DestroyReader(reader);
    cout << "\nDICOM reader destroyed\n";
}

void TestDicomSeries() {
    cout << "\n=== Testing DICOM Series Read ===\n";
    
    // Ĭ��ʹ�ùɹ�ͷ�����ļ���
    string folderPath = "D:\\Scripts\\Example\\�β�";
    //string folderPath = "D:\\Scripts\\Example\\�ɹ�ͷ\\0";
    cout << "Reading DICOM series from: " << folderPath << "\n";
    
    DicomReaderHandle reader = Dicom_CreateReader();
    if (!reader) {
        cout << "Failed to create DICOM reader\n";
        return;
    }
    
    int fileCount = 0;
    NativeResult result = Dicom_ReadDirectory(reader, folderPath.c_str(), &fileCount);
    if (result != NATIVE_OK) {
        cout << "Failed to read DICOM series: " << Dicom_GetLastError() << "\n";
        Dicom_DestroyReader(reader);
        return;
    }
    
    cout << "Successfully read " << fileCount << " DICOM files\n";
    
    // ��ȡ��һ���ļ�����Ϣ
    char buffer[256];
    if (Dicom_GetTag(reader, 0x0010, 0x0010, buffer, sizeof(buffer)) == NATIVE_OK) {
        cout << "  Patient Name: " << buffer << "\n";
    }
    if (Dicom_GetTag(reader, 0x0008, 0x0060, buffer, sizeof(buffer)) == NATIVE_OK) {
        cout << "  Modality: " << buffer << "\n";
    }
    
    Dicom_DestroyReader(reader);
}

void TestDicomVolume() {
    cout << "\n=== Testing DICOM Volume Creation ===\n";
    
    // ʹ�ùɹ�ͷ����
    string folderPath = "D:\\Scripts\\Example\\�β�";
    //string folderPath = "D:\\Scripts\\Example\\�ɹ�ͷ\\0";
    cout << "Loading DICOM series from: " << folderPath << "\n";
    
    // ���� Volume
    VolumeHandle volume = Dicom_Volume_Create();
    if (!volume) {
        cout << "Failed to create volume\n";
        return;
    }
    
    cout << "Loading DICOM series into volume... (501 files, please wait)\n";
    
    NativeResult result = Dicom_Volume_LoadFromDicomSeries(volume, folderPath.c_str());
    if (result != NATIVE_OK) {
        cout << "Failed to load DICOM series: " << Dicom_GetLastError() << "\n";
        Dicom_Volume_Destroy(volume);
        return;
    }
    
    // ��ȡ��������Ϣ
    int width, height, depth;
    Dicom_Volume_GetDimensions(volume, &width, &height, &depth);
    cout << "\n=== Volume Information ===\n";
    cout << "  Dimensions (�ߴ�): " << width << " x " << height << " x " << depth << "\n";
    
    float spacingX, spacingY, spacingZ;
    Dicom_Volume_GetSpacing(volume, &spacingX, &spacingY, &spacingZ);
    cout << "  Spacing (���): " << spacingX << " x " << spacingY << " x " << spacingZ << " mm\n";
    
    float sizeX = width * spacingX;
    float sizeY = height * spacingY;
    float sizeZ = depth * spacingZ;
    cout << "  Physical Size (�����ߴ�): " << sizeX << " x " << sizeY << " x " << sizeZ << " mm\n";
    
    size_t totalVoxels = (size_t)width * height * depth;
    size_t memoryMB = (totalVoxels * sizeof(short)) / (1024 * 1024);
    cout << "  Total Voxels: " << totalVoxels << "\n";
    cout << "  Memory Usage: " << memoryMB << " MB\n";
    
    // ���� Volume ���������ʹ��
    cout << "\nVolume loaded successfully! (Handle: " << volume << ")\n";
    cout << "You can now use 'viz-mpr' or 'viz-3d' to visualize this volume.\n";
    
    // ע�⣺��Ҫ�������٣�������������Ⱦʹ��
    // Dicom_Volume_Destroy(volume);
}

// ==================== Visualization ���Ժ��� ====================

// ȫ�� Volume ��������ڶ�����Ժ���������
static VolumeHandle g_Volume = nullptr;

void LoadVolumeIfNeeded() {
    if (g_Volume != nullptr) {
        return; // �Ѿ�����
    }
    
    cout << "Loading volume data first...\n";
    string folderPath = "D:\\Scripts\\Example\\�β�";
    //string folderPath = "D:\\Scripts\\Example\\�ɹ�ͷ\\0"; 
    g_Volume = Dicom_Volume_Create();
    if (!g_Volume) {
        cout << "Failed to create volume\n";
        return;
    }
    
    cout << "Loading 501 DICOM files... (this may take a while)\n";
    NativeResult result = Dicom_Volume_LoadFromDicomSeries(g_Volume, folderPath.c_str());
    if (result != NATIVE_OK) {
        cout << "Failed to load DICOM series: " << Dicom_GetLastError() << "\n";
        Dicom_Volume_Destroy(g_Volume);
        g_Volume = nullptr;
        return;
    }
    
    int width, height, depth;
    Dicom_Volume_GetDimensions(g_Volume, &width, &height, &depth);
    cout << "Volume loaded: " << width << "x" << height << "x" << depth << "\n";
}

void TestMPRRendering() {
    cout << "\n=== Testing MPR Rendering ===\n";
    
    // ȷ�� Volume �Ѽ���
    LoadVolumeIfNeeded();
    if (!g_Volume) {
        cout << "Volume not loaded, cannot render MPR\n";
        return;
    }
    
    // ���� MPR ��Ⱦ��
    MPRHandle mpr = MPR_Create();
    if (!mpr) {
        cout << "Failed to create MPR renderer: " << Visualization_GetLastError() << "\n";
        return;
    }
    
    // ���� Volume
    if (MPR_SetVolume(mpr, g_Volume) != NATIVE_OK) {
        cout << "Failed to set volume: " << Visualization_GetLastError() << "\n";
        MPR_Destroy(mpr);
        return;
    }
    
    // ��ȡ Volume �ߴ�
    int width, height, depth;
    Dicom_Volume_GetDimensions(g_Volume, &width, &height, &depth);
    
    // �������ĵ㣨Volume ���ģ�
    MPR_SetCenter(mpr, width / 2.0f, height / 2.0f, depth / 2.0f);
    cout << "MPR center set to: (" << width/2 << ", " << height/2 << ", " << depth/2 << ")\n";
    
    // ��ʾ��λ��
    MPR_SetShowCrossHair(mpr, true);
    
    // �������߹������Ͳ�������
    ToolManagerHandle toolMgr = ToolManager_Create();
    ToolHandle activeTool = Tool_CreateLine(toolMgr);  // Ĭ��ʹ���߶ι���
    
    // ��������
    WindowHandle window = Window_Create(800, 600, "MPR Viewer - Femoral Head");
    if (!window) {
        cout << "Failed to create window: " << Visualization_GetLastError() << "\n";
        MPR_Destroy(mpr);
        ToolManager_Destroy(toolMgr);
        return;
    }
    
    // �� MPR ��Ⱦ��������
    if (Window_BindRenderer(window, mpr, 1) != NATIVE_OK) { // 1 = MPR type
        cout << "Failed to bind renderer: " << Visualization_GetLastError() << "\n";
        Window_Destroy(window);
        MPR_Destroy(mpr);
        ToolManager_Destroy(toolMgr);
        return;
    }
    
    // �󶨲������ߵ�����
    Window_SetToolManager(window, toolMgr);
    Window_SetActiveTool(window, activeTool);
    Window_SetToolType(window, 1);  // 1 = �߶ι���
    
    cout << "MPR window created successfully!\n";
    cout << "Controls:\n";
    cout << "  - Arrow Keys: Move center point (��λ���ƶ�)\n";
    cout << "  - Page Up/Down: Change slice (�л���Ƭ)\n";
    cout << "  - 0: Toggle between CrossHair and Measurement tools (��λ��/���������л�)\n";
    cout << "  - 1-6: Select measurement tools (ѡ���������)\n";
    cout << "    1: Line (ֱ�߲�࣬�϶�����)\n";
    cout << "    2: Angle (�ǶȲ��������3��)\n";
    cout << "    3: Rectangle (���Σ��϶�����)\n";
    cout << "    4: Circle (Բ�Σ��϶�����)\n";
    cout << "    5: Spline (�������ߣ���ε��+˫������)\n";
    cout << "    6: Freehand (�������ߣ��϶�����)\n";
    cout << "  - Shift: Hold for square/circle constraint (Լ��Ϊ������/��Բ)\n";
    cout << "  - Right Click: Cancel current drawing (ȡ����ǰ����)\n";
    cout << "  - Ctrl+C: Clear all measurements (������в���)\n";
    cout << "  - Press ESC or close window to exit\n";
    
    // �����¼�ѭ��
    while (Window_PollEvents(window)) {
        MPR_Render(mpr);
        Window_Refresh(window);
        Sleep(16); // ~60 FPS
    }
    
    // ����
    Window_Destroy(window);
    MPR_Destroy(mpr);
    ToolManager_Destroy(toolMgr);
    cout << "MPR window closed\n";
}

void TestMPRAxial() {
    cout << "\n=== Testing MPR Axial View (XY Plane, Z-Slice) ===\n";
    
    LoadVolumeIfNeeded();
    if (!g_Volume) {
        cout << "Volume not loaded\n";
        return;
    }
    
    int width, height, depth;
    Dicom_Volume_GetDimensions(g_Volume, &width, &height, &depth);
    
    MPRHandle mpr = MPR_Create();
    if (!mpr) {
        cout << "Failed to create MPR\n";
        return;
    }
    
    MPR_SetVolume(mpr, g_Volume);
    MPR_SetSliceDirection(mpr, (MPRSliceDirection)0); // Axial
    
    float cx = width / 2.0f, cy = height / 2.0f, cz = depth / 2.0f;
    MPR_SetCenter(mpr, cx, cy, cz);
    
    cout << "Axial View - Volume: " << width << "x" << height << "x" << depth << "\n";
    cout << "Initial Z slice: " << (int)cz << "/" << depth << "\n";
    
    WindowHandle window = Window_Create(512, 512, "MPR - Axial (XY Plane)");
    if (!window) {
        cout << "Failed to create window\n";
        MPR_Destroy(mpr);
        return;
    }
    
    Window_BindRenderer(window, mpr, 1);
    
    cout << "Controls: [A/D] - Navigate slices, [R] - Reset, [ESC] - Exit\n";
    
    while (Window_PollEvents(window)) {
        if (GetAsyncKeyState('A') & 0x8000) {
            cz = std::max(cz - 1.0f, 0.0f);
            MPR_SetCenter(mpr, cx, cy, cz);
            Sleep(50);
        }
        if (GetAsyncKeyState('D') & 0x8000) {
            cz = std::min(cz + 1.0f, (float)(depth - 1));
            MPR_SetCenter(mpr, cx, cy, cz);
            Sleep(50);
        }
        if (GetAsyncKeyState('R') & 0x8000) {
            cz = depth / 2.0f;
            MPR_SetCenter(mpr, cx, cy, cz);
            Sleep(200);
        }
        
        MPR_Render(mpr);
        Window_Refresh(window);
        Sleep(16);
    }
    
    Window_Destroy(window);
    MPR_Destroy(mpr);
    cout << "Axial view closed\n";
}

void TestMPRCoronal() {
    cout << "\n=== Testing MPR Coronal View (XZ Plane, Y-Slice) ===\n";
    
    LoadVolumeIfNeeded();
    if (!g_Volume) {
        cout << "Volume not loaded\n";
        return;
    }
    
    int width, height, depth;
    Dicom_Volume_GetDimensions(g_Volume, &width, &height, &depth);
    
    MPRHandle mpr = MPR_Create();
    if (!mpr) {
        cout << "Failed to create MPR\n";
        return;
    }
    
    MPR_SetVolume(mpr, g_Volume);
    MPR_SetSliceDirection(mpr, (MPRSliceDirection)1); // Coronal
    
    float cx = width / 2.0f, cy = height / 2.0f, cz = depth / 2.0f;
    MPR_SetCenter(mpr, cx, cy, cz);
    
    cout << "Coronal View - Volume: " << width << "x" << height << "x" << depth << "\n";
    cout << "Initial Y slice: " << (int)cy << "/" << height << "\n";
    
    WindowHandle window = Window_Create(512, 512, "MPR - Coronal (XZ Plane)");
    if (!window) {
        cout << "Failed to create window\n";
        MPR_Destroy(mpr);
        return;
    }
    
    Window_BindRenderer(window, mpr, 1);
    
    cout << "Controls: [A/D] - Navigate slices, [R] - Reset, [ESC] - Exit\n";
    
    while (Window_PollEvents(window)) {
        if (GetAsyncKeyState('A') & 0x8000) {
            cy = std::max(cy - 1.0f, 0.0f);
            MPR_SetCenter(mpr, cx, cy, cz);
            Sleep(50);
        }
        if (GetAsyncKeyState('D') & 0x8000) {
            cy = std::min(cy + 1.0f, (float)(height - 1));
            MPR_SetCenter(mpr, cx, cy, cz);
            Sleep(50);
        }
        if (GetAsyncKeyState('R') & 0x8000) {
            cy = height / 2.0f;
            MPR_SetCenter(mpr, cx, cy, cz);
            Sleep(200);
        }
        
        MPR_Render(mpr);
        Window_Refresh(window);
        Sleep(16);
    }
    
    Window_Destroy(window);
    MPR_Destroy(mpr);
    cout << "Coronal view closed\n";
}

void TestMPRSagittal() {
    cout << "\n=== Testing MPR Sagittal View (YZ Plane, X-Slice) ===\n";
    
    LoadVolumeIfNeeded();
    if (!g_Volume) {
        cout << "Volume not loaded\n";
        return;
    }
    
    int width, height, depth;
    Dicom_Volume_GetDimensions(g_Volume, &width, &height, &depth);
    
    MPRHandle mpr = MPR_Create();
    if (!mpr) {
        cout << "Failed to create MPR\n";
        return;
    }
    
    MPR_SetVolume(mpr, g_Volume);
    MPR_SetSliceDirection(mpr, (MPRSliceDirection)2); // Sagittal
    
    float cx = width / 2.0f, cy = height / 2.0f, cz = depth / 2.0f;
    MPR_SetCenter(mpr, cx, cy, cz);
    
    cout << "Sagittal View - Volume: " << width << "x" << height << "x" << depth << "\n";
    cout << "Initial X slice: " << (int)cx << "/" << width << "\n";
    
    WindowHandle window = Window_Create(512, 512, "MPR - Sagittal (YZ Plane)");
    if (!window) {
        cout << "Failed to create window\n";
        MPR_Destroy(mpr);
        return;
    }
    
    Window_BindRenderer(window, mpr, 1);
    
    cout << "Controls: [A/D] - Navigate slices, [R] - Reset, [ESC] - Exit\n";
    
    while (Window_PollEvents(window)) {
        if (GetAsyncKeyState('A') & 0x8000) {
            cx = std::max(cx - 1.0f, 0.0f);
            MPR_SetCenter(mpr, cx, cy, cz);
            Sleep(50);
        }
        if (GetAsyncKeyState('D') & 0x8000) {
            cx = std::min(cx + 1.0f, (float)(width - 1));
            MPR_SetCenter(mpr, cx, cy, cz);
            Sleep(50);
        }
        if (GetAsyncKeyState('R') & 0x8000) {
            cx = width / 2.0f;
            MPR_SetCenter(mpr, cx, cy, cz);
            Sleep(200);
        }
        
        MPR_Render(mpr);
        Window_Refresh(window);
        Sleep(16);
    }
    
    Window_Destroy(window);
    MPR_Destroy(mpr);
    cout << "Sagittal view closed\n";
}

void TestAPRRendering() {
    cout << "\n=== Testing APR Rendering ===\n";
    
    // ȷ�� Volume �Ѽ���
    LoadVolumeIfNeeded();
    if (!g_Volume) {
        cout << "Volume not loaded, cannot render APR\n";
        return;
    }
    
    // ���� APR ��Ⱦ��
    APRHandle apr = APR_Create();
    if (!apr) {
        cout << "Failed to create APR renderer: " << Visualization_GetLastError() << "\n";
        return;
    }
    
    // ���� Volume
    if (APR_SetVolume(apr, g_Volume) != NATIVE_OK) {
        cout << "Failed to set volume: " << Visualization_GetLastError() << "\n";
        APR_Destroy(apr);
        return;
    }
    
    // ��ȡ Volume �ߴ�
    int width, height, depth;
    Dicom_Volume_GetDimensions(g_Volume, &width, &height, &depth);
    
    // �������ĵ����ת
    APR_SetCenter(apr, width / 2.0f, height / 2.0f, depth / 2.0f);
    APR_SetRotation(apr, 0.0f, 0.0f, 0.0f);
    APR_SetShowCrossHair(apr, true);
    
    cout << "APR configured with rotation support\n";
    
    // �������߹������Ͳ�������
    ToolManagerHandle toolMgr = ToolManager_Create();
    ToolHandle activeTool = Tool_CreateLine(toolMgr);  // Ĭ��ʹ���߶ι���
    
    // ��������
    WindowHandle window = Window_Create(800, 600, "APR Viewer - Femoral Head");
    if (!window) {
        cout << "Failed to create window: " << Visualization_GetLastError() << "\n";
        APR_Destroy(apr);
        ToolManager_Destroy(toolMgr);
        return;
    }
    
    // ����Ⱦ��
    if (Window_BindRenderer(window, apr, 0) != NATIVE_OK) { // 0 = APR type
        cout << "Failed to bind renderer: " << Visualization_GetLastError() << "\n";
        Window_Destroy(window);
        APR_Destroy(apr);
        ToolManager_Destroy(toolMgr);
        return;
    }
    
    // �󶨲������ߵ�����
    Window_SetToolManager(window, toolMgr);
    Window_SetActiveTool(window, activeTool);
    Window_SetToolType(window, 1);  // 1 = �߶ι���
    
    cout << "APR window created successfully!\n";
    cout << "Controls:\n";
    cout << "  - Arrow Keys: Rotate plane (��ת����)\n";
    cout << "  - Page Up/Down: Move along normal (�ط����ƶ�)\n";
    cout << "  - R: Reset rotation (������ת)\n";
    cout << "  - 0: Toggle between CrossHair and Measurement tools (��λ��/���������л�)\n";
    cout << "  - 1-6: Select measurement tools (ѡ���������)\n";
    cout << "    1: Line (ֱ�߲�࣬�϶�����)\n";
    cout << "    2: Angle (�ǶȲ��������3��)\n";
    cout << "    3: Rectangle (���Σ��϶�����)\n";
    cout << "    4: Circle (Բ�Σ��϶�����)\n";
    cout << "    5: Spline (�������ߣ���ε��+˫������)\n";
    cout << "    6: Freehand (�������ߣ��϶�����)\n";
    cout << "  - Shift: Hold for square/circle constraint (Լ��Ϊ������/��Բ)\n";
    cout << "  - Right Click: Cancel current drawing (ȡ����ǰ����)\n";
    cout << "  - Ctrl+C: Clear all measurements (������в���)\n";
    cout << "  - Press ESC or close window to exit\n";
    
    // �����¼�ѭ��
    while (Window_PollEvents(window)) {
        APR_Render(apr);
        Window_Refresh(window);
        Sleep(16); // ~60 FPS
    }
    
    // ����
    Window_Destroy(window);
    APR_Destroy(apr);
    ToolManager_Destroy(toolMgr);
    cout << "APR window closed\n";
}

void TestMPRTripleView() {
    cout << "\n=== Testing MPR Triple View (Axial + Coronal + Sagittal) ===\n";
    
    // ȷ�� Volume �Ѽ���
    LoadVolumeIfNeeded();
    if (!g_Volume) {
        cout << "Volume not loaded, cannot render MPR\n";
        return;
    }
    
    // ��ȡ Volume �ߴ�
    int width, height, depth;
    Dicom_Volume_GetDimensions(g_Volume, &width, &height, &depth);
    
    // �������� MPR ��Ⱦ�������򡢹�״��ʸ״��
    MPRHandle mprAxial = MPR_Create();
    MPRHandle mprCoronal = MPR_Create();
    MPRHandle mprSagittal = MPR_Create();
    
    if (!mprAxial || !mprCoronal || !mprSagittal) {
        cout << "Failed to create MPR renderers\n";
        if (mprAxial) MPR_Destroy(mprAxial);
        if (mprCoronal) MPR_Destroy(mprCoronal);
        if (mprSagittal) MPR_Destroy(mprSagittal);
        return;
    }
    
    // ���� Volume
    MPR_SetVolume(mprAxial, g_Volume);
    MPR_SetVolume(mprCoronal, g_Volume);
    MPR_SetVolume(mprSagittal, g_Volume);
    
    // ������Ƭ����
    MPR_SetSliceDirection(mprAxial, (MPRSliceDirection)0);    // Axial
    MPR_SetSliceDirection(mprCoronal, (MPRSliceDirection)1);  // Coronal
    MPR_SetSliceDirection(mprSagittal, (MPRSliceDirection)2); // Sagittal
    
    // �������ĵ�
    float cx = width / 2.0f, cy = height / 2.0f, cz = depth / 2.0f;
    MPR_SetCenter(mprAxial, cx, cy, cz);
    MPR_SetCenter(mprCoronal, cx, cy, cz);
    MPR_SetCenter(mprSagittal, cx, cy, cz);
    
    cout << "Volume dimensions: " << width << "x" << height << "x" << depth << "\n";
    cout << "Center point: (" << cx << ", " << cy << ", " << cz << ")\n";
    cout << "Axial will show Z=" << (int)cz << "/" << depth << "\n";
    cout << "Coronal will show Y=" << (int)cy << "/" << height << "\n";
    cout << "Sagittal will show X=" << (int)cx << "/" << width << "\n";
    
    // ������������
    WindowHandle winAxial = Window_Create(512, 512, "MPR - Axial (XY Plane, Z-Slice)");
    WindowHandle winCoronal = Window_Create(512, 512, "MPR - Coronal (XZ Plane, Y-Slice)");
    WindowHandle winSagittal = Window_Create(512, 512, "MPR - Sagittal (YZ Plane, X-Slice)");
    
    if (!winAxial || !winCoronal || !winSagittal) {
        cout << "Failed to create windows\n";
        if (winAxial) Window_Destroy(winAxial);
        if (winCoronal) Window_Destroy(winCoronal);
        if (winSagittal) Window_Destroy(winSagittal);
        MPR_Destroy(mprAxial);
        MPR_Destroy(mprCoronal);
        MPR_Destroy(mprSagittal);
        return;
    }
    
    // ����Ⱦ��
    Window_BindRenderer(winAxial, mprAxial, 1);
    Window_BindRenderer(winCoronal, mprCoronal, 1);
    Window_BindRenderer(winSagittal, mprSagittal, 1);
    
    // �������� MPR��ʹ���ǹ���ͬһ�����ĵ�
    MPRHandle mprs[] = { mprAxial, mprCoronal, mprSagittal };
    MPR_LinkCenter(mprs, 3);
    
    // �����������߹�����
    ToolManagerHandle toolMgr = ToolManager_Create();
    ToolHandle activeTool = Tool_CreateLine(toolMgr);
    
    // Ϊ�����������ù��߹�����
    Window_SetToolManager(winAxial, toolMgr);
    Window_SetActiveTool(winAxial, activeTool);
    Window_SetToolType(winAxial, 0);  // Ĭ���Ƕ�λ�߹���
    
    Window_SetToolManager(winCoronal, toolMgr);
    Window_SetActiveTool(winCoronal, activeTool);
    Window_SetToolType(winCoronal, 0);
    
    Window_SetToolManager(winSagittal, toolMgr);
    Window_SetActiveTool(winSagittal, activeTool);
    Window_SetToolType(winSagittal, 0);
    
    cout << "MPR Triple View created!\n";
    cout << "Controls:\n";
    cout << "  [0]   - Toggle CrossHair / Measurement Tools\n";
    cout << "  [1-6] - Select Measurement Tool (1=Line, 2=Angle, 3=Rect, 4=Circle, 5=Spline, 6=Freehand)\n";
    cout << "  [W/S] - Navigate Z slices (Axial view)\n";
    cout << "  [Q/E] - Navigate Y slices (Coronal view)\n";
    cout << "  [A/D] - Navigate X slices (Sagittal view)\n";
    cout << "  [R]   - Reset to center\n";
    cout << "  [Ctrl+C] - Clear all measurements\n";
    cout << "  [ESC] - Exit\n\n";
    
    // �¼�ѭ��
    bool running = true;
    while (running) {
        // ��鴰��
        if (!Window_PollEvents(winAxial) || !Window_PollEvents(winCoronal) || !Window_PollEvents(winSagittal)) {
            running = false;
            break;
        }
        
        // �� MPR ��ȡ��ǰ���ĵ㣨���ܱ�����޸ģ�
        MPR_GetCenter(mprAxial, &cx, &cy, &cz);
        
        // �������봦����ֻ�����һ�� MPR_SetCenter�����Զ�ͬ����
        if (GetAsyncKeyState('W') & 0x8000) {
            cz = std::min(cz + 1.0f, (float)(depth - 1));
            MPR_SetCenter(mprAxial, cx, cy, cz);
            Sleep(50); // ����
        }
        if (GetAsyncKeyState('S') & 0x8000) {
            cz = std::max(cz - 1.0f, 0.0f);
            MPR_SetCenter(mprAxial, cx, cy, cz);
            Sleep(50); // ����
        }
        if (GetAsyncKeyState('Q') & 0x8000) {
            cy = std::min(cy + 1.0f, (float)(height - 1));
            MPR_SetCenter(mprAxial, cx, cy, cz);
            Sleep(50); // ����
        }
        if (GetAsyncKeyState('E') & 0x8000) {
            cy = std::max(cy - 1.0f, 0.0f);
            MPR_SetCenter(mprAxial, cx, cy, cz);
            Sleep(50); // ����
        }
        if (GetAsyncKeyState('A') & 0x8000) {
            cx = std::max(cx - 1.0f, 0.0f);
            MPR_SetCenter(mprAxial, cx, cy, cz);
            Sleep(50); // ����
        }
        if (GetAsyncKeyState('D') & 0x8000) {
            cx = std::min(cx + 1.0f, (float)(width - 1));
            MPR_SetCenter(mprAxial, cx, cy, cz);
            Sleep(50); // ����
        }
        if (GetAsyncKeyState('R') & 0x8000) {
            cx = width / 2.0f;
            cy = height / 2.0f;
            cz = depth / 2.0f;
            MPR_SetCenter(mprAxial, cx, cy, cz);
            Sleep(200);
        }
        
        // Z/X: ���ſ��ƣ����д���ͬ�����ţ�
        static float zoom = 1.0f;
        if (GetAsyncKeyState('Z') & 0x8000) {
            zoom *= 1.1f;  // �Ŵ� 10%
            if (zoom > 10.0f) zoom = 10.0f;
            MPR_SetZoom(mprAxial, zoom);
            MPR_SetZoom(mprCoronal, zoom);
            MPR_SetZoom(mprSagittal, zoom);
            Sleep(100);
        }
        if (GetAsyncKeyState('X') & 0x8000) {
            zoom /= 1.1f;  // ��С 10%
            if (zoom < 0.1f) zoom = 0.1f;
            MPR_SetZoom(mprAxial, zoom);
            MPR_SetZoom(mprCoronal, zoom);
            MPR_SetZoom(mprSagittal, zoom);
            Sleep(100);
        }
        // C: ��������
        if (GetAsyncKeyState('C') & 0x8000) {
            zoom = 1.0f;
            MPR_SetZoom(mprAxial, zoom);
            MPR_SetZoom(mprCoronal, zoom);
            MPR_SetZoom(mprSagittal, zoom);
            Sleep(200);
        }
        
        // ��Ⱦ��ÿ���������Լ�������������Ⱦ��
        // Axial
        Window_MakeCurrent(winAxial);
        MPR_Render(mprAxial);
        Window_Refresh(winAxial);
        
        // Coronal
        Window_MakeCurrent(winCoronal);
        MPR_Render(mprCoronal);
        Window_Refresh(winCoronal);
        
        // Sagittal
        Window_MakeCurrent(winSagittal);
        MPR_Render(mprSagittal);
        Window_Refresh(winSagittal);
        
        Sleep(16);
    }
    
    // ����
    ToolManager_Destroy(toolMgr);
    Window_Destroy(winAxial);
    Window_Destroy(winCoronal);
    Window_Destroy(winSagittal);
    MPR_Destroy(mprAxial);
    MPR_Destroy(mprCoronal);
    MPR_Destroy(mprSagittal);
    cout << "MPR Triple View closed\n";
}

void TestAPRTripleView() {
    cout << "\n=== Testing APR Triple View + 3D Orthogonal View ===\n";
    
    // ȷ�� Volume �Ѽ���
    LoadVolumeIfNeeded();
    if (!g_Volume) {
        cout << "Volume not loaded, cannot render APR\n";
        return;
    }
    
    // ��ȡ Volume �ߴ�
    int width, height, depth;
    Dicom_Volume_GetDimensions(g_Volume, &width, &height, &depth);
    
    // �����ĸ� APR ��Ⱦ��������2D��ͼ + һ��3D������ͼ��
    APRHandle aprAxial = APR_Create();
    APRHandle aprCoronal = APR_Create();
    APRHandle aprSagittal = APR_Create();
    APRHandle apr3D = APR_Create();  // ������3D ������ͼ
    
    if (!aprAxial || !aprCoronal || !aprSagittal || !apr3D) {
        cout << "Failed to create APR renderers\n";
        if (aprAxial) APR_Destroy(aprAxial);
        if (aprCoronal) APR_Destroy(aprCoronal);
        if (aprSagittal) APR_Destroy(aprSagittal);
        if (apr3D) APR_Destroy(apr3D);
        return;
    }
    
    // ���� Volume��������ͼ��
    APR_SetVolume(aprAxial, g_Volume);
    APR_SetVolume(aprCoronal, g_Volume);
    APR_SetVolume(aprSagittal, g_Volume);
    APR_SetVolume(apr3D, g_Volume);
    
    // ������Ƭ����2D ��ͼ��
    APR_SetSliceDirection(aprAxial, 0);    // Axial
    APR_SetSliceDirection(aprCoronal, 1);  // Coronal
    APR_SetSliceDirection(aprSagittal, 2); // Sagittal
    APR_SetSliceDirection(apr3D, 0);       // 3D��ͼҲ��ΪAxial��Ϊ��׼
    
    // �������ĵ�
    float cx = width / 2.0f, cy = height / 2.0f, cz = depth / 2.0f;
    APR_SetCenter(aprAxial, cx, cy, cz);
    APR_SetCenter(aprCoronal, cx, cy, cz);
    APR_SetCenter(aprSagittal, cx, cy, cz);
    APR_SetCenter(apr3D, cx, cy, cz);
    
    cout << "Volume dimensions: " << width << "x" << height << "x" << depth << "\n";
    cout << "Center point: (" << cx << ", " << cy << ", " << cz << ")\n";
    
    // �����ĸ����ڣ�����2D + һ��3D��
    WindowHandle winAxial = Window_Create(512, 512, "APR - Axial (XY Plane)");
    WindowHandle winCoronal = Window_Create(512, 512, "APR - Coronal (XZ Plane)");
    WindowHandle winSagittal = Window_Create(512, 512, "APR - Sagittal (YZ Plane)");
    WindowHandle win3D = Window_Create(600, 600, "APR - 3D Orthogonal View");  // ����
    
    if (!winAxial || !winCoronal || !winSagittal || !win3D) {
        cout << "Failed to create windows\n";
        if (winAxial) Window_Destroy(winAxial);
        if (winCoronal) Window_Destroy(winCoronal);
        if (winSagittal) Window_Destroy(winSagittal);
        if (win3D) Window_Destroy(win3D);
        APR_Destroy(aprAxial);
        APR_Destroy(aprCoronal);
        APR_Destroy(aprSagittal);
        APR_Destroy(apr3D);
        return;
    }
    
    // ����Ⱦ����rendererType=2 ��ʾ APR��
    Window_BindRenderer(winAxial, aprAxial, 2);
    Window_BindRenderer(winCoronal, aprCoronal, 2);
    Window_BindRenderer(winSagittal, aprSagittal, 2);
    // ע�⣺3D ���ڲ�����Ⱦ����ʹ���ֶ����� APR_RenderOrthogonal3D
    
    // �������� 2D APR��ʹ���ǹ���ͬһ�����ĵ����ת
    APRHandle aprs[] = { aprAxial, aprCoronal, aprSagittal };
    APR_LinkCenter(aprs, 3);
    
    // �����������߹�����
    ToolManagerHandle toolMgr = ToolManager_Create();
    ToolHandle activeTool = Tool_CreateLine(toolMgr);
    
    // Ϊ����2D�������ù��߹�����
    Window_SetToolManager(winAxial, toolMgr);
    Window_SetActiveTool(winAxial, activeTool);
    Window_SetToolType(winAxial, 0);  // Ĭ���Ƕ�λ�߹���
    
    Window_SetToolManager(winCoronal, toolMgr);
    Window_SetActiveTool(winCoronal, activeTool);
    Window_SetToolType(winCoronal, 0);
    
    Window_SetToolManager(winSagittal, toolMgr);
    Window_SetActiveTool(winSagittal, activeTool);
    Window_SetToolType(winSagittal, 0);
    
    // ��ʼ�����п�ʹ�������ݳߴ磩
    APR_SetCropBox(width, height, depth);
    APR_EnableCropBox(true);
    
    cout << "APR Triple View + 3D View created!\n";
    cout << "Controls:\n";
    cout << "  [0]   - Toggle CrossHair / Measurement Tools\n";
    cout << "  [1-6] - Select Measurement Tool (1=Line, 2=Angle, 3=Rect, 4=Circle, 5=Spline, 6=Freehand)\n";
    cout << "  [Mouse Left Drag]   - Move crosshair (when tool 0) or Draw measurement (tool 1-6)\n";
    cout << "  [Mouse Right Drag (3D Window)] - Rotate 3D view\n";
    cout << "  [Shift + Left Drag] - Move crop box (when mouse inside box)\n";
    cout << "  [Drag Control Points] - Resize crop box (red corners / cyan edges)\n";
    cout << "  [Mouse Scroll]      - Navigate slices in current view\n";
    cout << "  [Z/X]               - Zoom in/out (all views)\n";
    cout << "  [Ctrl+C]            - Clear all measurements\n";
    cout << "  [W/S]               - Rotate around X-axis (all views update)\n";
    cout << "  [A/D]               - Rotate around Y-axis (all views update)\n";
    cout << "  [Q/E]               - Rotate around Z-axis (all views update)\n";
    cout << "  [R]                 - Reset rotation and center\n";
    cout << "  [B]                 - Toggle crop box\n";
    cout << "  [V (in window)]     - Execute crop (Volume)\n";
    cout << "  [ESC]               - Exit\n\n";
    cout << "ע�⣺3D ������ʾ���� APR ƽ���໥��ֱ���У��ṩ����ռ��\n\n";
    
    // �¼�ѭ��
    bool running = true;
    while (running) {
        // ������д��ڣ�����3D���ڣ�
        if (!Window_PollEvents(winAxial) || !Window_PollEvents(winCoronal) || 
            !Window_PollEvents(winSagittal) || !Window_PollEvents(win3D)) {
            running = false;
            break;
        }
        
        // ��Ⱦ��ÿ���������Լ�������������Ⱦ��
        // Axial
        Window_MakeCurrent(winAxial);
        APR_Render(aprAxial);
        Window_Refresh(winAxial);
        
        // Coronal
        Window_MakeCurrent(winCoronal);
        APR_Render(aprCoronal);
        Window_Refresh(winCoronal);
        
        // Sagittal
        Window_MakeCurrent(winSagittal);
        APR_Render(aprSagittal);
        Window_Refresh(winSagittal);
        
        // 3D Orthogonal View (��������ʾ����ƽ�洹ֱ����)
        Window_MakeCurrent(win3D);
        // ʹ�������3D������Ⱦ����������APR���
        NativeResult renderResult = APR_RenderOrthogonal3D(aprAxial, aprCoronal, aprSagittal);
        if (renderResult != NATIVE_OK) {
            cout << "APR_RenderOrthogonal3D failed with code " << renderResult << "\n";
        }
        Window_Refresh(win3D);
        
        Sleep(16);
    }
    
    // ����������3D���ڣ�
    ToolManager_Destroy(toolMgr);
    Window_Destroy(winAxial);
    Window_Destroy(winCoronal);
    Window_Destroy(winSagittal);
    Window_Destroy(win3D);
    APR_Destroy(aprAxial);
    APR_Destroy(aprCoronal);
    APR_Destroy(aprSagittal);
    APR_Destroy(apr3D);
    cout << "APR Triple View + 3D View closed\n";
}

void Test3DRendering() {
    cout << "\n=== Testing 3D Volume Rendering ===\n";
    
    // ȷ�� Volume �Ѽ���
    LoadVolumeIfNeeded();
    if (!g_Volume) {
        cout << "Volume not loaded, cannot render 3D\n";
        return;
    }
    
    // ���� 3D ��Ⱦ��
    Volume3DHandle renderer = Volume3D_Create();
    if (!renderer) {
        cout << "Failed to create 3D renderer: " << Visualization_GetLastError() << "\n";
        return;
    }
    
    // ���� Volume
    if (Volume3D_AddVolume(renderer, g_Volume) != NATIVE_OK) {
        cout << "Failed to add volume: " << Visualization_GetLastError() << "\n";
        Volume3D_Destroy(renderer);
        return;
    }
    
    // �������ݺ��������ڹ�����ʾ��
    TransferFunctionHandle tf = TransferFunction_Create();
    if (tf) {
        // �����ĵ��� HU ֵ��Χ: 200-3000
        // ���ӿ��Ƶ㣨value, r, g, b, a��
        TransferFunction_AddControlPoint(tf, 0.0f,    0.0f, 0.0f, 0.0f, 0.0f); // ��ȫ͸��
        TransferFunction_AddControlPoint(tf, 200.0f,  0.8f, 0.8f, 0.6f, 0.1f); // ����ɫ����͸����
        TransferFunction_AddControlPoint(tf, 1000.0f, 1.0f, 1.0f, 0.8f, 0.5f); // ����ɫ����͸����
        TransferFunction_AddControlPoint(tf, 3000.0f, 1.0f, 1.0f, 1.0f, 0.8f); // ����ɫ����͸����
        
        Volume3D_SetTransferFunction(renderer, 0, tf);
        cout << "Transfer function configured for bone rendering\n";
    }
    
    // ���ù��ղ���
    Volume3D_SetLightParameters(renderer, 0.3f, 0.6f, 0.3f); // ambient, diffuse, specular
    
    // ��������
    WindowHandle window = Window_Create(800, 600, "3D Volume Rendering - Femoral Head");
    if (!window) {
        cout << "Failed to create window: " << Visualization_GetLastError() << "\n";
        Volume3D_Destroy(renderer);
        if (tf) TransferFunction_Destroy(tf);
        return;
    }
    
    // ����Ⱦ��
    if (Window_BindRenderer(window, renderer, 2) != NATIVE_OK) { // 2 = 3D type
        cout << "Failed to bind renderer: " << Visualization_GetLastError() << "\n";
        Window_Destroy(window);
        Volume3D_Destroy(renderer);
        if (tf) TransferFunction_Destroy(tf);
        return;
    }
    
    cout << "3D window created successfully!\n";
    cout << "Controls:\n";
    cout << "  - Drag mouse to rotate\n";
    cout << "  - Scroll to zoom\n";
    cout << "  - Press ESC or close window to exit\n";
    
    // �����¼�ѭ��
    while (Window_PollEvents(window)) {
        Volume3D_Render(renderer);
        Window_Refresh(window);
        Sleep(16);
    }
    
    // ����
    Window_Destroy(window);
    Volume3D_Destroy(renderer);
    if (tf) TransferFunction_Destroy(tf);
    cout << "3D window closed\n";
}

void TestVisualizationWindow() {
    cout << "\n=== Testing OpenGL Window (Win32/WGL) ===\n";
    cout << "This will create a standalone window for testing.\n";
    cout << "Note: For Vue integration, use offscreen rendering instead.\n";
    
    WindowHandle window = Window_Create(800, 600, "Medical Image Viewer - Test");
    if (!window) {
        cout << "Failed to create window: " << Visualization_GetLastError() << "\n";
        return;
    }

#ifdef _WIN32
    // Window_Create creates a hidden Win32 window by default (to avoid flashing
    // when embedding into Electron). For standalone tests, show it.
    HWND hwnd = static_cast<HWND>(Window_GetNativeHandle(window));
    if (hwnd) {
        ShowWindow(hwnd, SW_SHOW);
        UpdateWindow(hwnd);
    }
#endif
    
    cout << "Window created! Press ESC or close window to exit.\n";
    
    // Pump messages; on Win32 this exits when the window is closed.
    while (Window_PollEvents(window)) {
        // Trigger async repaint (actual rendering happens in WM_PAINT).
        Window_Invalidate(window);
        Sleep(16); // ~60 FPS
    }
    
    Window_Destroy(window);
    cout << "Window destroyed\n";
}

#ifdef _WIN32
namespace {
    struct HostVizEmbedState {
        WindowHandle vizWindow = nullptr;
    };

    static const wchar_t* kHostWindowClassName = L"ConsoleDllTest.HostWindow";

    static LRESULT CALLBACK HostWindowProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam) {
        auto state = reinterpret_cast<HostVizEmbedState*>(GetWindowLongPtrW(hwnd, GWLP_USERDATA));

        switch (msg) {
        case WM_NCCREATE: {
            auto cs = reinterpret_cast<CREATESTRUCTW*>(lParam);
            SetWindowLongPtrW(hwnd, GWLP_USERDATA, reinterpret_cast<LONG_PTR>(cs->lpCreateParams));
            return TRUE;
        }

        case WM_SIZE:
            if (state && state->vizWindow) {
                RECT rc{};
                GetClientRect(hwnd, &rc);
                int w = rc.right - rc.left;
                int h = rc.bottom - rc.top;
                Window_Resize(state->vizWindow, 0, 0, w, h);
                Window_RefreshZOrder(state->vizWindow);
                Window_Invalidate(state->vizWindow);
            }
            return 0;

        case WM_TIMER:
            if (state && state->vizWindow) {
                Window_Invalidate(state->vizWindow);
            }
            return 0;

        case WM_DESTROY:
            PostQuitMessage(0);
            return 0;
        }

        return DefWindowProcW(hwnd, msg, wParam, lParam);
    }

    static HWND CreateHostWindow(int width, int height, const wchar_t* title, HostVizEmbedState* state) {
        HINSTANCE hInstance = GetModuleHandleW(nullptr);

        static bool registered = false;
        if (!registered) {
            WNDCLASSEXW wc{};
            wc.cbSize = sizeof(wc);
            wc.style = CS_HREDRAW | CS_VREDRAW;
            wc.lpfnWndProc = HostWindowProc;
            wc.hInstance = hInstance;
            wc.hCursor = LoadCursorW(nullptr, IDC_ARROW);
            wc.hbrBackground = reinterpret_cast<HBRUSH>(COLOR_WINDOW + 1);
            wc.lpszClassName = kHostWindowClassName;
            RegisterClassExW(&wc);
            registered = true;
        }

        RECT r{ 0, 0, width, height };
        AdjustWindowRect(&r, WS_OVERLAPPEDWINDOW, FALSE);
        int winW = r.right - r.left;
        int winH = r.bottom - r.top;

        HWND hwnd = CreateWindowExW(
            0,
            kHostWindowClassName,
            title,
            WS_OVERLAPPEDWINDOW,
            CW_USEDEFAULT,
            CW_USEDEFAULT,
            winW,
            winH,
            nullptr,
            nullptr,
            hInstance,
            state);

        if (!hwnd) {
            return nullptr;
        }

        ShowWindow(hwnd, SW_SHOW);
        UpdateWindow(hwnd);
        return hwnd;
    }
}
#endif

void TestHostEmbeddedMPR() {
#ifndef _WIN32
    cout << "viz-host-mpr is Windows-only.\n";
    return;
#else
    cout << "\n=== Testing Host-Style Embedded MPR (HWND child) ===\n";
    cout << "This simulates hiscan-analyzer: host owns the message loop; the OpenGL window is a child HWND.\n";

    LoadVolumeIfNeeded();
    if (!g_Volume) {
        cout << "Volume not loaded, cannot render MPR\n";
        return;
    }

    int volW = 0, volH = 0, volD = 0;
    Dicom_Volume_GetDimensions(g_Volume, &volW, &volH, &volD);

    MPRHandle mpr = MPR_Create();
    if (!mpr) {
        cout << "Failed to create MPR renderer: " << Visualization_GetLastError() << "\n";
        return;
    }
    if (MPR_SetVolume(mpr, g_Volume) != NATIVE_OK) {
        cout << "Failed to set volume: " << Visualization_GetLastError() << "\n";
        MPR_Destroy(mpr);
        return;
    }

    MPR_SetCenter(mpr, volW / 2.0f, volH / 2.0f, volD / 2.0f);
    MPR_SetShowCrossHair(mpr, true);

    ToolManagerHandle toolMgr = ToolManager_Create();
    ToolHandle activeTool = Tool_CreateLine(toolMgr);

    HostVizEmbedState state{};
    HWND host = CreateHostWindow(900, 700, L"Host Embedded MPR (parent HWND + child OpenGL HWND)", &state);
    if (!host) {
        cout << "Failed to create host window\n";
        ToolManager_Destroy(toolMgr);
        MPR_Destroy(mpr);
        return;
    }

    RECT rc{};
    GetClientRect(host, &rc);
    int clientW = rc.right - rc.left;
    int clientH = rc.bottom - rc.top;

    WindowHandle child = Window_Create(clientW, clientH, "Embedded MPR Child");
    if (!child) {
        cout << "Failed to create visualization window: " << Visualization_GetLastError() << "\n";
        DestroyWindow(host);
        ToolManager_Destroy(toolMgr);
        MPR_Destroy(mpr);
        return;
    }

    state.vizWindow = child;

    if (Window_SetParentWindow(child, host, 0, 0, clientW, clientH) != NATIVE_OK) {
        cout << "Failed to embed child window: " << Visualization_GetLastError() << "\n";
        Window_Destroy(child);
        DestroyWindow(host);
        ToolManager_Destroy(toolMgr);
        MPR_Destroy(mpr);
        return;
    }

    if (Window_BindRenderer(child, mpr, 1) != NATIVE_OK) {
        cout << "Failed to bind renderer: " << Visualization_GetLastError() << "\n";
        Window_Destroy(child);
        DestroyWindow(host);
        ToolManager_Destroy(toolMgr);
        MPR_Destroy(mpr);
        return;
    }

    Window_SetToolManager(child, toolMgr);
    Window_SetActiveTool(child, activeTool);
    Window_SetToolType(child, 1);

    // Host drives redraw; actual rendering happens in child WM_PAINT.
    SetTimer(host, 1, 16, nullptr);
    Window_Invalidate(child);

    cout << "Host window running. Close the window to exit.\n";

    MSG msg;
    while (GetMessageW(&msg, nullptr, 0, 0) > 0) {
        TranslateMessage(&msg);
        DispatchMessageW(&msg);
    }

    KillTimer(host, 1);
    Window_Destroy(child);
    ToolManager_Destroy(toolMgr);
    MPR_Destroy(mpr);
    cout << "Host embedded MPR closed\n";
#endif
}

void TestOffscreenRendering() {
    cout << "\n=== Testing Offscreen Rendering (FBO) ===\n";
    cout << "This is the recommended method for Vue integration.\n";
    
    int width = 512, height = 512;
    WindowHandle context = OffscreenContext_Create(width, height);
    if (!context) {
        cout << "Failed to create offscreen context: " << Visualization_GetLastError() << "\n";
        return;
    }
    
    cout << "Offscreen context created (" << width << "x" << height << ")\n";
    cout << "\nFor Vue integration, you would:\n";
    cout << "  1. Render to FBO using OffscreenContext_RenderToBuffer()\n";
    cout << "  2. Get pixel data (RGBA buffer)\n";
    cout << "  3. Pass to Node.js via node-addon-api\n";
    cout << "  4. Convert to Base64 or use SharedArrayBuffer\n";
    cout << "  5. Display in Vue using <img> or <canvas>\n";
    
    OffscreenContext_Destroy(context);
    cout << "Offscreen context destroyed\n";
}

// ==================== DICOM ����ͼ���� ====================
void TestDicomThumbnail() {
    cout << "\n=== DICOM ����ͼ���ɲ��� ===\n";
    std::string folderPath = "D:\\Scripts\\Example\\�β�"; // ����������Ĳ����ļ���·��
    int thumbWidth = 256, thumbHeight = 256;
    cout << "ʹ�ò����ļ���: " << folderPath << "\n";
    cout << "����ͼ���� (Ĭ��256): ";
    string wstr; getline(cin, wstr); if (!wstr.empty()) thumbWidth = stoi(wstr);
    cout << "����ͼ�߶� (Ĭ��256): ";
    string hstr; getline(cin, hstr); if (!hstr.empty()) thumbHeight = stoi(hstr);

    // �Զ����ҵ�һ�� DICOM �ļ�
    std::string dicomPath;
    WIN32_FIND_DATAA findData;
    HANDLE hFind = FindFirstFileA((folderPath + "\\*.dcm").c_str(), &findData);
    if (hFind != INVALID_HANDLE_VALUE) {
        dicomPath = folderPath + "\\" + findData.cFileName;
        FindClose(hFind);
        cout << "�Զ�ѡ�е�һ�� DICOM �ļ�: " << dicomPath << "\n";
    } else {
        cout << "δ�ҵ� DICOM �ļ� (*.dcm)�������ļ���·����\n";
        return;
    }

    DicomHandle handle = Dicom_CreateReader();
    if (!handle) { cout << "����: �޷����� DICOM ��ȡ��\n"; return; }
    if (Dicom_ReadFile(handle, dicomPath.c_str()) != NATIVE_OK) {
        cout << "����: ��ȡ DICOM �ļ�ʧ��: " << Dicom_GetLastError() << "\n";
        Dicom_DestroyReader(handle); return;
    }
    int width = 0, height = 0, depth = 0;
    void* pixelData = Dicom_GetPixelData(handle, &width, &height, &depth);
    if (!pixelData) {
        cout << "����: ��ȡ��������ʧ��: " << Dicom_GetLastError() << "\n";
        Dicom_DestroyReader(handle); return;
    }
    cout << "ԭʼ�ߴ�: " << width << " x " << height << " x " << depth << "\n";
    // ��������ͼ
    short* srcData = static_cast<short*>(pixelData);
    short* thumbData = new short[thumbWidth * thumbHeight];
    float xRatio = static_cast<float>(width) / thumbWidth;
    float yRatio = static_cast<float>(height) / thumbHeight;
    for (int y = 0; y < thumbHeight; ++y) {
        for (int x = 0; x < thumbWidth; ++x) {
            int srcX = std::min(static_cast<int>(x * xRatio), width - 1);
            int srcY = std::min(static_cast<int>(y * yRatio), height - 1);
            thumbData[y * thumbWidth + x] = srcData[srcY * width + srcX];
        }
    }
    // �Ҷȹ�һ��
    unsigned char* grayData = new unsigned char[thumbWidth * thumbHeight];
    short minVal = thumbData[0], maxVal = thumbData[0];
    for (int i = 0; i < thumbWidth * thumbHeight; ++i) {
        if (thumbData[i] < minVal) minVal = thumbData[i];
        if (thumbData[i] > maxVal) maxVal = thumbData[i];
    }
    float range = static_cast<float>(maxVal - minVal);
    if (range < 1.0f) range = 1.0f;
    for (int i = 0; i < thumbWidth * thumbHeight; ++i) {
        float normalized = (thumbData[i] - minVal) / range;
        grayData[i] = static_cast<unsigned char>(normalized * 255);
    }
    // BMP ����
    #pragma pack(push, 1)
    struct BMPFileHeader {
        uint16_t bfType = 0x4D42; // 'BM'
        uint32_t bfSize = 0;
        uint16_t bfReserved1 = 0;
        uint16_t bfReserved2 = 0;
        uint32_t bfOffBits = 0;
    };
    struct BMPInfoHeader {
        uint32_t biSize = 40;
        int32_t  biWidth = 0;
        int32_t  biHeight = 0;
        uint16_t biPlanes = 1;
        uint16_t biBitCount = 8;
        uint32_t biCompression = 0;
        uint32_t biSizeImage = 0;
        int32_t  biXPelsPerMeter = 0;
        int32_t  biYPelsPerMeter = 0;
        uint32_t biClrUsed = 256;
        uint32_t biClrImportant = 256;
    };
    #pragma pack(pop)
    int rowPadding = (4 - (thumbWidth % 4)) % 4;
    int rowSize = thumbWidth + rowPadding;
    BMPFileHeader fileHeader;
    BMPInfoHeader infoHeader;
    infoHeader.biWidth = thumbWidth;
    infoHeader.biHeight = thumbHeight;
    infoHeader.biSizeImage = rowSize * thumbHeight;
    int paletteSize = 256 * 4;
    fileHeader.bfOffBits = sizeof(BMPFileHeader) + sizeof(BMPInfoHeader) + paletteSize;
    fileHeader.bfSize = fileHeader.bfOffBits + infoHeader.biSizeImage;
    {
        std::ofstream file("thumbnail.bmp", std::ios::binary);
        if (!file.is_open()) { cout << "�޷����� BMP �ļ�\n"; goto cleanup; }
        file.write(reinterpret_cast<const char*>(&fileHeader), sizeof(fileHeader));
        file.write(reinterpret_cast<const char*>(&infoHeader), sizeof(infoHeader));
        for (int i = 0; i < 256; ++i) {
            unsigned char color[4] = { (unsigned char)i, (unsigned char)i, (unsigned char)i, 0 }; // B,G,R,0
            file.write(reinterpret_cast<const char*>(color), 4);
        }
        unsigned char* paddingBytes = new unsigned char[rowPadding]();
        for (int y = thumbHeight - 1; y >= 0; --y) {
            file.write(reinterpret_cast<const char*>(&grayData[y * thumbWidth]), thumbWidth);
            if (rowPadding > 0) file.write(reinterpret_cast<const char*>(paddingBytes), rowPadding);
        }
        delete[] paddingBytes;
        file.close();
        cout << "? ����ͼ�ѱ���Ϊ thumbnail.bmp\n";
    }
    // ��ͼƬ
    ShellExecuteA(NULL, "open", "thumbnail.bmp", NULL, NULL, SW_SHOWNORMAL);
    cout << "? �ѵ���ϵͳͼƬ�鿴��\n";
cleanup:
    delete[] thumbData;
    delete[] grayData;
    delete[] static_cast<char*>(pixelData);
    Dicom_DestroyReader(handle);
}

int main()
{
    cout << "=================================================\n";
    cout << "  Medical Image Processing DLL Test Program\n";
    cout << "=================================================\n";
    
    ShowHelp();
    
    map<string, function<void()>> commandMap = {
        // Core
        {"help", ShowHelp},
        {"h", ShowHelp},
        {"core", TestCore},
        {"memory", TestMemory},
        {"log", TestLogger},
        {"thread", TestThreadPool},
        {"timer", TestTimer},
        // DICOM
        {"dicom-read", TestDicomRead},
        {"dicom-series", TestDicomSeries},
        {"dicom-volume", TestDicomVolume},
        {"dicom-thumbnail", TestDicomThumbnail},
        // Visualization
        {"viz-mpr", TestMPRRendering},
        {"viz-mpr-axial", TestMPRAxial},
        {"viz-mpr-coronal", TestMPRCoronal},
        {"viz-mpr-sagittal", TestMPRSagittal},
        {"viz-mpr3", TestMPRTripleView},
        {"viz-apr", TestAPRRendering},
        {"viz-apr3", TestAPRTripleView},
        {"viz-3d", Test3DRendering},
        {"viz-window", TestVisualizationWindow},
        {"viz-host-mpr", TestHostEmbeddedMPR},
        {"viz-offscreen", TestOffscreenRendering},
    /*{"viz-tools", TestMeasurementTools},
    // Image Processing - Mask Operations
    {"mask-create", TestMaskCreate},
    {"mask-threshold", TestMaskThreshold},
    {"mask-boolean", TestMaskBoolean},
    {"mask-morphology", TestMaskMorphology},
    {"mask-connected", TestMaskConnected},
    {"mask-draw", TestMaskDraw},
    {"mask-measure", TestMaskMeasure},
    {"mask-io", TestMaskIO},
    {"mask-all", TestMaskAll},
    {"mask-edit", TestMaskInteractiveEdit},
    // MPR Interactive
    {"mpr-measure", TestMPRMeasurement}*/
    };
    
    string command;
    bool running = true;
    
    while (running) {
        cout << "\nEnter command (type 'help' for options): ";
        getline(cin, command);
        
        // ת��ΪСд
        transform(command.begin(), command.end(), command.begin(), ::tolower);
        
        if (command == "exit" || command == "quit" || command == "q") {
            running = false;
            cout << "Exiting program...\n";
        } else if (commandMap.find(command) != commandMap.end()) {
            try {
                commandMap[command](); // ִ������
            } catch (const exception& e) {
                cout << "Error executing command: " << e.what() << "\n";
            }
        } else if (!command.empty()) {
            cout << "Unknown command: " << command << "\n";
            cout << "Type 'help' to see available commands.\n";
        }
    }
    
    cout << "\nProgram ended.\n";
    return 0;
}

// ���г���: Ctrl + F5 ����� >����ʼִ��(������)���˵�
// ���Գ���: F5 ����� >����ʼ���ԡ��˵�

// ����ʹ�ü���: 
//   1. ʹ�ý��������Դ��������������/�����ļ�
//   2. ʹ���Ŷ���Դ�������������ӵ�Դ�������
//   3. ʹ��������ڲ鿴���������������Ϣ
//   4. ʹ�ô����б����ڲ鿴����
//   5. ת������Ŀ��>����������Դ����µĴ����ļ�����ת������Ŀ��>������������Խ����д����ļ����ӵ���Ŀ
//   6. ��������Ҫ�ٴδ򿪴���Ŀ����ת�����ļ���>���򿪡�>����Ŀ����ѡ�� .sln �ļ�
