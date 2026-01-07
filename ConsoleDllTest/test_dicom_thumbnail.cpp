// test_dicom_thumbnail.cpp - 测试 DllDicom 的像素数据获取并生成缩略图
#include <iostream>
#include <windows.h>
#include <string>
#include <algorithm>
#include <fstream>
#include "Common/NativeInterfaces.h"

// 函数指针定义
typedef DicomHandle (*CreateReaderFunc)();
typedef void (*DestroyReaderFunc)(DicomHandle);
typedef NativeResult (*ReadFileFunc)(DicomHandle, const char*);
typedef void* (*GetPixelDataFunc)(DicomHandle, int*, int*, int*);
typedef const char* (*GetLastErrorFunc)();

// 生成缩略图（简单的最近邻下采样）
void GenerateThumbnail(const short* srcData, int srcWidth, int srcHeight, 
                      short* dstData, int thumbWidth, int thumbHeight) {
    float xRatio = static_cast<float>(srcWidth) / thumbWidth;
    float yRatio = static_cast<float>(srcHeight) / thumbHeight;
    
    for (int y = 0; y < thumbHeight; ++y) {
        for (int x = 0; x < thumbWidth; ++x) {
            int srcX = static_cast<int>(x * xRatio);
            int srcY = static_cast<int>(y * yRatio);
            
            // 确保不越界
            srcX = std::min(srcX, srcWidth - 1);
            srcY = std::min(srcY, srcHeight - 1);
            
            dstData[y * thumbWidth + x] = srcData[srcY * srcWidth + srcX];
        }
    }
}

// 归一化到 0-255 范围
void NormalizeToGrayscale(const short* srcData, int width, int height, unsigned char* dstData) {
    // 找到最小值和最大值
    short minVal = srcData[0];
    short maxVal = srcData[0];
    
    int totalPixels = width * height;
    for (int i = 0; i < totalPixels; ++i) {
        if (srcData[i] < minVal) minVal = srcData[i];
        if (srcData[i] > maxVal) maxVal = srcData[i];
    }
    
    std::cout << "像素值范围: [" << minVal << ", " << maxVal << "]" << std::endl;
    
    // 归一化到 0-255
    float range = static_cast<float>(maxVal - minVal);
    if (range < 1.0f) range = 1.0f; // 避免除零
    
    for (int i = 0; i < totalPixels; ++i) {
        float normalized = (srcData[i] - minVal) / range;
        dstData[i] = static_cast<unsigned char>(normalized * 255);
    }
}

// 保存为 BMP 格式（简单的 8 位灰度图）
bool SaveAsBMP(const char* filename, const unsigned char* data, int width, int height) {
    // BMP 文件头
    #pragma pack(push, 1)
    struct BMPFileHeader {
        uint16_t fileType{0x4D42};  // "BM"
        uint32_t fileSize{0};
        uint16_t reserved1{0};
        uint16_t reserved2{0};
        uint32_t offsetData{0};
    };
    
    struct BMPInfoHeader {
        uint32_t size{40};
        int32_t width{0};
        int32_t height{0};
        uint16_t planes{1};
        uint16_t bitCount{8};
        uint32_t compression{0};
        uint32_t sizeImage{0};
        int32_t xPixelsPerMeter{0};
        int32_t yPixelsPerMeter{0};
        uint32_t colorsUsed{256};
        uint32_t colorsImportant{256};
    };
    #pragma pack(pop)
    
    BMPFileHeader fileHeader;
    BMPInfoHeader infoHeader;
    
    // 计算行的填充（每行必须是 4 字节对齐）
    int rowPadding = (4 - (width % 4)) % 4;
    int rowSize = width + rowPadding;
    
    // 设置文件头
    infoHeader.width = width;
    infoHeader.height = height;
    infoHeader.sizeImage = rowSize * height;
    
    // 调色板大小（256 色 * 4 字节）
    int paletteSize = 256 * 4;
    
    fileHeader.offsetData = sizeof(BMPFileHeader) + sizeof(BMPInfoHeader) + paletteSize;
    fileHeader.fileSize = fileHeader.offsetData + infoHeader.sizeImage;
    
    std::ofstream file(filename, std::ios::binary);
    if (!file) {
        std::cerr << "无法创建文件: " << filename << std::endl;
        return false;
    }
    
    // 写入文件头
    file.write(reinterpret_cast<const char*>(&fileHeader), sizeof(fileHeader));
    file.write(reinterpret_cast<const char*>(&infoHeader), sizeof(infoHeader));
    
    // 写入灰度调色板
    for (int i = 0; i < 256; ++i) {
        unsigned char color[4] = {
            static_cast<unsigned char>(i),
            static_cast<unsigned char>(i),
            static_cast<unsigned char>(i),
            0
        };
        file.write(reinterpret_cast<const char*>(color), 4);
    }
    
    // 写入像素数据（BMP 是从下到上存储的）
    unsigned char* paddingBytes = new unsigned char[rowPadding]();
    for (int y = height - 1; y >= 0; --y) {
        file.write(reinterpret_cast<const char*>(&data[y * width]), width);
        if (rowPadding > 0) {
            file.write(reinterpret_cast<const char*>(paddingBytes), rowPadding);
        }
    }
    delete[] paddingBytes;
    
    file.close();
    return true;
}

