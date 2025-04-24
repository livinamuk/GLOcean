#include <iostream>
#include "GL_texture.h"
#include "../../../Util.hpp"
#include "DDS/DDS_Helpers.h"
#include <stb_image.h>
#include "../Tools/ImageTools.h"
#include "../GL_util.hpp"
#include "tinyexr.h"

#define ALLOW_BINDLESS_TEXTURES 0

GLuint64 OpenGLTexture::GetBindlessID() {
    return m_bindlessID;
}

TextureData LoadEXRData(std::string filepath) {
    TextureData textureData;
    const char* err = nullptr;
    const char** layer_names = nullptr;
    int num_layers = 0;
    bool status = EXRLayers(filepath.c_str(), &layer_names, &num_layers, &err);
    free(layer_names);
    const char* layername = NULL;
    float* floatPtr = nullptr;
    status = LoadEXRWithLayer(&floatPtr, &textureData.m_width, &textureData.m_height, filepath.c_str(), layername, &err);
    textureData.m_data = floatPtr;
    return textureData;
}

bool OpenGLTexture::Load(const FileInfo& fileInfo, ImageDataType imageDataType) {
   //if (!Util::FileExists(fileInfo.path)) {
   //    std::cout << fileInfo.path << " does not exist.\n";
   //    return false;
   //}
   //if (imageDataType == ImageDataType::UNCOMPRESSED) {
   //    TextureData textureData = ImageTools::LoadTextureData(fileInfo.path, imageDataType);
   //    m_data = textureData.m_data;
   //    m_width = textureData.m_width;
   //    m_height = textureData.m_height;
   //    m_channelCount = textureData.m_channelCount;
   //    m_dataSize = textureData.m_dataSize;
   //    m_format = textureData.m_format;
   //    m_internalFormat = textureData.m_internalFormat;
   //    m_imageDataType = imageDataType;
   //}
   //if (imageDataType == ImageDataType::COMPRESSED) {
   //    TextureData textureData = ImageTools::LoadTextureData(fileInfo.path, imageDataType);
   //    m_data = textureData.m_data;
   //    m_width = textureData.m_width;
   //    m_height = textureData.m_height;
   //    m_channelCount = textureData.m_channelCount;
   //    m_dataSize = textureData.m_dataSize;
   //    m_format = textureData.m_format;
   //    m_internalFormat = textureData.m_internalFormat;
   //    m_imageDataType = imageDataType;
   //}
   //if (imageDataType == ImageDataType::EXR) {
   //    int datasize = 0;
   //    TextureData textureData = LoadEXRData(fileInfo.path);
   //    m_data = textureData.m_data;
   //    m_width = textureData.m_width;
   //    m_height = textureData.m_height;
   //    m_format = GL_RGB;
   //    m_internalFormat = GL_RGB16;
   //    m_channelCount = 3;
   //    m_dataSize = 1;
   //    m_imageDataType = imageDataType;
   //}

    return true;
}

//void OpenGLTexture::AllocateEmptyMipmaps() {
//    int mipWidth = m_width;
//    int mipHeight = m_height;
//    int level = 0; 
//    glBindTexture(GL_TEXTURE_2D, m_handle);
//
//    while (mipWidth > 1 || mipHeight > 1) {
//        if (m_imageDataType == ImageDataType::UNCOMPRESSED) {
//            glTexImage2D(GL_TEXTURE_2D, level, m_internalFormat, mipWidth, mipHeight, 0, m_format, GL_UNSIGNED_BYTE, nullptr);
//        }
//        if (m_imageDataType == ImageDataType::COMPRESSED) {
//            size_t mipDataSize = OpenGLUtil::CalculateCompressedDataSize(m_internalFormat, mipWidth, mipHeight);
//            glCompressedTexImage2D(GL_TEXTURE_2D, level, m_internalFormat, mipWidth, mipHeight, 0, mipDataSize, nullptr);
//        }
//        mipWidth = std::max(1, mipWidth / 2);
//        mipHeight = std::max(1, mipHeight / 2);
//        level++;
//    }
//    if (m_imageDataType == ImageDataType::UNCOMPRESSED) {
//        glTexImage2D(GL_TEXTURE_2D, level, m_internalFormat, 1, 1, 0, m_format, GL_UNSIGNED_BYTE, nullptr);
//    }
//    if (m_imageDataType == ImageDataType::COMPRESSED) {
//        size_t mipDataSize = OpenGLUtil::CalculateCompressedDataSize(m_internalFormat, mipWidth, mipHeight);
//        glCompressedTexImage2D(GL_TEXTURE_2D, level, m_internalFormat, 1, 1, 0, mipDataSize, nullptr);
//    }
//    glBindTexture(GL_TEXTURE_2D, 0);
//    m_mipmapCount = level + 1;
//}

