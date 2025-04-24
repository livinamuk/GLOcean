#pragma once

#include <memory>
#include <complex>
#include <cstdint>
#include <unordered_map>
#include <vector>

#include "glfft/glfft_common.hpp"
#include "glfft/glfft.hpp"
#include "glfft/glfft_gl_interface.hpp" 

namespace GLFFT {
    struct FFTPlan {
        std::shared_ptr<GLFFT::FFTWisdom> mFFTWisdom;
        std::shared_ptr<GLFFT::FFT> mFFT;

        // Constructor
        FFTPlan(const std::shared_ptr<GLFFT::FFTWisdom>& wisdom, const std::shared_ptr<GLFFT::FFT>& fft) : mFFTWisdom(wisdom), mFFT(fft) {
        }
    };
}

struct FFTSolver {
    FFTSolver() = default;
    void fft2D(GLuint inputHandle, GLuint outputHandle, int sizeX, int sizeY);
    void fftInv2D(GLuint inputHandle, GLuint outputHandle, int sizeX, int sizeY);

private:
    std::int64_t getKey(int sizeX, int sizeY);
    GLFFT::FFT& getOrCreateFFT(std::shared_ptr<GLFFT::FFTPlan>& fftPlan, int sizeX, int sizeY, GLFFT::Direction direction);
    GLFFT::FFT& getCachedFFT(int sizeX, int sizeY);
    GLFFT::FFT& getCachedInvFFT(int sizeX, int sizeY);

    GLFFT::GLContext m_glContext;
    std::unordered_map<std::int64_t, std::shared_ptr<GLFFT::FFTPlan>> m_forwardCache;
    std::unordered_map<std::int64_t, std::shared_ptr<GLFFT::FFTPlan>> m_inverseCache;
};