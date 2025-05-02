#pragma once
#include "HellTypes.h"
#include <complex>

struct FFTBand {
    glm::uvec2 fftResolution = {};   // Grid resolution for this band (number of FFT cells per side)
    glm::vec2 patchSimSize = {};     // Physical size of one patch in simulation units (meters)
    glm::vec2 windDir = {};
    float amplitude = 0;
    float crossWindDampingCoefficient = 1.0f;         // Controls the presence of waves perpendicular to the wind direction
    float smallWavesDampingCoefficient = 0.0000001f;  // controls the presence of waves of small wave longitude
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

    std::string FFTBandToString(int bandIndex);

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

    void ReComputeH0();
    FFTBand& GetFFTBandByIndex(int bandIndex);
};