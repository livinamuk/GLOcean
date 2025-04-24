#pragma once
#include <string>
#include <vector>
#include "HellTypes.h"

enum class CompressionType { DXT3, BC5, UNDEFINED };

namespace ImageTools {
    // Compresesenator
    void InitializeCMPFramework();
    bool IsCMPFrameworkInitialized();
    void CreateAndExportDDS(const std::string& inputFilepath, const std::string& outputFilepath, bool createMipMaps);

    // Texture Data
    std::vector<TextureData> LoadTextureDataFromDDSThreadSafe(const std::string filepath);
    std::vector<TextureData> LoadTextureDataFromDDSThreadUnsafe(const std::string filepath);
    TextureData LoadUncompressedTextureData(const std::string& filepath);
    TextureData LoadEXRData(const std::string& filepath);

    // Util
    void SaveBitmap(const char* filename, unsigned char* data, int width, int height, int numChannels);

    // Debug
    std::string CMPErrorToString(int error);
    std::string CMPFormatToString(int format);
    int GetChannelCountFromCMPFormat(int format);
}