void OpenGLTexture::AllocateMemory(int width, int height, int format, int internalFormat, int mipmapLevelCount) {
    if (m_handle == 0) {
        glGenTextures(1, &m_handle);
    }
    if (m_memoryAllocated) {
        return;
    }
    m_memoryAllocated = true;
    m_width = width;
    m_height = height;
    m_mipmapLevelCount = mipmapLevelCount;
    glBindTexture(GL_TEXTURE_2D, m_handle);
    int mipmapWidth = width;
    int mipmapHeight = height;
    for (int i = 0; i < mipmapLevelCount; i++) {

        std::cout << " preallocated miplevel " << m_handle << " " << i << " " << mipmapWidth << "x" << mipmapHeight << " " << OpenGLUtil::GetGLFormatAsString(format) << " " << OpenGLUtil::GetGLInternalFormatAsString(internalFormat) << "\n";

        if (m_imageDataType == ImageDataType::UNCOMPRESSED) {
            glTexImage2D(GL_TEXTURE_2D, i, internalFormat, mipmapWidth, mipmapHeight, 0, format, GL_UNSIGNED_BYTE, nullptr);
        }
        if (m_imageDataType == ImageDataType::COMPRESSED) {
            glCompressedTexImage2D(GL_TEXTURE_2D, i, internalFormat, mipmapWidth, mipmapHeight, 0, 0, nullptr);
        }
        if (m_imageDataType == ImageDataType::EXR) {
            glTexImage2D(GL_TEXTURE_2D, i, GL_RGB16, mipmapWidth, mipmapHeight, 0, GL_RGBA, GL_FLOAT, nullptr);
        }
        // Calculate the dimensions for the next mipmap level
        mipmapWidth = std::max(1, mipmapWidth / 2);
        mipmapHeight = std::max(1, mipmapHeight / 2);
    }
    glBindTexture(GL_TEXTURE_2D, 0);
}

GLuint& OpenGLTexture::GetHandle() {
    return m_handle;
}

void OpenGLTexture::Bind(unsigned int slot) {
    glActiveTexture(GL_TEXTURE0 + slot);
    glBindTexture(GL_TEXTURE_2D, m_handle);
}

int OpenGLTexture::GetWidth() {
    return m_width;
}

int OpenGLTexture::GetHeight() {
    return m_height;
}

int OpenGLTexture::GetChannelCount() {
    return m_channelCount;
}

int OpenGLTexture::GetDataSize() {
    return m_dataSize;
}

void* OpenGLTexture::GetData() {
    return m_data;
}

GLint OpenGLTexture::GetFormat() {
    return m_format;
}
GLint OpenGLTexture::GetInternalFormat() {
    return m_internalFormat;
}

GLint OpenGLTexture::GetMipmapLevelCount() {
    return m_mipmapLevelCount;
}

void OpenGLTexture::SetWrapMode(TextureWrapMode wrapMode) {
    glBindTexture(GL_TEXTURE_2D, m_handle);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, OpenGLUtil::TextureWrapModeToGLEnum(wrapMode));
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, OpenGLUtil::TextureWrapModeToGLEnum(wrapMode));
    glBindTexture(GL_TEXTURE_2D, 0);
}

void OpenGLTexture::SetMinFilter(TextureFilter filter) {
    glBindTexture(GL_TEXTURE_2D, m_handle);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, OpenGLUtil::TextureFilterToGLEnum(filter));
    glBindTexture(GL_TEXTURE_2D, 0);
}

void OpenGLTexture::SetMagFilter(TextureFilter filter) {
    glBindTexture(GL_TEXTURE_2D, m_handle);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, OpenGLUtil::TextureFilterToGLEnum(filter));
    glBindTexture(GL_TEXTURE_2D, 0);
}

void OpenGLTexture::MakeBindlessTextureResident() {
#if ALLOW_BINDLESS_TEXTURES
    if (m_bindlessID == 0) {
        m_bindlessID = glGetTextureHandleARB(m_handle);
    }
    glMakeTextureHandleResidentARB(m_bindlessID);
#endif
}

void OpenGLTexture::MakeBindlessTextureNonResident() {
#if ALLOW_BINDLESS_TEXTURES
    if (m_bindlessID == 0) {
        glMakeTextureHandleNonResidentARB(m_bindlessID);
    }
#endif
}