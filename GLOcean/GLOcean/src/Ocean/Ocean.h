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
    void SetDisplacementFactor(float displacementFactor);

    glm::vec2 KVector(int x, int z);
    float PhillipsSpectrum(const glm::vec2& k);

    std::vector<std::complex<float>> ComputeH0();

    void ComputeInverseFFT2D(unsigned int inputHandle, unsigned int outputHandle);

    const float GetDisplacementFactor();
    const float GetGravity();
    const float GetMeshSubdivisionFactor();
    const glm::uvec2 GetFFTGridSize();
    const glm::uvec2 GetMeshSize();
    const glm::vec2 GetOceanLength();
    const glm::uvec2 GetTesslationMeshSize();

};