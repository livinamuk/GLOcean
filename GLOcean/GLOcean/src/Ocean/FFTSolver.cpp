#include "FFTSolver.h"

#include <cstdint>
#include <memory>
#include <unordered_map>
#include <stdexcept> 
#include "glfft/glfft.hpp"
#include "glfft/glfft_gl_interface.hpp"
#ifdef _WIN32
#include <windows.h>
#endif
#include <GL/gl.h> 

namespace {
    const bool kDoFFTWisdom = false;
} 

void FFTSolver::fft2D(GLuint inputHandle, GLuint outputHandle, int sizeX, int sizeY) {
    // Directly call the internal cache method
    GLFFT::FFT& fft = getCachedFFT(sizeX, sizeY);

    // Wrap DeviceMemory IDs in GLFFT::GLBuffer
    GLFFT::GLBuffer inputBuffer(inputHandle);
    GLFFT::GLBuffer outputBuffer(outputHandle);

    // Execute the FFT
    GLFFT::CommandBuffer* command = m_glContext.request_command_buffer();
    if (!command) {
        throw std::runtime_error("Failed to request command buffer from GLContext");
    }
    fft.process(command, &outputBuffer, &inputBuffer);
    m_glContext.submit_command_buffer(command);

    glMemoryBarrier(GL_SHADER_STORAGE_BARRIER_BIT);
}

void FFTSolver::fftInv2D(GLuint inputHandle, GLuint outputHandle, int sizeX, int sizeY) {
    // Directly call the internal cache method
    GLFFT::FFT& fft = getCachedInvFFT(sizeX, sizeY);

    // Wrap DeviceMemory IDs in GLFFT::GLBuffer
    GLFFT::GLBuffer inputBuffer(inputHandle);
    GLFFT::GLBuffer outputBuffer(outputHandle);

    // Execute the inverse FFT
    GLFFT::CommandBuffer* command = m_glContext.request_command_buffer();
    if (!command) {
       // Handle error: Could not get command buffer
        throw std::runtime_error("Failed to request command buffer from GLContext");
    }
    fft.process(command, &outputBuffer, &inputBuffer);
    m_glContext.submit_command_buffer(command);

    // Ensure GPU operations complete
    glMemoryBarrier(GL_SHADER_STORAGE_BARRIER_BIT);
}


std::int64_t FFTSolver::getKey(int sizeX, int sizeY) {
    return (static_cast<int64_t>(sizeX) << 32) | static_cast<uint32_t>(sizeY);
}

GLFFT::FFT& FFTSolver::getOrCreateFFT(std::shared_ptr<GLFFT::FFTPlan>& fftPlan, int sizeX, int sizeY, GLFFT::Direction direction) {
    if (!fftPlan)  {
        GLFFT::FFTOptions options; 
        options.type.fp16 = false;
        options.type.input_fp16 = false;  // Use FP32 input.
        options.type.output_fp16 = false; // Use FP32 output.
        options.type.normalize = false;   // Normalized FFT.

        // Optional: Use FFTWisdom for performance tuning
        auto fftWisdom = std::make_shared<GLFFT::FFTWisdom>();
        
        // Pass the raw pointer from the unique_ptr
        if (kDoFFTWisdom) {
            fftWisdom->set_static_wisdom(GLFFT::FFTWisdom::get_static_wisdom_from_renderer(&m_glContext));
            fftWisdom->learn_optimal_options_exhaustive(&m_glContext, sizeX, sizeY, GLFFT::ComplexToComplex, GLFFT::SSBO, GLFFT::SSBO, options.type);
        }
        
        // Pass the raw pointer from the unique_ptr
        auto fft = std::make_shared<GLFFT::FFT>(&m_glContext, sizeX, sizeY, GLFFT::ComplexToComplex, direction, GLFFT::SSBO, GLFFT::SSBO, std::make_shared<GLFFT::ProgramCache>(), options);

        // Store the wisdom (even if empty) and the FFT object in a new plan
        fftPlan = std::make_shared<GLFFT::FFTPlan>(fftWisdom, fft);
    }

    if (!fftPlan || !fftPlan->mFFT) {
        throw std::runtime_error("Failed to get or create FFT object");
    }

    return *fftPlan->mFFT;
}

GLFFT::FFT& FFTSolver::getCachedFFT(int sizeX, int sizeY) {
    auto& fftPlan = m_forwardCache[getKey(sizeX, sizeY)];
    return getOrCreateFFT(fftPlan, sizeX, sizeY, GLFFT::Direction::Forward);
}

GLFFT::FFT& FFTSolver::getCachedInvFFT(int sizeX, int sizeY) {
    auto& fftPlan = m_inverseCache[getKey(sizeX, sizeY)];
    return getOrCreateFFT(fftPlan, sizeX, sizeY, GLFFT::Direction::Inverse);
}