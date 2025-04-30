#pragma once
#include "HellTypes.h"
#include <complex>

namespace Ocean {
    void SetWindDir(glm::vec2 windDir);
    void SetWindSpeed(float windSpeed);
    void SetGravity(float gravity);
    void SetAmplitude(float amplitude);
    void SetCrossWindDampingCoefficient(float crossWindDampingCoefficient);
    void SetSmallWavesDampingCoefficient(float smallWavesDampingCoefficient);

    glm::vec2 KVector(int x, int z);
    float PhillipsSpectrum(const glm::vec2& k);

    std::vector<std::complex<float>> ComputeH0(size_t fftGridSize, uint32_t seed);

    void ComputeInverseFFT2D(unsigned int inputHandle, unsigned int outputHandle);

    const float GetDisplacementScale();
    const float GetHeightScale();

    const float GetGravity();
    const float GetMeshSubdivisionFactor();
    const float GetModelMatrixScale();
    const float GetOceanOriginY();
    const glm::uvec2 GetFFTGridSize();
    const glm::uvec2 GetMeshSize();
    const glm::vec2 GetOceanLength();
    const glm::uvec2 GetTesslationMeshSize();

};