#pragma once
#include "HellTypes.h"
#include <complex>

namespace Ocean {
    void Init();
    void SetWindDir(glm::vec2 windDir);
    void SetWindSpeed(float windSpeed);
    void SetGravity(float gravity);
    void SetAmplitude(float amplitude);
    void SetCrossWindDampingCoefficient(float crossWindDampingCoefficient);
    void SetSmallWavesDampingCoefficient(float smallWavesDampingCoefficient);


    const std::vector<std::complex<float>>& GetH0(int bandIndex);

    void ComputeInverseFFT2D(unsigned int inputHandle, unsigned int outputHandle);

    const float GetDisplacementScale();
    const float GetHeightScale();

    const float GetGravity();
    const float GetMeshSubdivisionFactor();
    const float GetModelMatrixScale();
    const float GetOceanOriginY();
    const glm::uvec2 GetFFTGridSize();
    const glm::uvec2 GetMeshSize();
    const glm::vec2 GetPatchSimSize(int bandIndex);
    const glm::uvec2 GetTesslationMeshSize();
    const glm::uvec2 GetFFTResolution(int bandIndex);
};