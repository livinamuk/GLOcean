#pragma once

#include <memory>
#include <complex>
#include <cstdint>
#include <unordered_map>
#include <vector>

#include "glfft/glfft_common.hpp"
#include "glfft/glfft.hpp"
#include "glfft/glfft_gl_interface.hpp" 

struct FFTSolver {
    FFTSolver() = default;
    void fftInv2D(GLuint inputHandle, GLuint outputHandle, int sizeX, int sizeY);

private:
    int64_t GetKey(int sizeX, int sizeY);
    bool KeyExsits(int sizeX, int sizeY);
    void CreateFTT(int sizeX, int sizeY);
    void PerformBenchmark(int sizeX, int sizeY, GLFFT::FFTOptions options);
    void LoadBenchmarkFromFile();

    GLFFT::FFTWisdom m_wisdom;
    GLFFT::GLContext m_glContext;
    std::unordered_map<std::int64_t, GLFFT::FFT*> m_fftCache;
};