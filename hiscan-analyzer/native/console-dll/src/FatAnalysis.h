#pragma once
#include <vector>
#include <utility>
#include <cstdint>

class FatAnalysis {
public:
    // Returns pair of buffers: {visceral, subcutaneous}
    // Caller owns the returned pointers.
    static std::pair<unsigned char*, unsigned char*> SeprateFat(
        const short* pData, 
        int nWidth, int nHeight, int nDepth, 
        int nowPos, 
        int minFat, int maxFat
    );

    // Returns vector of 4 buffers: {visceral, subcutaneous, lung, bone}
    static std::vector<unsigned char*> SeprateLung(
        const short* pData,
        int nWidth, int nHeight, int nDepth,
        int minFat, int maxFat,
        int minLung, int maxLung,
        int minBone, int maxBone
    );
};

