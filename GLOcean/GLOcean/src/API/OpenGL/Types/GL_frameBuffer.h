#pragma once
#include <glad/glad.h>
#include <vector>
#include <iostream>
#include <cstring>
#include "HellTypes.h"

struct ColorAttachment {
    const char* name = "undefined";
    GLuint handle = 0;
    GLenum internalFormat = 0;;
    GLenum format = 0;
    GLenum type = 0;
};

struct DepthAttachment {
    GLuint handle = 0;
    GLenum internalFormat = 0;
};

class OpenGLFrameBuffer {
private:
    const char* m_name = "undefined";
    GLuint m_handle = 0;
    GLuint m_width = 0;
    GLuint m_height = 0;
    std::vector<ColorAttachment> m_colorAttachments;
    DepthAttachment m_depthAttachment;

    static bool StrCmp(const char* queryA, const char* queryB);

public:
    OpenGLFrameBuffer() = default;
    OpenGLFrameBuffer(const char* name, int width, int height);
    OpenGLFrameBuffer(const char* name, const glm::ivec2& resolution);
    void Create(const char* name, int width, int height);
    void Create(const char* name, const glm::ivec2& resolution);
    void CleanUp();
    void CreateAttachment(const char* name, GLenum internalFormat, GLenum minFilter = GL_LINEAR, GLenum magFilter = GL_LINEAR, GLenum wrapFilter = GL_CLAMP_TO_EDGE);
    void CreateDepthAttachment(GLenum internalFormat, GLenum minFilter = GL_LINEAR, GLenum magFilter = GL_LINEAR, GLint wrap = GL_CLAMP_TO_EDGE, glm::vec4 borderColor = glm::vec4(1.0f));
    void BindDepthAttachmentFrom(const OpenGLFrameBuffer& srcFrameBuffer);
    void Bind();
    void SetViewport();
    void DrawBuffers(std::vector<const char*> attachmentNames);
    void DrawBuffer(const char* attachmentName);
    void ClearAttachment(const char* attachmentName, GLfloat r, GLfloat g = 0.0f, GLfloat b = 0.0f, GLfloat a = 0.0f);
    void ClearAttachmentI(const char* attachmentName, GLint r, GLint g = 0, GLint b = 0, GLint a = 0);
    void ClearAttachmentUI(const char* attachmentName, GLint r, GLint g = 0, GLint b = 0, GLint a = 0);
    void ClearAttachmenSubRegion(const char* attachmentName, GLint xOffset, GLint yOffset, GLsizei width, GLsizei height, GLfloat r, GLfloat g = 0.0f, GLfloat b = 0.0f, GLfloat a = 0.0f);
    void ClearAttachmenSubRegionInt(const char* attachmentName, GLint xOffset, GLint yOffset, GLsizei width, GLsizei height, GLint r, GLint g = 0.0f, GLint b = 0.0f, GLint a = 0.0f);
    void ClearAttachmenSubRegionUInt(const char* attachmentName, GLint xOffset, GLint yOffset, GLsizei width, GLsizei height, GLuint r, GLuint g = 0.0f, GLuint b = 0.0f, GLuint a = 0.0f);
    void ClearDepthAttachment();
    void Resize(int width, int height);
    GLuint GetHandle() const;
    GLuint GetWidth() const;
    GLuint GetHeight() const;
    GLuint GetColorAttachmentHandleByName(const char* name) const;
    GLuint GetDepthAttachmentHandle() const;
    GLenum GetColorAttachmentSlotByName(const char* name) const;
    void BlitToDefaultFrameBuffer(const char* srcName, GLint dstX0, GLint dstY0, GLint dstX1, GLint dstY1, GLbitfield mask, GLenum filter);
};
