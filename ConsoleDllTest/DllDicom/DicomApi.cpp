#include "pch.h"
#include "DicomApi.h"
#include "../Common/EncodingUtils.h"
#include "../Common/VolumeData.h"
#include <locale>
#include <codecvt>
#include <limits>

// ��ֹ Windows.h ���� min/max ��
#ifndef NOMINMAX
#define NOMINMAX
#endif

namespace fs = std::filesystem;

// ==================== �ڲ����ݽṹ ====================
struct DicomReaderContext {
    gdcm::ImageReader reader;
    std::string filepath;
    bool isLoaded = false;
};

// ʹ��Common/VolumeData.h�е�VolumeContext����
// struct VolumeContext ����VolumeData.h�ж���

struct DicomSeriesContext {
    std::string folderPath;
    std::vector<std::string> filePaths;  // ���� DICOM �ļ�·��
    gdcm::ImageReader firstReader;       // ��һ���ļ��� reader
    bool isScanned = false;
};

// ==================== ������ ====================
static thread_local std::string g_lastError;

static void SetLastError(const std::string& message) {
    g_lastError = message;
}

static bool TryParseFirstFloat(const std::string& raw, float* outValue) {
    if (!outValue) return false;
    std::string s = raw;
    // Trim leading/trailing whitespace and stop at first multi-value separator.
    const size_t sep = s.find('\\');
    if (sep != std::string::npos) s = s.substr(0, sep);

    auto ltrim = [](std::string& t) {
        size_t i = 0;
        while (i < t.size() && std::isspace(static_cast<unsigned char>(t[i]))) ++i;
        t.erase(0, i);
    };
    auto rtrim = [](std::string& t) {
        size_t i = t.size();
        while (i > 0 && std::isspace(static_cast<unsigned char>(t[i - 1]))) --i;
        t.erase(i);
    };
    ltrim(s);
    rtrim(s);
    if (s.empty()) return false;

    try {
        *outValue = std::stof(s);
        return true;
    } catch (...) {
        return false;
    }
}

