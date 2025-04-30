#include "Ocean.h"

#include <cmath>
#include <cassert>
#include <random>
#include <glm/glm.hpp>
#include <glm/gtc/type_ptr.hpp>

#include "../Util.hpp"
#include "HellDefines.h"
#include "FFTSolver.h"

namespace Ocean {

    struct FFTBand {        
        glm::uvec2 fftResolution;   // Grid resolution for this band (number of FFT cells per side)
        glm::vec2  patchSize;       // Physical size of one patch in world units (meters)
        glm::vec2  cellSize;        // World space size of one grid cell = patchSize / fftResolution
        glm::vec2  uvScale;         // UV scale to sample this bands texture maps        
        uint32_t randomSeed;        // Seed for the random phases when building H0
    };

    FFTBand g_fftBands[2];


    const float g_baseGridSize = 256.0f;

    glm::uvec2 g_fftGridSize = glm::uvec2(g_baseGridSize, g_baseGridSize);

    const float g_spectrumScaleFactor = 0.45f;
    glm::vec2 g_oceanLength = glm::vec2(g_baseGridSize * g_spectrumScaleFactor, g_baseGridSize * g_spectrumScaleFactor);
    
    const float g_oceanMeshToGridRatio = 8.0f;      // Ratio of original ocean mesh size to the FFT grid size; used to scale the model matrix
    const float g_meshSubdivisionFactor = 16.0f;     // Number of mesh subdivisions per FFT grid cell; controls mesh density 
    const float g_modelMatrixScale = g_oceanMeshToGridRatio / g_fftGridSize.x;
    const float g_oceanOriginY = -0.65;


    glm::vec2 g_patchSize = glm::vec2(g_fftGridSize.x * g_modelMatrixScale, g_fftGridSize.y * g_modelMatrixScale);


    glm::vec2 g_mWindDir = glm::normalize(glm::vec2(1.0f, 0.0f));
    float g_windSpeed = 75.0f;
    float g_gravity = 9.8f;
    float g_amplitude = 0.00001f;
    float g_crossWindDampingCoefficient = 1.0f;         // Controls the presence of waves perpendicular to the wind direction
    float g_smallWavesDampingCoefficient = 0.0000001f;  // controls the presence of waves of small wave longitude

    float g_dispScale = 1.0f;     // Controls the choppiness of the ocean waves
    float g_heightScale = 0.5f;   // Controls the height of the ocean waves

    FFTSolver g_FFTSolver;

    void Init() {
        float cellScale = g_modelMatrixScale;
        float gridSize = g_baseGridSize;

        g_fftBands[0].fftResolution = glm::uvec2(gridSize, gridSize);
        g_fftBands[0].patchSize = glm::vec2(g_fftBands[0].fftResolution) * cellScale;
        g_fftBands[0].cellSize = g_fftBands[0].patchSize / glm::vec2(g_fftBands[0].fftResolution);
        g_fftBands[0].uvScale = glm::vec2(1.0f) / g_fftBands[0].patchSize;
        g_fftBands[0].randomSeed = 1337;

        g_fftBands[1].fftResolution = glm::uvec2(gridSize / 2, gridSize / 2);
        g_fftBands[1].patchSize = glm::vec2(g_fftBands[1].fftResolution) * cellScale;
        g_fftBands[1].cellSize = g_fftBands[1].patchSize / glm::vec2(g_fftBands[1].fftResolution);
        g_fftBands[1].uvScale = glm::vec2(1.0f) / g_fftBands[1].patchSize;
        g_fftBands[1].randomSeed = 1338;

    }

    void ComputeInverseFFT2D(unsigned int inputHandle, unsigned int outputHandle) {
        g_FFTSolver.fftInv2D(inputHandle, outputHandle, g_fftGridSize.x, g_fftGridSize.y);
    }

    void SetWindDir(glm::vec2 windDir) {
        if (glm::length(windDir) == 0.0f) {
            std::cout << "Ocean::SetWindDir() failed because wind direction vector has zero length\n";
        }
        g_mWindDir = glm::normalize(windDir);
    }

