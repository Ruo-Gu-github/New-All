#pragma once
#include "../Common/NativeInterfaces.h"

// ǰ������ ImageProcessing ����
typedef void* MaskManagerHandle;

// ������
#ifdef DLLVISUALIZATION_EXPORTS
#define VIZ_API __declspec(dllexport)
#else
#define VIZ_API __declspec(dllimport)
#endif

#ifdef __cplusplus
extern "C" {
#endif

    // ==================== ������ ====================
    /// ��ȡ��������Ϣ
    VIZ_API const char* Visualization_GetLastError();

    // ==================== Tab Session Management ====================
    /// Create or get a tab session context
    VIZ_API NativeResult Session_Create(const char* sessionId);
    /// Destroy a tab session and all its resources (APRs, windows, etc.)
    VIZ_API NativeResult Session_Destroy(const char* sessionId);
    /// Get the tab session's APR center
    VIZ_API NativeResult Session_GetAPRCenter(const char* sessionId, float* x, float* y, float* z);
    /// Set the tab session's APR center
    VIZ_API NativeResult Session_SetAPRCenter(const char* sessionId, float x, float y, float z);
    /// Get 3D view state for a session
    VIZ_API NativeResult Session_Get3DState(const char* sessionId, float* rotX, float* rotY, float* zoom, float* panX, float* panY);
    /// Set 3D view state for a session
    VIZ_API NativeResult Session_Set3DState(const char* sessionId, float rotX, float rotY, float zoom, float panX, float panY);
    /// Get 3D rotation matrix for a session
    VIZ_API NativeResult Session_Get3DRotMat(const char* sessionId, float outMat[16]);
    /// Set 3D rotation matrix for a session
    VIZ_API NativeResult Session_Set3DRotMat(const char* sessionId, const float inMat[16]);
    /// Reset 3D view for a session to default state
    VIZ_API NativeResult Session_Reset3DView(const char* sessionId);

    // ==================== APR (����ƽ���ؽ�) ====================
    /// ���� APR ��Ⱦ��
    VIZ_API APRHandle APR_Create();
    /// ���� APR ��Ⱦ��
    VIZ_API void APR_Destroy(APRHandle handle);
    /// ����������
    VIZ_API NativeResult APR_SetVolume(APRHandle handle, VolumeHandle volume);
    /// ������Ƭ����0=Axial, 1=Coronal, 2=Sagittal��
    VIZ_API void APR_SetSliceDirection(APRHandle handle, int direction);
    /// ��ȡ��Ƭ����
    VIZ_API int APR_GetSliceDirection(APRHandle handle);
    /// �������ĵ�
    VIZ_API void APR_SetCenter(APRHandle handle, float x, float y, float z);
    /// ��ȡ���ĵ�
    VIZ_API void APR_GetCenter(APRHandle handle, float* x, float* y, float* z);
    /// ���Ӷ�� APR��ʹ���ǹ���ͬһ�����ĵ㣨���ڶ���ͼͬ����
    VIZ_API void APR_LinkCenter(APRHandle* handles, int count);
    /// ������ת�Ƕȣ�ŷ���ǣ���X��Y��Z�����ת�Ƕȣ���λ���ȣ�
    VIZ_API void APR_SetRotation(APRHandle handle, float angleX, float angleY, float angleZ);
    /// ��ȡ��ת�Ƕ�
    VIZ_API void APR_GetRotation(APRHandle handle, float* angleX, float* angleY, float* angleZ);
    /// ��ȡָ���������Ƭ��0=����/�����, 1=ʸ״��, 2=��״�棩
    VIZ_API void* APR_GetSlice(APRHandle handle, int direction, int* width, int* height);
    /// 只更新displayBuffer不渲染（用于3D正交视图）
    VIZ_API NativeResult APR_UpdateSlice(APRHandle handle);
    /// ��ʾ/���ض�λ��
    VIZ_API void APR_SetShowCrossHair(APRHandle handle, bool show);
    /// ��ȡ��λ����ʾ״̬
    VIZ_API bool APR_GetShowCrossHair(APRHandle handle);
    /// �����������ӣ�1.0 = ԭʼ��С��>1.0 �Ŵ�<1.0 ��С��
    VIZ_API void APR_SetZoom(APRHandle handle, float zoomFactor);
    /// ��ȡ��������
    VIZ_API float APR_GetZoom(APRHandle handle);

    // Window/level (HU)
    VIZ_API void APR_SetWindowLevel(APRHandle handle, float windowWidth, float windowLevel);
    VIZ_API void APR_GetWindowLevel(APRHandle handle, float* windowWidth, float* windowLevel);
    
    // MIP/MinIP projection mode
    // mode: 0=Normal (single slice), 1=MIP (max intensity), 2=MinIP (min intensity)
    VIZ_API void APR_SetProjectionMode(APRHandle handle, int mode, float thickness);
    VIZ_API void APR_GetProjectionMode(APRHandle handle, int* mode, float* thickness);

    // Reset APR in-plane rotation to default.
    VIZ_API void APR_ResetRotation(APRHandle handle);
        // Bind an APR renderer to a sessionId registered via MPR_RegisterSessionVolume.
        // This allows APR_Render to draw masks/preview masks managed by the MPR session.
        VIZ_API void APR_SetSessionId(APRHandle handle, const char* sessionId);
    /// ��Ⱦ��ˢ�£�
    VIZ_API NativeResult APR_Render(APRHandle handle);
    /// 设置3D正交视图模式（同时显示三个切片平面）
    VIZ_API void APR_SetOrthogonal3DMode(APRHandle handle, bool enable);
    /// 渲染3D正交视图（显示三个正交平面）
    VIZ_API NativeResult APR_RenderOrthogonal3D(APRHandle axial, APRHandle coronal, APRHandle sagittal);

    // ==================== APR 裁切框 ====================
    /// 设置裁切框（按体数据的初始大小占50%居中）- 全局版本（向后兼容）
    VIZ_API void APR_SetCropBox(int volumeWidth, int volumeHeight, int volumeDepth);
    /// 设置裁切框范围（体素坐标）- 全局版本（向后兼容）
    VIZ_API void APR_SetCropBoxRange(float xStart, float xEnd, float yStart, float yEnd, float zStart, float zEnd);
    /// 获取裁切框范围（虚拟坐标）- 全局版本（向后兼容）
    VIZ_API void APR_GetCropBox(float* xStart, float* xEnd, float* yStart, float* yEnd, float* zStart, float* zEnd);
    /// 启用/禁用裁切框 - 全局版本（向后兼容）
    VIZ_API void APR_EnableCropBox(bool enable);
    /// 获取裁切框启用状态 - 全局版本（向后兼容）
    VIZ_API bool APR_IsCropBoxEnabled();
    
    // ==================== APR 裁切框（Session版本）====================
    /// 设置裁切框范围（session级别）
    VIZ_API void APR_SetCropBoxRangeForSession(const char* sessionId, float xStart, float xEnd, float yStart, float yEnd, float zStart, float zEnd);
    /// 获取裁切框范围（session级别）
    VIZ_API void APR_GetCropBoxForSession(const char* sessionId, float* xStart, float* xEnd, float* yStart, float* yEnd, float* zStart, float* zEnd);
    /// 启用/禁用裁切框（session级别）
    VIZ_API void APR_EnableCropBoxForSession(const char* sessionId, bool enable);
    /// 获取裁切框启用状态（session级别）
    VIZ_API bool APR_IsCropBoxEnabledForSession(const char* sessionId);
    /// 设置裁切形状（session级别）
    VIZ_API void APR_SetCropShapeForSession(const char* sessionId, int shape);
    /// 获取裁切形状（session级别）
    VIZ_API int APR_GetCropShapeForSession(const char* sessionId);
    
    /// 设置裁切形状 (0=立方体, 1=球体, 2=圆柱体)
    VIZ_API void APR_SetCropShape(int shape);
    /// 获取裁切形状 (0=立方体, 1=球体, 2=圆柱体)
    VIZ_API int APR_GetCropShape();
    /// 设置圆柱体方向 (0=轴向Z, 1=冠状Y, 2=矢状X)
    VIZ_API void APR_SetCropCylinderDirection(int direction);
    /// 获取圆柱体方向 (0=轴向Z, 1=冠状Y, 2=矢状X)
    VIZ_API int APR_GetCropCylinderDirection();
    /// 按尺寸设置裁切框（以当前中心为基准，设置指定尺寸的裁切框，单位为像素）
    VIZ_API void APR_SetCropBoxSize(int sizeX, int sizeY, int sizeZ, int volumeWidth, int volumeHeight, int volumeDepth);
    /// 裁切体数据，返回新的APR句柄（用于显示裁切后的体数据）
    VIZ_API APRHandle APR_CropVolume(APRHandle sourceHandle);
    /// 获取最后一次裁切的结果（不创建新的，如果没有裁切的结果返回nullptr）
    VIZ_API APRHandle APR_GetLastCroppedVolume();
    /// 获取裁切后体数据的尺寸（如果没有裁切结果，返回0,0,0）
    VIZ_API void APR_GetCroppedVolumeDimensions(int* width, int* height, int* depth);
    /// 获取裁切后体数据的spacing
    VIZ_API void APR_GetCroppedVolumeSpacing(float* spacingX, float* spacingY, float* spacingZ);
    /// 应用裁切结果到所有关联的APR（替换volume，重置中心点和旋转）
    /// 返回1表示成功，0表示失败（无裁切结果）
    VIZ_API int APR_ApplyCroppedVolume();
    /// 应用裁切结果到指定session的所有APR（session级别管理）
    /// sessionId: 要应用裁切的session标识
    /// 返回1表示成功，0表示失败
    VIZ_API int APR_ApplyCroppedVolumeForSession(const char* sessionId);
    /// 执行裁切并创建新的 volume 实例
    /// 裁切后相当于打开了一组新的图像，原始 volume 可以释放
    /// 如果是球或圆柱形状，外部区域填充0形成立方体
    /// 返回1表示成功，0表示失败
    VIZ_API int APR_ApplyCroppedVolumeTo3D(APRHandle sourceHandle);
    /// 获取当前是否有活跃的裁切后 volume
    VIZ_API bool APR_HasActiveVolume();
    /// 获取当前活跃 volume 的尺寸
    VIZ_API void APR_GetActiveVolumeSize(int* width, int* height, int* depth);
    /// 清除当前活跃的 volume（准备加载新的 DICOM）
    VIZ_API void APR_ClearActiveVolume();
    /// 删除APR句柄并释放资源
    VIZ_API void APR_Destroy(APRHandle handle);

    // ==================== MPR (多平面重建) ====================
    /// MPR ��Ƭ����
    typedef enum {
        MPR_AXIAL = 0,      // ����/����� (XY plane, Z direction)
        MPR_CORONAL = 1,    // ��״�� (XZ plane, Y direction)
        MPR_SAGITTAL = 2    // ʸ״�� (YZ plane, X direction)
    } MPRSliceDirection;

    /// ���� MPR ��Ⱦ��
    VIZ_API MPRHandle MPR_Create();
    /// ���� MPR ��Ⱦ��
    VIZ_API void MPR_Destroy(MPRHandle handle);
    /// ����������
    VIZ_API NativeResult MPR_SetVolume(MPRHandle handle, VolumeHandle volume);
    /// ������Ƭ����
    VIZ_API void MPR_SetSliceDirection(MPRHandle handle, MPRSliceDirection direction);
    /// ��ȡ��Ƭ����
    VIZ_API MPRSliceDirection MPR_GetSliceDirection(MPRHandle handle);
    /// �������ĵ�
    VIZ_API void MPR_SetCenter(MPRHandle handle, float x, float y, float z);
    /// ��ȡ���ĵ�
    VIZ_API void MPR_GetCenter(MPRHandle handle, float* x, float* y, float* z);
    /// ���Ӷ�� MPR��ʹ���ǹ���ͬһ�����ĵ㣨���ڶ���ͼͬ����
    VIZ_API void MPR_LinkCenter(MPRHandle* handles, int count);
    /// ��ȡָ���������Ƭ
    VIZ_API void* MPR_GetSlice(MPRHandle handle, int direction, int* width, int* height);
    /// ��ʾ/���ض�λ��
    VIZ_API void MPR_SetShowCrossHair(MPRHandle handle, bool show);
    /// �����������ӣ�1.0 = ԭʼ��С��>1.0 �Ŵ�<1.0 ��С��
    VIZ_API void MPR_SetZoom(MPRHandle handle, float zoomFactor);
    /// ��ȡ��������
    VIZ_API float MPR_GetZoom(MPRHandle handle);

    /// 设置关联的Session ID（用于从Session获取mask数据）
    VIZ_API void MPR_SetSessionId(MPRHandle handle, const char* sessionId);

    // Window/level (HU)
    VIZ_API void MPR_SetWindowLevel(MPRHandle handle, float windowWidth, float windowLevel);
    VIZ_API void MPR_GetWindowLevel(MPRHandle handle, float* windowWidth, float* windowLevel);
    /// ��Ⱦ��ˢ�£�
    VIZ_API NativeResult MPR_Render(MPRHandle handle);

    // ==================== MPR Mask ��ʾ ====================
    /// ���� Mask �� MPR��֧�ֶ�� Mask ������ʾ��
    VIZ_API NativeResult MPR_AddMask(MPRHandle handle, MaskManagerHandle maskManager, int maskIndex);
    /// �Ƴ�ָ���� Mask
    VIZ_API void MPR_RemoveMask(MPRHandle handle, int maskIndex);
    /// ������� Mask
    VIZ_API void MPR_ClearMasks(MPRHandle handle);
    /// ���� Mask ����ʾ͸���ȣ�0.0-1.0��
    VIZ_API void MPR_SetMaskOpacity(MPRHandle handle, int maskIndex, float opacity);
    /// ���� Mask ����ɫ��RGBA��ÿ������ 0.0-1.0��
    VIZ_API void MPR_SetMaskColor(MPRHandle handle, int maskIndex, float r, float g, float b, float a);
    /// ��ʾ/����ָ�� Mask
    VIZ_API void MPR_SetMaskVisible(MPRHandle handle, int maskIndex, bool visible);
    /// �����Ƿ���ʾ���� Masks
    VIZ_API void MPR_SetShowAllMasks(MPRHandle handle, bool show);

    // ==================== MPR Mask �༭�͹���������Session��====================
    
    /// Mask��Ϣ�ṹ�壨���ڷ��ؼ��ص�mask���ݣ�
    typedef struct {
        int maskId;
        char name[256];
        char color[16];       // #rrggbb��ʽ
        bool visible;
        float minThreshold;
        float maxThreshold;
    } MaskInfo;

    /// ��ȡ�����ݵ�ֱ��ͼ��������ֵ�ָ���棩
    /// @param sessionId Session��ʶ��
    /// @param outData ���256��bin��Ƶ�����飨�����������int[256]��
    /// @param outMinValue ���CTֵ��Сֵ
    /// @param outMaxValue ���CTֵ���ֵ
    /// @return �ɹ�����NATIVE_OK
    VIZ_API NativeResult MPR_GetVolumeHistogram(
        const char* sessionId,
        int* outData,
        int* outMinValue,
        int* outMaxValue
    );

    /// ע��Session��Volume���ڴ���APR��ͼ����ã�
    /// @param sessionId Session��ʶ��
    /// @param volume �����ݾ��
    /// @return �ɹ�����NATIVE_OK
    VIZ_API NativeResult MPR_RegisterSessionVolume(
        const char* sessionId,
        VolumeHandle volume
    );

    /// ����Ԥ��mask��ʵʱ��ʾ��ֵ�ָ�Ч����
    /// @param sessionId Session��ʶ��
    /// @param minThreshold ��С��ֵ
    /// @param maxThreshold �����ֵ
    /// @param hexColor ��ɫ��#rrggbb��ʽ��
    /// @return �ɹ�����NATIVE_OK
    VIZ_API NativeResult MPR_UpdatePreviewMask(
        const char* sessionId,
        float minThreshold,
        float maxThreshold,
        const char* hexColor
    );

    /// ���Ԥ��mask
    /// @param sessionId Session��ʶ��
    /// @return �ɹ�����NATIVE_OK
    VIZ_API NativeResult MPR_ClearPreviewMask(
        const char* sessionId
    );

    /// ������ֵ����permanent mask
    /// @param sessionId Session��ʶ��
    /// @param minThreshold ��С��ֵ
    /// @param maxThreshold �����ֵ
    /// @param hexColor ��ɫ��#rrggbb��ʽ��
    /// @param maskName Mask����
    /// @param outMaskId ��������maskId
    /// @return �ɹ�����NATIVE_OK
    VIZ_API NativeResult MPR_CreateMaskFromThreshold(
        const char* sessionId,
        float minThreshold,
        float maxThreshold,
        const char* hexColor,
        const char* maskName,
        int* outMaskId
    );

    /// ����һ������(ȫ0)��permanent mask������ROI���Ƶȣ�
    /// @param sessionId Session��ʶ��
    /// @param hexColor ��ɫ��#rrggbb��ʽ��
    /// @param maskName Mask����
    /// @param outMaskId ��������maskId
    /// @return �ɹ�����NATIVE_OK
    VIZ_API NativeResult MPR_CreateEmptyMask(
        const char* sessionId,
        const char* hexColor,
        const char* maskName,
        int* outMaskId
    );

    /// 统计指定 mask 内的 HU 分布与基本统计
    /// @param sessionId Session标识
    /// @param maskId 要统计的 maskId
    /// @param outHistogram 256-bin 直方图（可传 nullptr）
    /// @param outMinValue HU最小值
    /// @param outMaxValue HU最大值
    /// @param outMean 平均HU
    /// @param outStdDev HU标准差
    /// @param outCount mask内体素数量
    /// @param outVolumeMm3 体积（mm^3），按 spacing 计算
    VIZ_API NativeResult MPR_GetMaskStatistics(
        const char* sessionId,
        int maskId,
        int* outHistogram,
        int* outMinValue,
        int* outMaxValue,
        double* outMean,
        double* outStdDev,
        unsigned long long* outCount,
        double* outVolumeMm3
    );

    // Export a permanent mask as STL mesh (binary). `step` controls precision (1=full res, 2/4=downsample).
    VIZ_API NativeResult MPR_ExportMaskToSTL(
        const char* sessionId,
        int maskId,
        const char* filepath,
        int step
    );

    VIZ_API NativeResult MPR_GetMaskData(
        const char* sessionId,
        int maskId,
        unsigned char* buffer,
        size_t bufferSize
    );

    VIZ_API NativeResult MPR_UpdateMaskData(
        const char* sessionId,
        int maskId,
        const unsigned char* buffer,
        size_t bufferSize
    );

    /// ɾ��mask
    /// @param sessionId Session��ʶ��
    /// @param maskId Ҫɾ����maskId
    /// @return �ɹ�����NATIVE_OK
    VIZ_API NativeResult MPR_DeleteMask(
        const char* sessionId,
        int maskId
    );

    /// ��������masks���ļ�
    /// @param sessionId Session��ʶ��
    /// @param folderPath ͼ���ļ���·��
    /// @param maskName ������ļ�����������չ����
    /// @param outFilePath �������������ļ�·���������������char[1024]��
    /// @param outFilePathSize outFilePath�Ļ�������С
    /// @return �ɹ�����NATIVE_OK
    VIZ_API NativeResult MPR_SaveMasks(
        const char* sessionId,
        const char* folderPath,
        const char* maskName,
        char* outFilePath,
        int outFilePathSize
    );

    /// ����masks���ļ�����Windows�ļ�ѡ��Ի���
    /// @param sessionId Session��ʶ��
    /// @param folderPath ��ʼ�ļ���·�������飺{imageFolderPath}/masks/��
    /// @param outMaskCount ������ص�mask����
    /// @param outMaskInfos ���mask��Ϣ���飨��������Ҫdelete[]�ͷţ�
    /// @return �ɹ�����NATIVE_OK���û�ȡ������NATIVE_USER_CANCELLED
    VIZ_API NativeResult MPR_LoadMasks(
        const char* sessionId,
        const char* folderPath,
        int* outMaskCount,
        MaskInfo** outMaskInfos
    );

    // ==================== Mask Morphology Operations ====================

    /// 形态学类型枚举
    typedef enum {
        MORPH_DILATE = 0,   // 膨胀
        MORPH_ERODE = 1,    // 腐蚀
        MORPH_OPEN = 2,     // 开运算（先腐蚀后膨胀）
        MORPH_CLOSE = 3     // 闭运算（先膨胀后腐蚀）
    } MorphologyOperation;

    /// 对指定mask执行形态学操作（2D逐层处理）
    /// @param sessionId Session标识
    /// @param maskId 要处理的maskId
    /// @param operation 形态学操作类型
    /// @param kernelSize 核大小（3, 5, 7等奇数）
    /// @param iterations 迭代次数
    /// @return 成功返回NATIVE_OK
    VIZ_API NativeResult MPR_MaskMorphology2D(
        const char* sessionId,
        int maskId,
        MorphologyOperation operation,
        int kernelSize,
        int iterations
    );

    /// 对指定mask执行3D形态学操作
    /// @param sessionId Session标识
    /// @param maskId 要处理的maskId
    /// @param operation 形态学操作类型
    /// @param kernelSize 核大小（3, 5, 7等奇数）
    /// @param iterations 迭代次数
    /// @return 成功返回NATIVE_OK
    VIZ_API NativeResult MPR_MaskMorphology3D(
        const char* sessionId,
        int maskId,
        MorphologyOperation operation,
        int kernelSize,
        int iterations
    );

    // ==================== Mask Boolean Operations ====================

    /// 布尔运算类型枚举
    typedef enum {
        BOOL_UNION = 0,       // 并集 (A | B)
        BOOL_INTERSECT = 1,   // 交集 (A & B)
        BOOL_SUBTRACT = 2     // 差集 (A - B)
    } BooleanOperation;

    /// 对两个mask执行布尔运算，结果存入新mask
    /// @param sessionId Session标识
    /// @param maskIdA 第一个maskId
    /// @param maskIdB 第二个maskId
    /// @param operation 布尔运算类型
    /// @param hexColor 结果mask颜色
    /// @param resultName 结果mask名称
    /// @param outMaskId 输出新创建的maskId
    /// @return 成功返回NATIVE_OK
    VIZ_API NativeResult MPR_MaskBoolean(
        const char* sessionId,
        int maskIdA,
        int maskIdB,
        BooleanOperation operation,
        const char* hexColor,
        const char* resultName,
        int* outMaskId
    );

    /// 反转指定mask（0变255，255变0）
    /// @param sessionId Session标识
    /// @param maskId 要反转的maskId
    /// @return 成功返回NATIVE_OK
    VIZ_API NativeResult MPR_MaskInverse(
        const char* sessionId,
        int maskId
    );

    // ==================== Bone Metrics (Mask-based) ====================

    /// ���ڻ�ȡSession�Ŀռ�Spacing��mm��
    VIZ_API NativeResult MPR_GetVolumeSpacing(
        const char* sessionId,
        float* outSpacingX,
        float* outSpacingY,
        float* outSpacingZ
    );

    /// 骨分析指标输出（单位见字段注释）
    typedef struct {
        int maskId;
        // Optional ROI mask id used to compute TV (0 if not provided)
        int roiMaskId;
        int voxelCount;

        // ROI volume (TV) and marrow volume (MV) when ROI is provided
        int roiVoxelCount;
        double tvRoiMm3;
        double mvRoiMm3;
        double bv_tv_roi; // BV/TV based on ROI TV

        // Physical metrics
        double volumeMm3;
        double volumeCm3;
        double surfaceAreaMm2;
        double surfaceAreaCm2;
        double bs_bv_1_per_mm; // BS/BV

        // Bounding box (voxel index space)
        int bboxMinX;
        int bboxMinY;
        int bboxMinZ;
        int bboxMaxX;
        int bboxMaxY;
        int bboxMaxZ;

        // Centroid (mm)
        double centroidXmm;
        double centroidYmm;
        double centroidZmm;

        // Derived trabecular metrics (plate-model approximation)
        double tvBoxMm3;        // TV based on bone mask bounding box (legacy/debug)
        double bv_tv;           // BV/TV (based on bounding box TV, legacy/debug)
        double tbThMm;          // Tb.Th
        double tbSpMm;          // Tb.Sp
        double tbNm_1_per_mm;   // Tb.N

        // SMI approximation (1-voxel dilation)
        double smi;

        // Anisotropy (MIL-based)
        double da;
        double daEigen1;
        double daEigen2;
        double daEigen3;
    } BoneMetrics;

    /// 计算骨分析指标
    VIZ_API NativeResult MPR_CalculateBoneMetrics(
        const char* sessionId,
        int maskId,
        BoneMetrics* outMetrics
    );

    /// 计算骨分析指标（ROI-aware 版本，TV 由 roiMaskId 定义）
    /// @param roiMaskId ROI mask id; <=0 means use full volume / legacy behavior
    VIZ_API NativeResult MPR_CalculateBoneMetricsEx(
        const char* sessionId,
        int maskId,
        int roiMaskId,
        BoneMetrics* outMetrics
    );

    // ==================== 3D ����� ====================
    /// ���� 3D �������Ⱦ��
    VIZ_API Volume3DHandle Volume3D_Create();
    /// ���� 3D �������Ⱦ��
    VIZ_API void Volume3D_Destroy(Volume3DHandle handle);
    /// ����������
    VIZ_API NativeResult Volume3D_AddVolume(Volume3DHandle handle, VolumeHandle volume);
    /// �Ƴ�������
    VIZ_API NativeResult Volume3D_RemoveVolume(Volume3DHandle handle, int index);
    /// ��ȡ����������
    VIZ_API int Volume3D_GetVolumeCount(Volume3DHandle handle);
    /// ���ô��ݺ�����Ϊָ�������������ã�
    VIZ_API NativeResult Volume3D_SetTransferFunction(Volume3DHandle handle, int volumeIndex, TransferFunctionHandle tf);
    /// ���ù��ղ���
    VIZ_API void Volume3D_SetLightParameters(Volume3DHandle handle, float ambient, float diffuse, float specular);
    /// ��ȡ���ղ���
    VIZ_API void Volume3D_GetLightParameters(Volume3DHandle handle, float* ambient, float* diffuse, float* specular);
    /// ��Ⱦ��ˢ�£�
    VIZ_API NativeResult Volume3D_Render(Volume3DHandle handle);

    // ==================== 3D Primitives / Scene (Window-based) ====================
    // NOTE: These APIs operate on the 3D WindowHandle created/managed by the host.
    // The coordinate system is the same normalized space used by the 3D volume box
    // (centered at origin, preserving volume aspect ratio).

    typedef enum {
        PRIM3D_CUBE = 1,
        PRIM3D_SPHERE = 2,
        PRIM3D_CYLINDER = 3
    } Primitive3DType;

    /// Add a cube primitive. Returns primitiveId (>0) on success, <=0 on failure.
    VIZ_API int Window3D_AddCube(WindowHandle handle, float sizeX, float sizeY, float sizeZ);
    /// Add a sphere primitive. Returns primitiveId (>0) on success, <=0 on failure.
    VIZ_API int Window3D_AddSphere(WindowHandle handle, float radius);
    /// Add a cylinder primitive (Y axis). Returns primitiveId (>0) on success, <=0 on failure.
    VIZ_API int Window3D_AddCylinder(WindowHandle handle, float radius, float height);

    /// Remove a primitive by id.
    VIZ_API NativeResult Window3D_RemovePrimitive(WindowHandle handle, int primitiveId);
    /// Remove all primitives.
    VIZ_API void Window3D_ClearPrimitives(WindowHandle handle);

    /// Set per-primitive transform.
    VIZ_API NativeResult Window3D_SetPrimitiveTransform(
        WindowHandle handle,
        int primitiveId,
        float tx, float ty, float tz,
        float rxDeg, float ryDeg, float rzDeg,
        float sx, float sy, float sz
    );

    /// Set per-primitive color (RGBA 0..1).
    VIZ_API NativeResult Window3D_SetPrimitiveColor(WindowHandle handle, int primitiveId, float r, float g, float b, float a);
    /// Set per-primitive visibility.
    VIZ_API NativeResult Window3D_SetPrimitiveVisible(WindowHandle handle, int primitiveId, bool visible);

    /// Set a transform applied to the whole primitive collection (scene transform).
    VIZ_API NativeResult Window3D_SetSceneTransform(
        WindowHandle handle,
        float tx, float ty, float tz,
        float rxDeg, float ryDeg, float rzDeg,
        float sx, float sy, float sz
    );

    // ==================== ���ݺ��� ====================
    /// �������ݺ���
    VIZ_API TransferFunctionHandle TransferFunction_Create();
    /// ���ٴ��ݺ���
    VIZ_API void TransferFunction_Destroy(TransferFunctionHandle handle);
    /// ���ӿ��Ƶ㣨value: ����ֵ, r/g/b/a: ��ɫ��͸���� 0.0-1.0��
    VIZ_API NativeResult TransferFunction_AddControlPoint(TransferFunctionHandle handle, float value, float r, float g, float b, float a);
    /// �Ƴ����Ƶ�
    VIZ_API NativeResult TransferFunction_RemoveControlPoint(TransferFunctionHandle handle, int index);
    /// ������п��Ƶ�
    VIZ_API void TransferFunction_Clear(TransferFunctionHandle handle);
    /// ��ȡ���Ƶ�����
    VIZ_API int TransferFunction_GetControlPointCount(TransferFunctionHandle handle);

    // ==================== �����������Ͷ��� ====================
    typedef void* ToolManagerHandle;
    typedef void* ToolHandle;

    // ==================== ���ڹ��� ====================
    /// ������Ⱦ���ڣ�GLFW ���� - ���ڶ������ԣ�
    VIZ_API WindowHandle Window_Create(int width, int height, const char* title);
    /// ������Ⱦ����
    VIZ_API void Window_Destroy(WindowHandle handle);
    /// �������д��ڲ�ֹͣ��Ϣ�����������л�tabʱ���������ݣ�
    VIZ_API void Window_HideAllWindows();
    /// ��ʾ���д��ڲ�������Ϣ�����������л���viewer tab��
    VIZ_API void Window_ShowAllWindows();
    /// ��������3D���ڣ������л�tabʱ�ͷ���Դ��
    VIZ_API void Window_DestroyAll3DWindows();
    /// �������д��ڲ��ͷ�������Դ�����ڴ���ͼ��ʱ������
    VIZ_API void Window_DestroyAllWindows();
    /// ����Ⱦ�������ڣ�֧�� APR/MPR/Volume3D��
    VIZ_API NativeResult Window_BindRenderer(WindowHandle handle, void* rendererHandle, int rendererType);
    /// ����3D���ڵ�APR�������������3D��Ⱦ��
    VIZ_API NativeResult Window_Set3DViewAPRs(WindowHandle handle, void* aprAxial, void* aprCoronal, void* aprSagittal);
    // Per-window crop box visibility (does not change the global crop box state; only affects drawing/interaction for this window)
    VIZ_API NativeResult Window_SetCropBoxVisible(WindowHandle handle, bool visible);

    // 3D renderer kind selector:
    // 1 = ImageBrowser orthogonal (tri-planar)
    // 2 = ROI orthogonal (tri-planar)
    // 3 = 3D reconstruction (raycast)
    VIZ_API NativeResult Window_Set3DRendererKind(WindowHandle handle, int kind);
    /// ���ÿ�3D���ڲ�����ʾģʽ��true=��ʾ������3D(��������)��false=��ʾ�ռ�Raycast 3D
    VIZ_API NativeResult Window_Set3DViewOrthogonalMode(WindowHandle handle, bool enableOrthogonal);
    /// 3D Raycast �Ե����ݶ� (GL_TEXTURE_3D) ���ϴ�ʱ��ʹ�����ڴ��Ż���true=�����ֱ���/�������ݼ�����VRAM
    VIZ_API NativeResult Window_Set3DViewVramOptimized(WindowHandle handle, bool enableOptimized);
    /// 3D Raycast mask iso-surface: when enabled, raycast uses current session mask(s) as an isosurface (binary mask -> iso=0.5).
    VIZ_API NativeResult Window_Set3DViewMaskIsoSurfaceEnabled(WindowHandle handle, bool enable);

    /// Set 3D raycast transfer function control points.
    /// pointsPacked: array of floats with layout [value,r,g,b,a] repeated pointCount times.
    /// All values are expected in normalized range 0..1.
    /// If pointsPacked is null or pointCount<=0, resets to default grayscale ramp.
    VIZ_API NativeResult Window_Set3DViewTransferFunctionPoints(WindowHandle handle, const float* pointsPacked, int pointCount);

    /// Set 3D raycast lighting parameters (used by iso-surface shading; normalized 0..1).
    VIZ_API NativeResult Window_Set3DViewLightParameters(WindowHandle handle, float ambient, float diffuse, float specular);

    /// Get 3D raycast lighting parameters.
    VIZ_API NativeResult Window_Get3DViewLightParameters(WindowHandle handle, float* ambient, float* diffuse, float* specular);
    /// ���ô��ڵĹ��߹����������ڲ������ߣ�
    VIZ_API NativeResult Window_SetToolManager(WindowHandle handle, ToolManagerHandle toolManager);
    /// ���ô��ڵļ���ߣ����ڲ������ߣ�
    VIZ_API NativeResult Window_SetActiveTool(WindowHandle handle, ToolHandle tool);
    /// ���õ�ǰ�������ͣ�1=Line,2=Angle,3=Rect,4=Circle,5=Bezier��
    VIZ_API void Window_SetToolType(WindowHandle handle, int toolType);
    /// ˢ�´���
    VIZ_API void Window_Refresh(WindowHandle handle);
    /// �л������ڵ� OpenGL ������
    VIZ_API void Window_MakeCurrent(WindowHandle handle);
        ///  OpenGL  (GL_VENDOR/GL_RENDERER/GL_VERSION)
        ///  out* 
        VIZ_API NativeResult Window_GetGLInfo(
            WindowHandle handle,
            char* outVendor,
            int vendorSize,
            char* outRenderer,
            int rendererSize,
            char* outVersion,
            int versionSize
        );
    /// ��ȡ���ھ�������ڰ� Electron div��
    VIZ_API void* Window_GetNativeHandle(WindowHandle handle);
    /// �����¼�ѭ������������
    VIZ_API bool Window_PollEvents(WindowHandle handle);
    /// ��GLFW��������Ϊָ�������ڵ��Ӵ��ڣ�����Ƕ�뵽Electron�ȣ�
    VIZ_API NativeResult Window_SetParentWindow(WindowHandle handle, void* parentHwnd, int x, int y, int width, int height);
    /// ˢ�´��ڲ㼶���ڸ�����resize����ã���ֹ��Electron��GPU�㸲�ǣ�
    VIZ_API NativeResult Window_RefreshZOrder(WindowHandle handle);
    /// �������ڴ�С��λ��
    VIZ_API NativeResult Window_Resize(WindowHandle handle, int x, int y, int width, int height);
    /// �����̶�֡����Ⱦѭ�������������Ľ������飩
    VIZ_API NativeResult Window_StartRenderLoop(int targetFPS);
    /// ֹͣ��Ⱦѭ��
    VIZ_API void Window_StopRenderLoop();
    /// ���������߳�����GLFW�¼�ѭ���������Electron/Chromium���¼�ѭ����ͻ��
    VIZ_API NativeResult Window_StartEventLoop();
    /// ֹͣGLFW�¼�ѭ���߳�
    VIZ_API void Window_StopEventLoop();

    /// Reset 3D view state (zoom/pan/rotation) back to defaults.
    VIZ_API void Window_ResetView(WindowHandle handle);

    // ==================== ������Ⱦ������ Web ���ɣ� ====================
    /// �������� OpenGL �����ģ�FBO��
    VIZ_API WindowHandle OffscreenContext_Create(int width, int height);
    /// ��������������
    VIZ_API void OffscreenContext_Destroy(WindowHandle handle);
    /// ��Ⱦ�� FBO ����ȡ�������ݣ�RGBA��ʽ����targetWidth/targetHeight ָ������ߴ�
    VIZ_API FrameBuffer* OffscreenContext_RenderToBuffer(WindowHandle handle, void* rendererHandle, int rendererType, int targetWidth, int targetHeight);
    /// �ͷ� FrameBuffer
    VIZ_API void FrameBuffer_Destroy(FrameBuffer* buffer);

    // ==================== �������� ====================
    // ==================== Completed Measurements ====================

    typedef struct {
        int id;
        int toolType;
        float result;
        bool isAPR;
        int sliceDirection;
        int sliceIndex;
        float centerX;
        float centerY;
        float centerZ;
        float rotX;
        float rotY;
        float rotZ;
        // Session id (UTF-8). Empty if unknown.
        char sessionId[64];
    } CompletedMeasurementInfo;

    /// ��ȡ���в�������Ŀ
    VIZ_API int Measurement_GetCompletedCount();

    /// ��ȡ���в�������（outItems ���鳤�� maxItems）
    /// @return ʵ��д��Ŀ
    VIZ_API int Measurement_GetCompletedList(CompletedMeasurementInfo* outItems, int maxItems);
    // Returns number of points written (0 if not available / not supported).
    // Axis is cumulative distance in mm; values are HU.
    VIZ_API int Measurement_GetProfileData(const char* sessionId, int measurementId, double* outAxis, double* outValues, int maxPoints);

    /// ɾ��ָ������ID�Ĳ�����
    VIZ_API bool Measurement_Delete(int measurementId);

    // ==================== Session-aware Measurement APIs ====================
    /// 获取 session 内已完成的测量数量
    VIZ_API int Measurement_GetCompletedCountForSession(const char* sessionId);
    /// 获取 session 内已完成的测量列表
    VIZ_API int Measurement_GetCompletedListForSession(const char* sessionId, CompletedMeasurementInfo* outItems, int maxItems);
    /// 删除 session 内的指定测量
    VIZ_API bool Measurement_DeleteForSession(const char* sessionId, int measurementId);
    /// 清除 session 内所有测量
    VIZ_API void Measurement_ClearAllForSession(const char* sessionId);
    // ==================== End Session-aware Measurement APIs ====================

    /// ��ȡ ROI/���� (Rect/Circle) ������� HU ֱ��ͼ (256 bins)
    /// outBins must point to an array of 256 ints.
    VIZ_API NativeResult Measurement_GetRegionHistogram(
        const char* sessionId,
        int measurementId,
        int* outBins,
        int* outMinValue,
        int* outMaxValue
    );

    /// �������߹�����
    VIZ_API ToolManagerHandle ToolManager_Create();
    /// ���ٹ��߹�����
    VIZ_API void ToolManager_Destroy(ToolManagerHandle handle);

    /// ����ֱ�߲�������
    VIZ_API ToolHandle Tool_CreateLine(ToolManagerHandle manager);
    /// �����ǶȲ�������
    VIZ_API ToolHandle Tool_CreateAngle(ToolManagerHandle manager);
    /// ��������ROI����
    VIZ_API ToolHandle Tool_CreateRectangle(ToolManagerHandle manager);
    /// ����Բ��ROI����
    VIZ_API ToolHandle Tool_CreateCircle(ToolManagerHandle manager);
    /// �������������߹���
    VIZ_API ToolHandle Tool_CreateBezier(ToolManagerHandle manager);
    /// �������ɻ��ƹ���
    VIZ_API ToolHandle Tool_CreateFreehand(ToolManagerHandle manager);

    /// ���ӿ��Ƶ㵽����
    VIZ_API void Tool_AddPoint(ToolHandle tool, float x, float y);
    /// ��ɹ��߻���
    VIZ_API void Tool_Finish(ToolHandle tool);
    /// ��ȡ������������롢�Ƕȡ�����ȣ�
    VIZ_API float Tool_GetMeasurement(ToolHandle tool);
    /// ��ȡ��������
    VIZ_API const char* Tool_GetName(ToolHandle tool);
    /// ��ȡ���߿��Ƶ�����
    VIZ_API int Tool_GetPointCount(ToolHandle tool);
    /// ��ȡָ�����Ƶ�����
    VIZ_API void Tool_GetPoint(ToolHandle tool, int index, float* x, float* y);

    /// ���û����
    VIZ_API void ToolManager_SetActiveTool(ToolManagerHandle manager, ToolHandle tool);
    /// ��ȡ��������
    VIZ_API int ToolManager_GetToolCount(ToolManagerHandle manager);
    /// ��ȡָ������
    VIZ_API ToolHandle ToolManager_GetTool(ToolManagerHandle manager, int index);
    /// ɾ������
    VIZ_API void ToolManager_DeleteTool(ToolManagerHandle manager, ToolHandle tool);
    /// ������й���
    VIZ_API void ToolManager_Clear(ToolManagerHandle manager);

    // ==================== Mask�༭����API ====================
    
    /// ���õ�ǰ�༭��Mask��ָ��MaskManager��mask������
    VIZ_API void Mask_SetCurrentMask(void* maskManager, int maskIndex);

    /// ѡ��Session�ڲ���permanent mask������MaskEdit���ƣ�
    /// ע�⣺ֻ��sessionId�Ե�MPR_Window(����renderer)��Ч��
    VIZ_API NativeResult MPR_SelectMaskForEditing(
        const char* sessionId,
        int maskId
    );

    /// ���õ�ǰMaskEdit���ߣ�1=Brush,2=Eraser,3=RectROI,4=CircleROI,5=PolygonROI,6=FloodFill,7=ConnectedComponent
    VIZ_API void Mask_SetTool(int maskTool);

    /// ��ȡ��ǰMaskEdit���ߣ�
    VIZ_API int Mask_GetTool();
    
    /// ���û���/��Ƥ���뾶����λ�����أ�
    VIZ_API void Mask_SetBrushRadius(float radius);
    
    /// ��ȡ��ǰ���ʰ뾶
    VIZ_API float Mask_GetBrushRadius();

    // ==================== Session-aware Mask Editing APIs ====================
    /// 设置 session 内的 mask 工具类型
    VIZ_API void Mask_SetToolForSession(const char* sessionId, int tool);
    /// 获取 session 内的 mask 工具类型
    VIZ_API int Mask_GetToolForSession(const char* sessionId);
    /// 设置 session 内的画笔半径
    VIZ_API void Mask_SetBrushRadiusForSession(const char* sessionId, float radius);
    /// 获取 session 内的画笔半径
    VIZ_API float Mask_GetBrushRadiusForSession(const char* sessionId);
    /// 设置 session 内的当前 mask 索引
    VIZ_API void Mask_SetCurrentIndexForSession(const char* sessionId, int maskIndex);
    /// 获取 session 内的当前 mask 索引
    VIZ_API int Mask_GetCurrentIndexForSession(const char* sessionId);
    // ==================== End Session-aware Mask Editing APIs ====================

    // ==================== MPR Mask Overlay API ====================
    
    /// ����Mask���Ӳ㵽MPR��ͼ
    VIZ_API void MPR_AddMaskOverlay(MPRHandle handle, void* maskManager, int maskIndex,
                                    float r, float g, float b, float a);
    
    /// �Ƴ�ָ����Mask���Ӳ�
    VIZ_API void MPR_RemoveMaskOverlay(MPRHandle handle, int overlayIndex);
    
    /// ����Mask���Ӳ���ɫ
    VIZ_API void MPR_SetMaskOverlayColor(MPRHandle handle, int overlayIndex,
                                         float r, float g, float b, float a);
    
    /// ����Mask���Ӳ�ɼ���
    VIZ_API void MPR_SetMaskOverlayVisible(MPRHandle handle, int overlayIndex, bool visible);
    
    /// �������Mask���Ӳ�
    VIZ_API void MPR_ClearMaskOverlays(MPRHandle handle);
    
    /// ��ȡMask���Ӳ�����
    VIZ_API int MPR_GetMaskOverlayCount(MPRHandle handle);

    // ==================== Window Update API ====================
    
    /// ���������ػ棨����WM_PAINT��Ϣ��
    VIZ_API void Window_Invalidate(WindowHandle handle);
    
    /// �������д����ػ�
    VIZ_API void Window_InvalidateAll();

#ifdef __cplusplus
}
#endif
