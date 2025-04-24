#pragma once
#include <glad/glad.h>
#include <cstdint>

struct OpenGLSSBO {
public:

    OpenGLSSBO() = default;

    OpenGLSSBO(size_t size, GLbitfield flags) {
        m_flags = flags;
        PreAllocate(size, m_flags);
    }

    uint32_t GetHandle() const {
        return m_handle;
    }

    void PreAllocate(size_t size, GLbitfield flags) {
        m_flags = flags;
        CleanUp();
        glCreateBuffers(1, &m_handle);
        glNamedBufferStorage(m_handle, (GLsizeiptr)size, nullptr, m_flags);
        m_bufferSize = size;
    }

    void Update(size_t size, const void* data) {
        if (size == 0 || data == nullptr) {
            return;
        }
        if (m_handle == 0 || m_bufferSize < size) {
            // Destroy old buffer and allocate a new one
            CleanUp();
            glCreateBuffers(1, &m_handle);
            glNamedBufferStorage(m_handle, (GLsizeiptr)size, nullptr, m_flags);
            m_bufferSize = size;
        }
        // Upload data
        glNamedBufferSubData(m_handle, 0, (GLsizeiptr)size, data);
    }

    void Bind(GLint index) const {
        glBindBufferBase(GL_SHADER_STORAGE_BUFFER, index, m_handle);
    }

    void copyFrom(const void* hostPtr, size_t sizeInBytes) {
        if (!hostPtr || sizeInBytes == 0 || m_handle == 0) {
            return;
        }

        glBindBuffer(GL_SHADER_STORAGE_BUFFER, m_handle);
        void* devPtrRaw = glMapBufferRange(GL_SHADER_STORAGE_BUFFER, 0, sizeInBytes, GL_MAP_WRITE_BIT | GL_MAP_INVALIDATE_BUFFER_BIT);

        if (!devPtrRaw) {
            glBindBuffer(GL_SHADER_STORAGE_BUFFER, 0);
            throw std::runtime_error("glMapBufferRange failed in copyFrom.");
            return;
        }

        std::memcpy(devPtrRaw, hostPtr, sizeInBytes);
        glUnmapBuffer(GL_SHADER_STORAGE_BUFFER);
        glBindBuffer(GL_SHADER_STORAGE_BUFFER, 0);
    }

    void CopyTo(void* hostPtr, size_t sizeInBytes) const {
        if (!hostPtr || sizeInBytes == 0 || m_handle == 0) {
            return;
        }

        glBindBuffer(GL_SHADER_STORAGE_BUFFER, m_handle);
        const void* devPtrRaw = glMapBufferRange(GL_SHADER_STORAGE_BUFFER, 0, sizeInBytes, GL_MAP_READ_BIT);

        if (!devPtrRaw) {
            glBindBuffer(GL_SHADER_STORAGE_BUFFER, 0);
            std::cout << "OpenGLSSBO::CopyTo failed because devPtrRaw was nullptr\n";
            return;
        }

        std::memcpy(hostPtr, devPtrRaw, sizeInBytes);
        glUnmapBuffer(GL_SHADER_STORAGE_BUFFER);
        glBindBuffer(GL_SHADER_STORAGE_BUFFER, 0);
    }

    void CleanUp() {
        if (m_handle != 0) {
            glDeleteBuffers(1, &m_handle);
            m_handle = 0;
            m_bufferSize = 0;
        }
    }

private:
    GLbitfield m_flags = 0;
    uint32_t m_handle = 0;
    size_t m_bufferSize = 0;
};