    void SetWindSpeed(float windSpeed) {
        g_windSpeed = windSpeed;
    }

    void SetGravity(float gravity) {
        g_gravity = gravity;
    }

    void SetAmplitude(float amplitude) {
        g_amplitude = amplitude;
    }

    void SetCrossWindDampingCoefficient(float crossWindDampingCoefficient) {
        g_crossWindDampingCoefficient = crossWindDampingCoefficient;
    }

    void SetSmallWavesDampingCoefficient(float smallWavesDampingCoefficient) {
        g_smallWavesDampingCoefficient = smallWavesDampingCoefficient;
    }

    //void SetDisplacementFactor(float displacementFactor) {
    //    g_displacementFactor = displacementFactor;
    //}

    glm::vec2 KVector(int x, int z) {
        return glm::vec2((x - g_fftGridSize.x / 2.0f) * (2.0f * HELL_PI / g_oceanLength.x), (z - g_fftGridSize.y / 2.0f) * (2.0f * HELL_PI / g_oceanLength.y));
    }

    float PhillipsSpectrum(const glm::vec2& k) {
        const float lengthK = glm::length(k);
        const float lengthKSquared = lengthK * lengthK;
        const float dotKWind = glm::dot(k / lengthK, g_mWindDir);
        const float L = g_windSpeed * g_windSpeed / g_gravity;

        float phillips = g_amplitude * expf(-1.0f / (lengthKSquared * L * L)) * dotKWind * dotKWind / (lengthKSquared * lengthKSquared);

        if (dotKWind < 0.0f) {
            phillips *= g_crossWindDampingCoefficient;
        }

        return phillips * expf(-lengthKSquared * L * L * g_smallWavesDampingCoefficient);
    }

    std::vector<std::complex<float>> ComputeH0(size_t fftGridSize, uint32_t seed) {
        std::vector<std::complex<float>> h0(fftGridSize * fftGridSize);
        std::mt19937 randomGen(seed);
        std::normal_distribution<float> normalDist(0.0f, 1.0f);

        for (unsigned int z = 0; z < fftGridSize; ++z) {
            for (unsigned int x = 0; x < fftGridSize; ++x) {
                int idx = z * fftGridSize + x;
                glm::vec2 k = KVector(x, z);

                if (k == glm::vec2(0.0f)) {
                    h0[idx] = { 0.0f, 0.0f };
                }
                else {
                    float amp = sqrt(PhillipsSpectrum(k)) * HELL_SQRT_OF_HALF;
                    float a = normalDist(randomGen) * amp;
                    float b = normalDist(randomGen) * amp;
                    h0[idx] = { a, b };

                    // Enforce Hermitian symmetry for real, periodic heights
                    int ix = (fftGridSize - x) % fftGridSize;
                    int iz = (fftGridSize - z) % fftGridSize;
                    h0[iz * fftGridSize + ix] = std::conj(h0[idx]);
                }
            }
        }
        
        return h0;
    }

    const float GetDisplacementScale() {
        return g_dispScale;
    }
    
    const float GetHeightScale() {
        return g_heightScale;
    }

    const float GetMeshSubdivisionFactor() {
        return g_meshSubdivisionFactor;
    }

    const float GetGravity() {
        return g_gravity;
    }

    const float GetOceanOriginY() {
        return g_oceanOriginY;
    }

    const glm::uvec2 GetFFTGridSize() {
        return g_fftGridSize;
    }

    const float GetModelMatrixScale() {
        return g_modelMatrixScale;
    }

    const glm::uvec2 GetMeshSize() {
        return g_fftGridSize + glm::uvec2(1, 1);
    }

    const glm::vec2 GetOceanLength() {
        return g_oceanLength;
    }

    const glm::uvec2 GetTesslationMeshSize() {
        return Ocean::GetFFTGridSize() / glm::uvec2(g_meshSubdivisionFactor) + glm::uvec2(1);
    }
}