extern "C" {

DICOM_API void Dicom_FreeBuffer(void* ptr) {
    if (ptr) free(ptr);
}

DICOM_API NativeResult Dicom_GenerateThumbnailBMP(DicomHandle handle, unsigned char** outData, int* outSize) {
    if (!handle || !outData || !outSize) {
        SetLastError("Invalid argument");
        return NATIVE_E_INVALID_ARGUMENT;
    }
    *outData = nullptr;
    *outSize = 0;
    auto* ctx = static_cast<DicomReaderContext*>(handle);
    if (!ctx->isLoaded) {
        SetLastError("No DICOM file loaded");
        return NATIVE_E_INTERNAL_ERROR;
    }
    try {
        const gdcm::Image& image = ctx->reader.GetImage();
        const unsigned int* dims = image.GetDimensions();
        int width = dims[0], height = dims[1];
        int depth = (dims[2] > 0) ? dims[2] : 1;
        unsigned long bufferSize = image.GetBufferLength();
        if (bufferSize == 0) {
            SetLastError("No pixel data available");
            return NATIVE_E_INTERNAL_ERROR;
        }
        std::vector<char> pixelBuffer(bufferSize);
        if (!image.GetBuffer(pixelBuffer.data())) {
            SetLastError("Failed to get pixel buffer");
            return NATIVE_E_INTERNAL_ERROR;
        }
        const short* srcData = reinterpret_cast<const short*>(pixelBuffer.data());
        // ��������ͼ
        const int thumbWidth = 256, thumbHeight = 256;
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
        // BMP �ڴ���
        #pragma pack(push, 1)
        struct BMPFileHeader {
            uint16_t bfType = 0x4D42;
            uint32_t bfSize = 0;
            uint16_t bfReserved1 = 0;
            uint16_t bfReserved2 = 0;
            uint32_t bfOffBits = 0;
        };
        struct BMPInfoHeader {
            uint32_t biSize = 40;
            int32_t  biWidth = thumbWidth;
            int32_t  biHeight = thumbHeight;
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
        infoHeader.biSizeImage = rowSize * thumbHeight;
        int paletteSize = 256 * 4;
        fileHeader.bfOffBits = sizeof(BMPFileHeader) + sizeof(BMPInfoHeader) + paletteSize;
        fileHeader.bfSize = fileHeader.bfOffBits + infoHeader.biSizeImage;
        int totalSize = fileHeader.bfSize;
        unsigned char* bmpData = (unsigned char*)malloc(totalSize);
        if (!bmpData) {
            delete[] thumbData;
            delete[] grayData;
            SetLastError("Out of memory");
            return NATIVE_E_INTERNAL_ERROR;
        }
        unsigned char* p = bmpData;
        memcpy(p, &fileHeader, sizeof(fileHeader)); p += sizeof(fileHeader);
        memcpy(p, &infoHeader, sizeof(infoHeader)); p += sizeof(infoHeader);
        for (int i = 0; i < 256; ++i) {
            unsigned char color[4] = { (unsigned char)i, (unsigned char)i, (unsigned char)i, 0 };
            memcpy(p, color, 4); p += 4;
        }
        unsigned char* paddingBytes = new unsigned char[rowPadding]();
        for (int y = thumbHeight - 1; y >= 0; --y) {
            memcpy(p, &grayData[y * thumbWidth], thumbWidth); p += thumbWidth;
            if (rowPadding > 0) { memcpy(p, paddingBytes, rowPadding); p += rowPadding; }
        }
        delete[] paddingBytes;
        delete[] thumbData;
        delete[] grayData;
        *outData = bmpData;
        *outSize = totalSize;
        return NATIVE_OK;
    } catch (const std::exception& e) {
        SetLastError(std::string("Generate thumbnail BMP failed: ") + e.what());
        return NATIVE_E_INTERNAL_ERROR;
    }
}

DICOM_API const char* Dicom_GetLastError() {
    return g_lastError.c_str();
}

// ========== ���� Handle ���������� ===========

DICOM_API DicomReaderHandle Dicom_CreateReader() {
    try {
        auto* ctx = new DicomReaderContext();
        return static_cast<DicomReaderHandle>(ctx);
    } catch (const std::exception& e) {
        SetLastError(std::string("Failed to create reader: ") + e.what());
        return nullptr;
    }
}

DICOM_API DicomHandle Dicom_CreateHandle() {
    try {
        auto* ctx = new DicomReaderContext();
        return static_cast<DicomHandle>(ctx);
    } catch (const std::exception& e) {
        SetLastError(std::string("Failed to create handle: ") + e.what());
        return nullptr;
    }
}

DICOM_API VolumeHandle Dicom_CreateVolume() {
    try {
        auto* ctx = new VolumeContext();
        return static_cast<VolumeHandle>(ctx);
    } catch (const std::exception& e) {
        SetLastError(std::string("Failed to create volume: ") + e.what());
        return nullptr;
    }
}

DICOM_API void Dicom_DestroyReader(DicomReaderHandle handle) {
    if (!handle) return;
    auto* ctx = static_cast<DicomReaderContext*>(handle);
    delete ctx;
}

DICOM_API void Dicom_DestroyHandle(DicomHandle handle) {
    if (!handle) return;
    auto* ctx = static_cast<DicomReaderContext*>(handle);
    delete ctx;
}

DICOM_API void Dicom_DestroyVolume(VolumeHandle handle) {
    if (!handle) return;
    auto* ctx = static_cast<VolumeContext*>(handle);
    delete ctx;
}

// ==================== DICOM Reader ====================
DICOM_API NativeResult Dicom_ReadFile(DicomReaderHandle handle, const char* filepath) {
    if (!handle || !filepath) {
        SetLastError("Invalid handle or filepath");
        return NATIVE_E_INVALID_ARGUMENT;
    }

    auto* ctx = static_cast<DicomReaderContext*>(handle);
    
    try {
        // ����ļ��Ƿ����
        if (!fs::exists(filepath)) {
            SetLastError(std::string("File does not exist: ") + filepath);
            return NATIVE_E_INVALID_ARGUMENT;
        }
        
        ctx->filepath = filepath;
        
        // ת��Ϊ UTF-8 ·����GDCM ��Ҫ��- ʹ��ͳһ�ı��빤��
        std::string utf8path = EncodingUtils::GetUtf8Path(filepath);
        
        ctx->reader.SetFileName(utf8path.c_str());
        
        if (!ctx->reader.Read()) {
            SetLastError("Failed to read DICOM file: " + std::string(filepath));
            return NATIVE_E_INTERNAL_ERROR;
        }
        
        ctx->isLoaded = true;
        return NATIVE_OK;
    } catch (const std::exception& e) {
        SetLastError(std::string("Exception reading file: ") + e.what());
        return NATIVE_E_INTERNAL_ERROR;
    }
}

DICOM_API NativeResult Dicom_ReadDirectory(DicomReaderHandle handle, const char* directory, int* fileCount) {
    if (!handle || !directory) {
        SetLastError("Invalid handle or directory");
        return NATIVE_E_INVALID_ARGUMENT;
    }

    try {
        int count = 0;
        for (const auto& entry : fs::directory_iterator(directory)) {
            if (entry.is_regular_file()) {
                auto ext = entry.path().extension().string();
                std::transform(ext.begin(), ext.end(), ext.begin(), ::tolower);
                if (ext == ".dcm" || ext == ".dicom" || ext.empty()) {
                    count++;
                }
            }
        }
        
        if (fileCount) {
            *fileCount = count;
        }
        
        return NATIVE_OK;
    } catch (const std::exception& e) {
        SetLastError(std::string("Failed to read directory: ") + e.what());
        return NATIVE_E_INTERNAL_ERROR;
    }
}

// ==================== Tag ���� ====================
DICOM_API NativeResult Dicom_GetTag(DicomReaderHandle handle, uint16_t group, uint16_t element, 
                                     char* buffer, int bufferSize) {
    if (!handle || !buffer || bufferSize <= 0) {
        SetLastError("Invalid parameters");
        return NATIVE_E_INVALID_ARGUMENT;
    }

    auto* ctx = static_cast<DicomReaderContext*>(handle);
    if (!ctx->isLoaded) {
        SetLastError("No DICOM file loaded");
        return NATIVE_E_NOT_INITIALIZED;
    }

    try {
        const gdcm::DataSet& ds = ctx->reader.GetFile().GetDataSet();
        gdcm::Tag tag(group, element);
        
        if (!ds.FindDataElement(tag)) {
            SetLastError("Tag not found");
            return NATIVE_E_INTERNAL_ERROR;
        }

        const gdcm::DataElement& de = ds.GetDataElement(tag);
        const gdcm::ByteValue* bv = de.GetByteValue();
        
        if (!bv) {
            buffer[0] = '\0';
            return NATIVE_OK;
        }

        std::string value(bv->GetPointer(), bv->GetLength());
        
        // �Ƴ�ĩβ�Ŀո�Ϳ��ַ�
        value.erase(value.find_last_not_of(" \0\t\n\r\f\v") + 1);
        
        // ת��Ϊ UTF-8���������� Tag��
        strncpy_s(buffer, bufferSize, value.c_str(), _TRUNCATE);
        
        return NATIVE_OK;
    } catch (const std::exception& e) {
        SetLastError(std::string("Failed to get tag: ") + e.what());
        return NATIVE_E_INTERNAL_ERROR;
    }
}

DICOM_API NativeResult Dicom_GetImageSize(DicomReaderHandle handle, int* width, int* height) {
    if (!handle || !width || !height) {
        SetLastError("Invalid parameters");
        return NATIVE_E_INVALID_ARGUMENT;
    }

    auto* ctx = static_cast<DicomReaderContext*>(handle);
    if (!ctx->isLoaded) {
        SetLastError("No DICOM file loaded");
        return NATIVE_E_NOT_INITIALIZED;
    }

    try {
        const gdcm::Image& image = ctx->reader.GetImage();
        const unsigned int* dims = image.GetDimensions();
        
        *width = dims[0];
        *height = dims[1];
        
        return NATIVE_OK;
    } catch (const std::exception& e) {
        SetLastError(std::string("Failed to get image size: ") + e.what());
        return NATIVE_E_INTERNAL_ERROR;
    }
}

DICOM_API void* Dicom_GetPixelData(DicomHandle handle, int* width, int* height, int* depth) {
    if (!handle) {
        SetLastError("Invalid handle");
        return nullptr;
    }

    auto* ctx = static_cast<DicomReaderContext*>(handle);
    if (!ctx->isLoaded) {
        SetLastError("No DICOM file loaded");
        return nullptr;
    }

    try {
        const gdcm::Image& image = ctx->reader.GetImage();
        const unsigned int* dims = image.GetDimensions();
        
        // �����������
        if (width) *width = dims[0];
        if (height) *height = dims[1];
        if (depth) *depth = (dims[2] > 0) ? dims[2] : 1;
        
        // �����������ݴ�С
        unsigned long bufferSize = image.GetBufferLength();
        
        if (bufferSize == 0) {
            SetLastError("No pixel data available");
            return nullptr;
        }
        
        // �����ڴ�洢��������
        char* buffer = new char[bufferSize];
        
        // ��ȡ��������
        if (!image.GetBuffer(buffer)) {
            delete[] buffer;
            SetLastError("Failed to get pixel buffer");
            return nullptr;
        }
        
        // ע�⣺��������Ҫ�����ͷ�����ڴ�
        return static_cast<void*>(buffer);
        
    } catch (const std::exception& e) {
        SetLastError(std::string("Failed to get pixel data: ") + e.what());
        return nullptr;
    }
}

// ==================== Volume ���� ====================
DICOM_API VolumeHandle Dicom_Volume_Create() {
    try {
        auto* vol = new VolumeContext();
        return static_cast<VolumeHandle>(vol);
    } catch (const std::exception& e) {
        SetLastError(std::string("Failed to create volume: ") + e.what());
        return nullptr;
    }
}

DICOM_API void Dicom_Volume_Destroy(VolumeHandle handle) {
    if (handle) {
        delete static_cast<VolumeContext*>(handle);
    }
}

DICOM_API NativeResult Dicom_Volume_LoadFromDicomSeries(VolumeHandle handle, const char* directory) {
    if (!handle || !directory) {
        SetLastError("Invalid handle or directory");
        return NATIVE_E_INVALID_ARGUMENT;
    }

    auto* vol = static_cast<VolumeContext*>(handle);

    try {
        // �ռ����� DICOM �ļ�
        std::vector<std::string> dicomFiles;
        for (const auto& entry : fs::directory_iterator(directory)) {
            if (entry.is_regular_file()) {
                auto ext = entry.path().extension().string();
                std::transform(ext.begin(), ext.end(), ext.begin(), ::tolower);
                if (ext == ".dcm" || ext == ".dicom" || ext.empty()) {
                    dicomFiles.push_back(entry.path().string());
                }
            }
        }

        if (dicomFiles.empty()) {
            SetLastError("No DICOM files found in directory");
            return NATIVE_E_INTERNAL_ERROR;
        }

        // ��ȡ�����ļ��� Instance Number ������
        struct DicomFileInfo {
            std::string filename;
            int instanceNumber;
            double imagePosition;  // Image Position (Patient) Z ����
        };
        
        std::vector<DicomFileInfo> fileInfos;
        
        for (const auto& file : dicomFiles) {
            gdcm::ImageReader reader;
            // ʹ��ͳһ�ı��빤�߽� GBK תΪ UTF-8
            std::string utf8path = EncodingUtils::GetUtf8Path(file.c_str());
            reader.SetFileName(utf8path.c_str());
            
            if (!reader.Read()) continue;
            
            const gdcm::DataSet& ds = reader.GetFile().GetDataSet();
            DicomFileInfo info;
            info.filename = file;
            info.instanceNumber = 0;
            info.imagePosition = 0.0;
            
            // ��ȡ Instance Number (0020,0013)
            gdcm::Tag instanceTag(0x0020, 0x0013);
            if (ds.FindDataElement(instanceTag)) {
                const gdcm::DataElement& de = ds.GetDataElement(instanceTag);
                const gdcm::ByteValue* bv = de.GetByteValue();
                if (bv) {
                    std::string value(bv->GetPointer(), bv->GetLength());
                    info.instanceNumber = std::stoi(value);
                }
            }
            
            // ��ȡ Image Position (Patient) (0020,0032) �� Z ������Ϊ��������
            gdcm::Tag positionTag(0x0020, 0x0032);
            if (ds.FindDataElement(positionTag)) {
                const gdcm::DataElement& de = ds.GetDataElement(positionTag);
                const gdcm::ByteValue* bv = de.GetByteValue();
                if (bv) {
                    std::string value(bv->GetPointer(), bv->GetLength());
                    // ��ʽ: "x\y\z"
                    size_t pos1 = value.find('\\');
                    size_t pos2 = value.find('\\', pos1 + 1);
                    if (pos2 != std::string::npos) {
                        std::string zStr = value.substr(pos2 + 1);
                        info.imagePosition = std::stod(zStr);
                    }
                }
            }
            
            fileInfos.push_back(info);
        }
        
        // �� Instance Number ���������ͬ�� Image Position ����
        std::sort(fileInfos.begin(), fileInfos.end(), 
            [](const DicomFileInfo& a, const DicomFileInfo& b) {
                if (a.instanceNumber != b.instanceNumber) {
                    return a.instanceNumber < b.instanceNumber;
                }
                return a.imagePosition < b.imagePosition;
            });
        
        // �����������ļ��б�
        dicomFiles.clear();
        for (const auto& info : fileInfos) {
            dicomFiles.push_back(info.filename);
        }

        // ��ȡ��һ���ļ���ȡ�ߴ���Ϣ
        gdcm::ImageReader firstReader;
        
        // ʹ��ͳһ�ı��빤�߽� GBK תΪ UTF-8
        std::string utf8path = EncodingUtils::GetUtf8Path(dicomFiles[0].c_str());
        
        firstReader.SetFileName(utf8path.c_str());
        if (!firstReader.Read()) {
            SetLastError("Failed to read first DICOM file");
            return NATIVE_E_INTERNAL_ERROR;
        }

        const gdcm::Image& firstImage = firstReader.GetImage();
        const unsigned int* dims = firstImage.GetDimensions();
        vol->width = dims[0];
        vol->height = dims[1];
        vol->depth = static_cast<int>(dicomFiles.size());

        // �� DICOM Tag ֱ�Ӷ�ȡ Pixel Spacing (0028,0030)
        const gdcm::DataSet& ds = firstReader.GetFile().GetDataSet();
        gdcm::Tag pixelSpacingTag(0x0028, 0x0030); // Pixel Spacing
        
        if (ds.FindDataElement(pixelSpacingTag)) {
            const gdcm::DataElement& de = ds.GetDataElement(pixelSpacingTag);
            const gdcm::ByteValue* bv = de.GetByteValue();
            if (bv) {
                std::string value(bv->GetPointer(), bv->GetLength());
                // Pixel Spacing ��ʽ��"row_spacing\column_spacing"
                size_t pos = value.find('\\');
                if (pos != std::string::npos) {
                    std::string rowSpacing = value.substr(0, pos);
                    std::string colSpacing = value.substr(pos + 1);
                    vol->spacingY = std::stof(rowSpacing);
                    vol->spacingX = std::stof(colSpacing);
                    vol->spacingZ = vol->spacingX; // Z ����ʹ����ͬ�ļ��
                } else {
                    // ֻ��һ��ֵ
                    float spacing = std::stof(value);
                    vol->spacingX = vol->spacingY = vol->spacingZ = spacing;
                }
            }
        } else {
            // ���û�� Pixel Spacing tag��ʹ��Ĭ��ֵ
            vol->spacingX = vol->spacingY = vol->spacingZ = 1.0f;
        }

        // Rescale Slope/Intercept (0028,1053)/(0028,1052)
        // If tags are missing, keep defaults (1,0) so stored values are preserved.
        vol->rescaleSlope = 1.0f;
        vol->rescaleIntercept = 0.0f;
        {
            gdcm::Tag slopeTag(0x0028, 0x1053);
            gdcm::Tag interceptTag(0x0028, 0x1052);
            if (ds.FindDataElement(slopeTag)) {
                const gdcm::DataElement& de = ds.GetDataElement(slopeTag);
                const gdcm::ByteValue* bv = de.GetByteValue();
                if (bv) {
                    std::string value(bv->GetPointer(), bv->GetLength());
                    float slope = 0.0f;
                    if (TryParseFirstFloat(value, &slope) && slope != 0.0f) {
                        vol->rescaleSlope = slope;
                    }
                }
            }
            if (ds.FindDataElement(interceptTag)) {
                const gdcm::DataElement& de = ds.GetDataElement(interceptTag);
                const gdcm::ByteValue* bv = de.GetByteValue();
                if (bv) {
                    std::string value(bv->GetPointer(), bv->GetLength());
                    float intercept = 0.0f;
                    if (TryParseFirstFloat(value, &intercept)) {
                        vol->rescaleIntercept = intercept;
                    }
                }
            }
        }

        // �����ڴ�
        size_t totalSize = static_cast<size_t>(vol->width) * vol->height * vol->depth;
        vol->data.resize(totalSize);

        // ��ȡ������Ƭ
        for (size_t i = 0; i < dicomFiles.size(); ++i) {
            gdcm::ImageReader reader;
            
            // ʹ��ͳһ�ı��빤�߽� GBK תΪ UTF-8
            std::string utf8path = EncodingUtils::GetUtf8Path(dicomFiles[i].c_str());
            reader.SetFileName(utf8path.c_str());
            
            if (!reader.Read()) {
                continue;
            }

            const gdcm::Image& image = reader.GetImage();
            size_t sliceSize = vol->width * vol->height;
            
            // ��ȡ��������
            std::vector<char> buffer(sliceSize * 2); // ���� 16-bit
            image.GetBuffer(buffer.data());
            
            // ���Ƶ� volume
            uint16_t* src = reinterpret_cast<uint16_t*>(buffer.data());
            uint16_t* dst = vol->data.data() + (i * sliceSize);
            std::copy(src, src + sliceSize, dst);
        }

        return NATIVE_OK;
    } catch (const std::exception& e) {
        SetLastError(std::string("Failed to load DICOM series: ") + e.what());
        return NATIVE_E_INTERNAL_ERROR;
    }
}

DICOM_API NativeResult Dicom_Volume_Allocate(VolumeHandle handle, int width, int height, int depth) {
    if (!handle || width <= 0 || height <= 0 || depth <= 0) {
        SetLastError("Invalid handle or dimensions");
        return NATIVE_E_INVALID_ARGUMENT;
    }

    auto* vol = static_cast<VolumeContext*>(handle);

    const uint64_t w = static_cast<uint64_t>(width);
    const uint64_t h = static_cast<uint64_t>(height);
    const uint64_t d = static_cast<uint64_t>(depth);
    const uint64_t totalVoxels64 = w * h * d;

    // Guard against overflow on size_t and vector allocation.
    if (w == 0 || h == 0 || d == 0) {
        SetLastError("Invalid dimensions");
        return NATIVE_E_INVALID_ARGUMENT;
    }
    if (totalVoxels64 > static_cast<uint64_t>(std::numeric_limits<size_t>::max())) {
        SetLastError("Volume too large");
        return NATIVE_E_INTERNAL_ERROR;
    }

    try {
        vol->width = width;
        vol->height = height;
        vol->depth = depth;
        vol->data.resize(static_cast<size_t>(totalVoxels64));
        return NATIVE_OK;
    } catch (const std::exception& e) {
        SetLastError(std::string("Failed to allocate volume: ") + e.what());
        return NATIVE_E_INTERNAL_ERROR;
    }
}

DICOM_API NativeResult Dicom_Volume_LoadFromDicomSeriesWithProgress(VolumeHandle handle, const char* directory, ProgressCallback callback) {
    if (!handle || !directory) {
        SetLastError("Invalid handle or directory");
        return NATIVE_E_INVALID_ARGUMENT;
    }

    auto* vol = static_cast<VolumeContext*>(handle);

    try {
        // Step 1: �ռ����� DICOM �ļ�
        if (callback) callback(0, "Scanning directory...");
        
        std::vector<std::string> dicomFiles;
        for (const auto& entry : fs::directory_iterator(directory)) {
            if (entry.is_regular_file()) {
                auto ext = entry.path().extension().string();
                std::transform(ext.begin(), ext.end(), ext.begin(), ::tolower);
                if (ext == ".dcm" || ext == ".dicom" || ext.empty()) {
                    dicomFiles.push_back(entry.path().string());
                }
            }
        }

        if (dicomFiles.empty()) {
            SetLastError("No DICOM files found in directory");
            return NATIVE_E_INTERNAL_ERROR;
        }

        // Step 2: ��ȡ�����ļ��� Instance Number ������
        if (callback) callback(5, "Sorting files...");
        
        struct DicomFileInfo {
            std::string filename;
            int instanceNumber;
            double imagePosition;
        };
        
        std::vector<DicomFileInfo> fileInfos;
        size_t totalFiles = dicomFiles.size();
        
        for (size_t idx = 0; idx < totalFiles; ++idx) {
            const auto& file = dicomFiles[idx];
            
            // ÿ���� 10% �ļ�����һ�ν���
            if (callback && idx % (totalFiles / 10 + 1) == 0) {
                int progress = 5 + static_cast<int>((idx * 15.0) / totalFiles);
                callback(progress, "Reading file metadata...");
            }
            
            gdcm::ImageReader reader;
            // ʹ��ͳһ�ı��빤�߽� GBK תΪ UTF-8
            std::string utf8path = EncodingUtils::GetUtf8Path(file.c_str());
            reader.SetFileName(utf8path.c_str());
            
            if (!reader.Read()) continue;
            
            const gdcm::DataSet& ds = reader.GetFile().GetDataSet();
            DicomFileInfo info;
            info.filename = file;
            info.instanceNumber = 0;
            info.imagePosition = 0.0;
            
            gdcm::Tag instanceTag(0x0020, 0x0013);
            if (ds.FindDataElement(instanceTag)) {
                const gdcm::DataElement& de = ds.GetDataElement(instanceTag);
                const gdcm::ByteValue* bv = de.GetByteValue();
                if (bv) {
                    std::string value(bv->GetPointer(), bv->GetLength());
                    info.instanceNumber = std::stoi(value);
                }
            }
            
            gdcm::Tag positionTag(0x0020, 0x0032);
            if (ds.FindDataElement(positionTag)) {
                const gdcm::DataElement& de = ds.GetDataElement(positionTag);
                const gdcm::ByteValue* bv = de.GetByteValue();
                if (bv) {
                    std::string value(bv->GetPointer(), bv->GetLength());
                    size_t pos1 = value.find('\\');
                    size_t pos2 = value.find('\\', pos1 + 1);
                    if (pos2 != std::string::npos) {
                        std::string zStr = value.substr(pos2 + 1);
                        info.imagePosition = std::stod(zStr);
                    }
                }
            }
            
            fileInfos.push_back(info);
        }
        
        if (callback) callback(20, "Sorting files by instance number...");
        
        std::sort(fileInfos.begin(), fileInfos.end(), 
            [](const DicomFileInfo& a, const DicomFileInfo& b) {
                if (a.instanceNumber != b.instanceNumber) {
                    return a.instanceNumber < b.instanceNumber;
                }
                return a.imagePosition < b.imagePosition;
            });
        
        dicomFiles.clear();
        for (const auto& info : fileInfos) {
            dicomFiles.push_back(info.filename);
        }

        // Step 3: ��ȡ��һ���ļ���ȡ�ߴ���Ϣ
        if (callback) callback(25, "Reading image dimensions...");
        
        gdcm::ImageReader firstReader;
        
        // ʹ��ͳһ�ı��빤�߽� GBK תΪ UTF-8
        std::string utf8path = EncodingUtils::GetUtf8Path(dicomFiles[0].c_str());
        
        firstReader.SetFileName(utf8path.c_str());
        if (!firstReader.Read()) {
            SetLastError("Failed to read first DICOM file");
            return NATIVE_E_INTERNAL_ERROR;
        }

        const gdcm::Image& firstImage = firstReader.GetImage();
        const unsigned int* dims = firstImage.GetDimensions();
        vol->width = dims[0];
        vol->height = dims[1];
        vol->depth = static_cast<int>(dicomFiles.size());

        // ��ȡ Pixel Spacing
        const gdcm::DataSet& ds = firstReader.GetFile().GetDataSet();
        gdcm::Tag pixelSpacingTag(0x0028, 0x0030);
        
        if (ds.FindDataElement(pixelSpacingTag)) {
            const gdcm::DataElement& de = ds.GetDataElement(pixelSpacingTag);
            const gdcm::ByteValue* bv = de.GetByteValue();
            if (bv) {
                std::string value(bv->GetPointer(), bv->GetLength());
                size_t pos = value.find('\\');
                if (pos != std::string::npos) {
                    std::string rowSpacing = value.substr(0, pos);
                    std::string colSpacing = value.substr(pos + 1);
                    vol->spacingY = std::stof(rowSpacing);
                    vol->spacingX = std::stof(colSpacing);
                    vol->spacingZ = vol->spacingX;
                } else {
                    float spacing = std::stof(value);
                    vol->spacingX = vol->spacingY = vol->spacingZ = spacing;
                }
            }
        } else {
            vol->spacingX = vol->spacingY = vol->spacingZ = 1.0f;
        }

        // Rescale Slope/Intercept (0028,1053)/(0028,1052)
        vol->rescaleSlope = 1.0f;
        vol->rescaleIntercept = 0.0f;
        {
            gdcm::Tag slopeTag(0x0028, 0x1053);
            gdcm::Tag interceptTag(0x0028, 0x1052);
            if (ds.FindDataElement(slopeTag)) {
                const gdcm::DataElement& de = ds.GetDataElement(slopeTag);
                const gdcm::ByteValue* bv = de.GetByteValue();
                if (bv) {
                    std::string value(bv->GetPointer(), bv->GetLength());
                    float slope = 0.0f;
                    if (TryParseFirstFloat(value, &slope) && slope != 0.0f) {
                        vol->rescaleSlope = slope;
                    }
                }
            }
            if (ds.FindDataElement(interceptTag)) {
                const gdcm::DataElement& de = ds.GetDataElement(interceptTag);
                const gdcm::ByteValue* bv = de.GetByteValue();
                if (bv) {
                    std::string value(bv->GetPointer(), bv->GetLength());
                    float intercept = 0.0f;
                    if (TryParseFirstFloat(value, &intercept)) {
                        vol->rescaleIntercept = intercept;
                    }
                }
            }
        }

        // Step 4: �����ڴ�
        if (callback) callback(30, "Allocating memory...");
        
        size_t totalSize = static_cast<size_t>(vol->width) * vol->height * vol->depth;
        vol->data.resize(totalSize);

        // Step 5: ��ȡ������Ƭ��30% - 100%��ÿ����Ƭ���½��ȣ�
        for (size_t i = 0; i < dicomFiles.size(); ++i) {
            // ÿ��ȡһ����Ƭ���½���
            if (callback) {
                int progress = 30 + static_cast<int>((i * 70.0) / dicomFiles.size());
                char msg[256];
                sprintf_s(msg, sizeof(msg), "Loading slice %d/%d...", (int)(i + 1), (int)dicomFiles.size());
                callback(progress, msg);
            }
            
            gdcm::ImageReader reader;
            
            // ʹ��ͳһ�ı��빤�߽� GBK תΪ UTF-8
            std::string utf8path = EncodingUtils::GetUtf8Path(dicomFiles[i].c_str());
            reader.SetFileName(utf8path.c_str());
            
            if (!reader.Read()) {
                continue;
            }

            const gdcm::Image& image = reader.GetImage();
            size_t sliceSize = vol->width * vol->height;
            
            std::vector<char> buffer(sliceSize * 2);
            image.GetBuffer(buffer.data());
            
            uint16_t* src = reinterpret_cast<uint16_t*>(buffer.data());
            uint16_t* dst = vol->data.data() + (i * sliceSize);
            std::copy(src, src + sliceSize, dst);
        }

        if (callback) callback(100, "Completed");
        
        return NATIVE_OK;
    } catch (const std::exception& e) {
        SetLastError(std::string("Failed to load DICOM series: ") + e.what());
        return NATIVE_E_INTERNAL_ERROR;
    }
}

DICOM_API NativeResult Dicom_Volume_GetDimensions(VolumeHandle handle, int* width, int* height, int* depth) {
    if (!handle || !width || !height || !depth) {
        SetLastError("Invalid parameters");
        return NATIVE_E_INVALID_ARGUMENT;
    }

    auto* vol = static_cast<VolumeContext*>(handle);
    *width = vol->width;
    *height = vol->height;
    *depth = vol->depth;
    
    return NATIVE_OK;
}

DICOM_API NativeResult Dicom_Volume_GetSpacing(VolumeHandle handle, float* spacingX, float* spacingY, float* spacingZ) {
    if (!handle || !spacingX || !spacingY || !spacingZ) {
        SetLastError("Invalid parameters");
        return NATIVE_E_INVALID_ARGUMENT;
    }

    auto* vol = static_cast<VolumeContext*>(handle);
    *spacingX = vol->spacingX;
    *spacingY = vol->spacingY;
    *spacingZ = vol->spacingZ;
    
    return NATIVE_OK;
}

DICOM_API NativeResult Dicom_Volume_GetRescale(VolumeHandle handle, float* slope, float* intercept) {
    if (!handle || !slope || !intercept) {
        SetLastError("Invalid parameters");
        return NATIVE_E_INVALID_ARGUMENT;
    }

    auto* vol = static_cast<VolumeContext*>(handle);
    *slope = vol->rescaleSlope;
    *intercept = vol->rescaleIntercept;
    return NATIVE_OK;
}

DICOM_API NativeResult Dicom_Volume_SetSpacing(VolumeHandle handle, float spacingX, float spacingY, float spacingZ) {
    if (!handle) {
        SetLastError("Invalid parameters");
        return NATIVE_E_INVALID_ARGUMENT;
    }

    auto* vol = static_cast<VolumeContext*>(handle);
    vol->spacingX = spacingX;
    vol->spacingY = spacingY;
    vol->spacingZ = spacingZ;
    return NATIVE_OK;
}

DICOM_API void* Dicom_Volume_GetData(VolumeHandle handle) {
    if (!handle) {
        return nullptr;
    }

    auto* vol = static_cast<VolumeContext*>(handle);
    return vol->data.data();
}

// ==================== DICOM Series ���� ====================

DICOM_API DicomSeriesHandle Dicom_Series_Create() {
    try {
        auto* ctx = new DicomSeriesContext();
        return static_cast<DicomSeriesHandle>(ctx);
    } catch (const std::exception& e) {
        SetLastError(std::string("Failed to create series: ") + e.what());
        return nullptr;
    }
}

DICOM_API void Dicom_Series_Destroy(DicomSeriesHandle handle) {
    if (!handle) return;
    auto* ctx = static_cast<DicomSeriesContext*>(handle);
    delete ctx;
}

DICOM_API NativeResult Dicom_Series_ScanFolder(DicomSeriesHandle handle, const char* folderPath) {
    if (!handle || !folderPath) {
        SetLastError("Invalid handle or folder path");
        return NATIVE_E_INVALID_ARGUMENT;
    }

    auto* ctx = static_cast<DicomSeriesContext*>(handle);
    
    try {
        ctx->folderPath = folderPath;
        ctx->filePaths.clear();
        ctx->isScanned = false;

        // �����ļ���
        if (!fs::exists(folderPath)) {
            SetLastError("Folder does not exist");
            return NATIVE_E_INVALID_ARGUMENT;
        }

        for (const auto& entry : fs::directory_iterator(folderPath)) {
            if (entry.is_regular_file()) {
                std::string filename = entry.path().filename().string();
                // �򵥼�飺DICOM �ļ�ͨ���� .dcm ������չ��
                if (filename.find(".dcm") != std::string::npos || 
                    filename.find(".dicom") != std::string::npos ||
                    filename.find('.') == std::string::npos) {
                    ctx->filePaths.push_back(entry.path().string());
                }
            }
        }

        if (ctx->filePaths.empty()) {
            SetLastError("No DICOM files found");
            return NATIVE_E_INTERNAL_ERROR;
        }

        // �����ļ�
        std::sort(ctx->filePaths.begin(), ctx->filePaths.end());

        // ��ȡ��һ���ļ�
        // ʹ��ͳһ�ı��빤�߽� GBK תΪ UTF-8
        std::string utf8path = EncodingUtils::GetUtf8Path(ctx->filePaths[0].c_str());
        
        ctx->firstReader.SetFileName(utf8path.c_str());
        if (!ctx->firstReader.Read()) {
            SetLastError("Failed to read first DICOM file");
            return NATIVE_E_INTERNAL_ERROR;
        }

        ctx->isScanned = true;
        return NATIVE_OK;

    } catch (const std::exception& e) {
        SetLastError(std::string("Scan folder failed: ") + e.what());
        return NATIVE_E_INTERNAL_ERROR;
    }
}

DICOM_API int Dicom_Series_GetFileCount(DicomSeriesHandle handle) {
    if (!handle) return 0;
    auto* ctx = static_cast<DicomSeriesContext*>(handle);
    return static_cast<int>(ctx->filePaths.size());
}

DICOM_API const char* Dicom_Series_GetFilePath(DicomSeriesHandle handle, int index) {
    if (!handle) return nullptr;
    auto* ctx = static_cast<DicomSeriesContext*>(handle);
    if (index < 0 || index >= ctx->filePaths.size()) return nullptr;
    return ctx->filePaths[index].c_str();
}

DICOM_API NativeResult Dicom_Series_GetTag(DicomSeriesHandle handle, uint16_t group, uint16_t element, char* buffer, int bufferSize) {
    if (!handle || !buffer || bufferSize <= 0) {
        SetLastError("Invalid arguments");
        return NATIVE_E_INVALID_ARGUMENT;
    }

    auto* ctx = static_cast<DicomSeriesContext*>(handle);
    if (!ctx->isScanned) {
        SetLastError("Series not scanned");
        return NATIVE_E_NOT_INITIALIZED;
    }

    try {
        const gdcm::DataSet& ds = ctx->firstReader.GetFile().GetDataSet();
        gdcm::Tag tag(group, element);

        if (!ds.FindDataElement(tag)) {
            buffer[0] = '\0';
            return NATIVE_OK;
        }

        const gdcm::DataElement& de = ds.GetDataElement(tag);
        std::string value;
        
        if (de.IsEmpty()) {
            buffer[0] = '\0';
        } else {
            const gdcm::ByteValue* bv = de.GetByteValue();
            if (bv) {
                value = std::string(bv->GetPointer(), bv->GetLength());
                // �Ƴ�β���ո�
                value.erase(value.find_last_not_of(" \t\n\r\f\v") + 1);
            }
            strncpy_s(buffer, bufferSize, value.c_str(), _TRUNCATE);
        }

        return NATIVE_OK;

    } catch (const std::exception& e) {
        SetLastError(std::string("Get tag failed: ") + e.what());
        return NATIVE_E_INTERNAL_ERROR;
    }
}

DICOM_API NativeResult Dicom_Series_GetImageSize(DicomSeriesHandle handle, int* width, int* height) {
    if (!handle || !width || !height) {
        SetLastError("Invalid arguments");
        return NATIVE_E_INVALID_ARGUMENT;
    }

    auto* ctx = static_cast<DicomSeriesContext*>(handle);
    if (!ctx->isScanned) {
        SetLastError("Series not scanned");
        return NATIVE_E_NOT_INITIALIZED;
    }

    try {
        const gdcm::Image& image = ctx->firstReader.GetImage();
        const unsigned int* dims = image.GetDimensions();
        *width = dims[0];
        *height = dims[1];
        return NATIVE_OK;

    } catch (const std::exception& e) {
        SetLastError(std::string("Get image size failed: ") + e.what());
        return NATIVE_E_INTERNAL_ERROR;
    }
}

DICOM_API NativeResult Dicom_Series_GenerateThumbnail(DicomSeriesHandle handle, int thumbSize, unsigned char* buffer, int bufferSize) {
    if (!handle || !buffer || bufferSize <= 0) {
        SetLastError("Invalid arguments");
        return NATIVE_E_INVALID_ARGUMENT;
    }

    auto* ctx = static_cast<DicomSeriesContext*>(handle);
    if (!ctx->isScanned) {
        SetLastError("Series not scanned");
        return NATIVE_E_NOT_INITIALIZED;
    }

    try {
        const gdcm::Image& image = ctx->firstReader.GetImage();
        const unsigned int* dims = image.GetDimensions();
        int width = dims[0];
        int height = dims[1];

        // ��� buffer ��С
        int requiredSize = thumbSize * thumbSize * 4; // RGBA
        if (bufferSize < requiredSize) {
            SetLastError("Buffer too small");
            return NATIVE_E_INVALID_ARGUMENT;
        }

        // ��ȡ��������
        std::vector<char> pixelBuffer(image.GetBufferLength());
        image.GetBuffer(pixelBuffer.data());
        
        const short* pixelData = reinterpret_cast<const short*>(pixelBuffer.data());
        size_t pixelCount = width * height;

        // ���㴰����λ
        short minVal = *std::min_element(pixelData, pixelData + pixelCount);
        short maxVal = *std::max_element(pixelData, pixelData + pixelCount);

        // ��������ͼ��˫���Բ�ֵ��
        float scaleX = static_cast<float>(width) / thumbSize;
        float scaleY = static_cast<float>(height) / thumbSize;

        for (int y = 0; y < thumbSize; ++y) {
            for (int x = 0; x < thumbSize; ++x) {
                float srcX = x * scaleX;
                float srcY = y * scaleY;

                int x0 = static_cast<int>(srcX);
                int y0 = static_cast<int>(srcY);
                int x1 = std::min(x0 + 1, width - 1);
                int y1 = std::min(y0 + 1, height - 1);

                float fx = srcX - x0;
                float fy = srcY - y0;

                short v00 = pixelData[y0 * width + x0];
                short v10 = pixelData[y0 * width + x1];
                short v01 = pixelData[y1 * width + x0];
                short v11 = pixelData[y1 * width + x1];

                float v = (1 - fx) * (1 - fy) * v00 +
                          fx * (1 - fy) * v10 +
                          (1 - fx) * fy * v01 +
                          fx * fy * v11;

                // ��һ���� 0-255
                unsigned char gray = static_cast<unsigned char>(
                    std::clamp((v - minVal) * 255.0f / (maxVal - minVal), 0.0f, 255.0f)
                );

                int idx = (y * thumbSize + x) * 4;
                buffer[idx] = gray;
                buffer[idx + 1] = gray;
                buffer[idx + 2] = gray;
                buffer[idx + 3] = 255;
            }
        }

        return NATIVE_OK;

    } catch (const std::exception& e) {
        SetLastError(std::string("Generate thumbnail failed: ") + e.what());
        return NATIVE_E_INTERNAL_ERROR;
    }
}

} // extern "C"
