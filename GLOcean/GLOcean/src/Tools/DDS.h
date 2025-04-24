#pragma once
#include <cstdint>

#define FOURCC_DXT1 0x31545844 // "DXT1"
#define FOURCC_DXT3 0x33545844 // "DXT3"
#define FOURCC_DXT5 0x35545844 // "DXT5"
#define FOURCC_DX10 0x30315844 // "DX10"

struct DDSHeader {
    uint32_t dwMagic;
    uint32_t dwSize;
    uint32_t dwFlags;
    uint32_t dwHeight;
    uint32_t dwWidth;
    uint32_t dwPitchOrLinearSize;
    uint32_t dwDepth;
    uint32_t dwMipMapCount;
    uint32_t dwReserved1[11];
    uint32_t ddspf_dwSize;
    uint32_t ddspf_dwFlags;
    uint32_t ddspf_dwFourCC;
    uint32_t ddspf_dwRGBBitCount;
    uint32_t ddspf_dwRBitMask;
    uint32_t ddspf_dwGBitMask;
    uint32_t ddspf_dwBBitMask;
    uint32_t ddspf_dwABitMask;
    uint32_t dwCaps;
    uint32_t dwCaps2;
    uint32_t dwCaps3;
    uint32_t dwCaps4;
    uint32_t dwReserved2;
};

struct DDSHeaderDX10 {
    uint32_t dxgiFormat;
    uint32_t resourceDimension;
    uint32_t miscFlag;
    uint32_t arraySize;
    uint32_t reserved;
};

struct DDSFormatInfo {
    int internalFormat; // GPU internal storage format
    int format;         // Data format (if applicable, e.g., GL_RGBA for uncompressed)
    int blockSize;      // Block size for compressed formats
    int channelCount;   // Number of channels (1 for grayscale, 3 for RGB, 4 for RGBA)
};

inline DDSFormatInfo GetDDSFormatInfo(const DDSHeader& header, DDSHeaderDX10* dx10Header) {
    DDSFormatInfo formatInfo = {};
    if (header.ddspf_dwFourCC == FOURCC_DXT1) {
        formatInfo.internalFormat = GL_COMPRESSED_RGBA_S3TC_DXT1_EXT;
        formatInfo.format = GL_RGBA; // DXT1 represents RGB data with optional alpha
        formatInfo.blockSize = 8;
        formatInfo.channelCount = (header.ddspf_dwABitMask) ? 4 : 3; // DXT1 may omit alpha
    }
    else if (header.ddspf_dwFourCC == FOURCC_DXT3 || header.ddspf_dwFourCC == FOURCC_DXT5) {
        formatInfo.internalFormat = (header.ddspf_dwFourCC == FOURCC_DXT3)
            ? GL_COMPRESSED_RGBA_S3TC_DXT3_EXT
            : GL_COMPRESSED_RGBA_S3TC_DXT5_EXT;
        formatInfo.format = GL_RGBA; // DXT3/5 represent RGBA data
        formatInfo.blockSize = 16;
        formatInfo.channelCount = 4; // DXT3/5 store RGBA
    }
    else if (header.ddspf_dwFourCC == FOURCC_DX10 && dx10Header) {
        // DX10 header is present, check dxgiFormat
        switch (dx10Header->dxgiFormat) {
        case 98: // DXGI_FORMAT_BC7_UNORM
            formatInfo.internalFormat = GL_COMPRESSED_RGBA_BPTC_UNORM_ARB;
            formatInfo.format = GL_RGBA; // BC7 represents RGBA data
            formatInfo.blockSize = 16;
            formatInfo.channelCount = 4; // BC7 is always RGBA
            break;
        case 99: // DXGI_FORMAT_BC7_UNORM_SRGB
            formatInfo.internalFormat = GL_COMPRESSED_SRGB_ALPHA_BPTC_UNORM_ARB;
            formatInfo.format = GL_SRGB_ALPHA; // BC7 in sRGB
            formatInfo.blockSize = 16;
            formatInfo.channelCount = 4; // BC7 is always RGBA
            break;
        default:
            std::cerr << "Unsupported DX10 format: " << dx10Header->dxgiFormat << "\n";
            return {};
        }
    }
    else {
        std::cerr << "Unsupported DDS format: " << header.ddspf_dwFourCC << "\n";
        return {};
    }
    return formatInfo;
}
