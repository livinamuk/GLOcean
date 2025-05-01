#pragma once
#include "HellTypes.h"
#include <complex>

struct FFTBand {
    glm::uvec2 fftResolution;   // Grid resolution for this band (number of FFT cells per side)
    glm::vec2 patchSimSize;     // Physical size of one patch in simulation units (meters)
    glm::vec2 windDir;
    float amplitude;
    std::vector<std::complex<float>> h0;
};

namespace Ocean {
    void Init();
    void SetWindDir(glm::vec2 windDir);
    void SetWindSpeed(float windSpeed);
    void SetGravity(float gravity);
    void SetAmplitude(float amplitude);
    void SetCrossWindDampingCoefficient(float crossWindDampingCoefficient);
    void SetSmallWavesDampingCoefficient(float smallWavesDampingCoefficient);


    const std::vector<std::complex<float>>& GetH0(int bandIndex);

    void ComputeInverseFFT2D(unsigned int fftResolution, unsigned int inputHandle, unsigned int outputHandle);

    const float GetDisplacementScale();
    const float GetHeightScale();

    const float GetGravity();
    const float GetMeshSubdivisionFactor();
    const float GetModelMatrixScale();
    const float GetOceanOriginY();
    const glm::uvec2 GetBaseFFTResolution();
    const glm::vec2 GetPatchSimSize(int bandIndex);
    const glm::uvec2 GetTesslationMeshSize();
    const glm::uvec2 GetFFTResolution(int bandIndex);

    FFTBand& GetFFTBandByIndex(int bandIndex);
};