int main(int argc, char* argv[]) {
    // 设置控制台为 UTF-8
    SetConsoleOutputCP(CP_UTF8);
    
    std::cout << "========================================" << std::endl;
    std::cout << "  DICOM 缩略图生成测试" << std::endl;
    std::cout << "========================================" << std::endl;
    
    // 检查参数
    if (argc < 2) {
        std::cout << "用法: test_dicom_thumbnail <DICOM文件路径> [缩略图宽度] [缩略图高度]" << std::endl;
        std::cout << "示例: test_dicom_thumbnail test.dcm 256 256" << std::endl;
        return 1;
    }
    
    const char* dicomPath = argv[1];
    int thumbWidth = (argc > 2) ? std::stoi(argv[2]) : 256;
    int thumbHeight = (argc > 3) ? std::stoi(argv[3]) : 256;
    
    std::cout << "\n【测试配置】" << std::endl;
    std::cout << "DICOM 文件: " << dicomPath << std::endl;
    std::cout << "缩略图尺寸: " << thumbWidth << " x " << thumbHeight << std::endl;
    
    // 加载 DLL
    std::cout << "\n【步骤 1】加载 DllDicom.dll" << std::endl;
    HMODULE hDll = LoadLibraryA("DllDicom.dll");
    if (!hDll) {
        std::cerr << "错误: 无法加载 DllDicom.dll (错误码: " << GetLastError() << ")" << std::endl;
        return 1;
    }
    std::cout << "✓ DLL 加载成功" << std::endl;
    
    // 获取函数指针
    auto createReader = (CreateReaderFunc)GetProcAddress(hDll, "Dicom_CreateReader");
    auto destroyReader = (DestroyReaderFunc)GetProcAddress(hDll, "Dicom_DestroyReader");
    auto readFile = (ReadFileFunc)GetProcAddress(hDll, "Dicom_ReadFile");
    auto getPixelData = (GetPixelDataFunc)GetProcAddress(hDll, "Dicom_GetPixelData");
    auto getLastError = (GetLastErrorFunc)GetProcAddress(hDll, "Dicom_GetLastError");
    
    if (!createReader || !destroyReader || !readFile || !getPixelData || !getLastError) {
        std::cerr << "错误: 无法获取所需的函数" << std::endl;
        FreeLibrary(hDll);
        return 1;
    }
    std::cout << "✓ 函数地址获取成功" << std::endl;
    
    // 创建 DICOM 读取器
    std::cout << "\n【步骤 2】创建 DICOM 读取器" << std::endl;
    DicomHandle handle = createReader();
    if (!handle) {
        std::cerr << "错误: 无法创建 DICOM 读取器" << std::endl;
        FreeLibrary(hDll);
        return 1;
    }
    std::cout << "✓ 读取器创建成功" << std::endl;
    
    // 读取 DICOM 文件
    std::cout << "\n【步骤 3】读取 DICOM 文件" << std::endl;
    NativeResult result = readFile(handle, dicomPath);
    if (result != NATIVE_S_OK) {
        std::cerr << "错误: 读取 DICOM 文件失败" << std::endl;
        std::cerr << "错误信息: " << getLastError() << std::endl;
        destroyReader(handle);
        FreeLibrary(hDll);
        return 1;
    }
    std::cout << "✓ DICOM 文件读取成功" << std::endl;
    
    // 获取像素数据
    std::cout << "\n【步骤 4】获取像素数据" << std::endl;
    int width = 0, height = 0, depth = 0;
    void* pixelData = getPixelData(handle, &width, &height, &depth);
    
    if (!pixelData) {
        std::cerr << "错误: 获取像素数据失败" << std::endl;
        std::cerr << "错误信息: " << getLastError() << std::endl;
        destroyReader(handle);
        FreeLibrary(hDll);
        return 1;
    }
    
    std::cout << "✓ 像素数据获取成功" << std::endl;
    std::cout << "  原始尺寸: " << width << " x " << height << " x " << depth << std::endl;
    std::cout << "  总像素数: " << (width * height * depth) << std::endl;
    
    // 生成缩略图（假设是 16 位数据）
    std::cout << "\n【步骤 5】生成缩略图" << std::endl;
    short* srcData = static_cast<short*>(pixelData);
    short* thumbData = new short[thumbWidth * thumbHeight];
    
    // 处理第一个切片
    GenerateThumbnail(srcData, width, height, thumbData, thumbWidth, thumbHeight);
    std::cout << "✓ 缩略图生成成功 (" << thumbWidth << " x " << thumbHeight << ")" << std::endl;
    
    // 转换为 8 位灰度图
    std::cout << "\n【步骤 6】转换为 8 位灰度图" << std::endl;
    unsigned char* grayData = new unsigned char[thumbWidth * thumbHeight];
    NormalizeToGrayscale(thumbData, thumbWidth, thumbHeight, grayData);
    std::cout << "✓ 灰度转换完成" << std::endl;
    
    // 保存为 BMP
    std::cout << "\n【步骤 7】保存缩略图" << std::endl;
    const char* outputPath = "thumbnail.bmp";
    if (SaveAsBMP(outputPath, grayData, thumbWidth, thumbHeight)) {
        std::cout << "✓ 缩略图已保存: " << outputPath << std::endl;
    } else {
        std::cerr << "× 保存缩略图失败" << std::endl;
    }
    
    // 清理资源
    delete[] thumbData;
    delete[] grayData;
    delete[] static_cast<char*>(pixelData);
    destroyReader(handle);
    FreeLibrary(hDll);
    
    std::cout << "\n【步骤 8】尝试打开缩略图" << std::endl;
    // 使用系统默认程序打开图片
    ShellExecuteA(NULL, "open", outputPath, NULL, NULL, SW_SHOWNORMAL);
    std::cout << "✓ 已调用系统图片查看器" << std::endl;
    
    std::cout << "\n========================================" << std::endl;
    std::cout << "  测试完成！" << std::endl;
    std::cout << "========================================" << std::endl;
    
    return 0;
}
