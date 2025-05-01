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
        glm::vec2 patchSimSize;     // Physical size of one patch in simulation units (meters)
        glm::vec2 cellSize;         // World space size of one grid cell = patchSize / fftResolution
        glm::vec2 uvScale;          // UV scale to sample this bands texture maps        
        uint32_t randomSeed;        // Seed for the random phases when building H0
        std::vector<std::complex<float>> h0;
    };

    FFTBand g_fftBands[2];

    const float g_baseFftResolution = 512.0f;

    glm::uvec2 g_fftResolution = glm::uvec2(g_baseFftResolution, g_baseFftResolution);

    const float g_cellSize = 0.35f;
    //glm::vec2 g_patchSimSize = glm::vec2(g_baseFftResolution * g_cellSize, g_baseFftResolution * g_cellSize);
    
    const float g_oceanMeshToGridRatio = 8.0f;       // Ratio of original ocean mesh size to the FFT grid size; used to scale the model matrix
    const float g_meshSubdivisionFactor = 32.0f;     // Number of mesh subdivisions per FFT grid cell; controls mesh density 
    const float g_modelMatrixScale = g_oceanMeshToGridRatio / g_fftResolution.x;
    const float g_oceanOriginY = -0.65;

    glm::vec2 g_patchSize = glm::vec2(g_fftResolution.x * g_modelMatrixScale, g_fftResolution.y * g_modelMatrixScale);


    glm::vec2 g_mWindDir = glm::normalize(glm::vec2(1.0f, 0.0f));
    float g_windSpeed = 75.0f;
    float g_gravity = 9.8f;
    float g_amplitude = 0.00001f;
    float g_crossWindDampingCoefficient = 1.0f;         // Controls the presence of waves perpendicular to the wind direction
    float g_smallWavesDampingCoefficient = 0.0000001f;  // controls the presence of waves of small wave longitude

    float g_dispScale = 1.0f;     // Controls the choppiness of the ocean waves
    float g_heightScale = 0.5f;   // Controls the height of the ocean waves

    FFTSolver g_FFTSolver;

    //std::vector<std::complex<float>> g_h0;

    float PhillipsSpectrum(const glm::vec2& k);
    glm::vec2 KVector(int x, int z, glm::uvec2 fftResolution, glm::vec2 patchSimSize);
    std::vector<std::complex<float>> ComputeH0(glm::uvec2 fftResolution, glm::vec2 patchSize, uint32_t seed);

    void Init() {
        float cellScale = g_cellSize;
        float gridSize = g_baseFftResolution;

        g_fftBands[0].fftResolution = glm::uvec2(gridSize, gridSize);
        g_fftBands[0].patchSimSize = glm::vec2(g_fftBands[0].fftResolution) * cellScale;
        g_fftBands[0].cellSize = g_fftBands[0].patchSimSize / glm::vec2(g_fftBands[0].fftResolution);
        g_fftBands[0].uvScale = glm::vec2(1.0f) / g_fftBands[0].patchSimSize;
        g_fftBands[0].h0 = ComputeH0(g_fftBands[0].fftResolution, g_fftBands[0].patchSimSize, 1337);

        g_fftBands[1].fftResolution = glm::uvec2(gridSize / 2, gridSize / 2);
        g_fftBands[1].patchSimSize = glm::vec2(g_fftBands[1].fftResolution) * cellScale;
        g_fftBands[1].cellSize = g_fftBands[1].patchSimSize / glm::vec2(g_fftBands[1].fftResolution);
        g_fftBands[1].uvScale = glm::vec2(1.0f) / g_fftBands[1].patchSimSize;
        g_fftBands[1].h0 = ComputeH0(g_fftBands[1].fftResolution, g_fftBands[1].patchSimSize, 1338);

        //uint32_t seed = 1337;
        //g_h0 = Ocean::ComputeH0(g_fftBands[0].fftResolution, g_fftBands[0].patchSize, seed);
    }

    void ComputeInverseFFT2D(unsigned int inputHandle, unsigned int outputHandle) {
        g_FFTSolver.fftInv2D(inputHandle, outputHandle, g_fftResolution.x, g_fftResolution.y);
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

    glm::vec2 KVector(int x, int z, glm::uvec2 fftResolution, glm::vec2 patchSimSize) {
        return glm::vec2((x - fftResolution.x / 2.0f) * (2.0f * HELL_PI / patchSimSize.x), (z - fftResolution.y / 2.0f) * (2.0f * HELL_PI / patchSimSize.y));
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


    std::vector<std::complex<float>> ComputeH0(glm::uvec2 fftResolution, glm::vec2 patchSize, uint32_t seed) {
        std::vector<std::complex<float>> h0(fftResolution.x * fftResolution.y);
        std::mt19937 randomGen(seed);
        std::normal_distribution<float> normalDist(0.0f, 1.0f);

        for (unsigned int z = 0; z < fftResolution.y; ++z) {
            for (unsigned int x = 0; x < fftResolution.x; ++x) {
                int idx = z * fftResolution.x + x;
                glm::vec2 k = KVector(x, z, fftResolution, patchSize);

                if (k == glm::vec2(0.0f)) {
                    h0[idx] = { 0.0f, 0.0f };
                }
                else {
                    float amp = sqrt(PhillipsSpectrum(k)) * HELL_SQRT_OF_HALF;
                    float a = normalDist(randomGen) * amp;
                    float b = normalDist(randomGen) * amp;
                    h0[idx] = { a, b };

                    // Enforce Hermitian symmetry for real, periodic heights
                    int ix = (fftResolution.x - x) % fftResolution.x;
                    int iz = (fftResolution.y - z) % fftResolution.y;
                    h0[iz * fftResolution.x + ix] = std::conj(h0[idx]);
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
        return g_fftResolution;
    }

    const float GetModelMatrixScale() {
        return g_modelMatrixScale;
    }

    const glm::uvec2 GetMeshSize() {
        return g_fftResolution + glm::uvec2(1, 1);
    }

    const glm::vec2 GetPatchSimSize(int bandIndex) {
        return g_fftBands[0].patchSimSize;
    }

    const glm::uvec2 GetTesslationMeshSize() {
        return Ocean::GetFFTGridSize() / glm::uvec2(g_meshSubdivisionFactor) + glm::uvec2(1);
    }

    const std::vector<std::complex<float>>& GetH0(int bandIndex) {
        return g_fftBands[bandIndex].h0;
    }

    const glm::uvec2 GetFFTResolution(int bandIndex) {
        return g_fftBands[bandIndex].fftResolution;
    }
}