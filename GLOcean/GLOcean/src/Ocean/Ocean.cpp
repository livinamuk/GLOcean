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

    FFTBand g_fftBands[2];

    const unsigned int g_baseFftResolution = 512;
    const float g_cellSize = 0.3f;

    const float g_oceanMeshToGridRatio = 8.0f;       // Ratio of original ocean mesh size to the FFT grid size; used to scale the model matrix
    const float g_meshSubdivisionFactor = 32.0f;     // Number of mesh subdivisions per FFT grid cell; controls mesh density 
    const float g_modelMatrixScale = g_oceanMeshToGridRatio / g_baseFftResolution; // was g_fftResolution.x;
    const float g_oceanOriginY = -0.65;

    //glm::vec2 g_mWindDir = glm::normalize(glm::vec2(1.0f, 0.0f));
    float g_windSpeed = 75.0f;
    float g_gravity = 9.8f;

    float g_dispScale = 1.0f;    // Controls the choppiness of the ocean waves
    float g_heightScale = 0.5f;   // Controls the height of the ocean waves

    FFTSolver g_FFTSolver;

    float PhillipsSpectrum(const glm::vec2& k, FFTBand& fftBand);
    glm::vec2 KVector(int x, int z, glm::uvec2 fftResolution, glm::vec2 patchSimSize);
    std::vector<std::complex<float>> ComputeH0(FFTBand& fftBand, uint32_t randomSeed);

    std::string FFTBandToString(int bandIndex) {
        std::string result = "FFT Band " + std::to_string(bandIndex) + "\n";
        result += "- resolution: " + std::to_string(g_fftBands[bandIndex].fftResolution.x) + "\n";
        result += "- patchSimSize: " + std::to_string(g_fftBands[bandIndex].patchSimSize.x) + "\n";
        result += "- amplitude: " + std::to_string(g_fftBands[bandIndex].amplitude) + "\n";
        result += "- windDir: " + std::to_string(g_fftBands[bandIndex].windDir.x) + ", " + std::to_string(g_fftBands[bandIndex].windDir.y) + "\n";
        result += "- crossWindDamping: " + std::to_string(g_fftBands[bandIndex].crossWindDampingCoefficient) + "\n";
        result += "- smallWavesDamping: " + std::to_string(g_fftBands[bandIndex].smallWavesDampingCoefficient) + "\n";
        return result;
    }

    void Init() {
        float cellScale = g_cellSize;
        float gridSize = g_baseFftResolution;

        g_fftBands[0].fftResolution = glm::uvec2(512);
        g_fftBands[0].patchSimSize = glm::vec2(150);
        g_fftBands[0].amplitude = 0.00001f;
        g_fftBands[0].windDir = glm::normalize(glm::vec2(1.0f, 0.1f));
        g_fftBands[0].h0 = ComputeH0(g_fftBands[0], 1337);

        g_fftBands[1].fftResolution = glm::uvec2(512);
        g_fftBands[1].patchSimSize = glm::vec2(110); // 220 looks good too for more waves
        g_fftBands[1].amplitude = 0.00001f;
        g_fftBands[1].windDir = glm::normalize(glm::vec2(0.9f, -0.4f));
        g_fftBands[1].h0 = ComputeH0(g_fftBands[1], 42);
    }

    void ReComputeH0() {
        g_fftBands[0].h0 = ComputeH0(g_fftBands[0], 1337);
        g_fftBands[1].h0 = ComputeH0(g_fftBands[1], 42);
    }

    void ComputeInverseFFT2D(unsigned int fftResolution, unsigned int inputHandle, unsigned int outputHandle) {
        g_FFTSolver.fftInv2D(inputHandle, outputHandle, fftResolution, fftResolution);
    }

    //void SetWindDir(glm::vec2 windDir) {
    // //   if (glm::length(windDir) == 0.0f) {
    // //       std::cout << "Ocean::SetWindDir() failed because wind direction vector has zero length\n";
    // //   }
    // //   g_mWindDir = glm::normalize(windDir);
    //}
    //
    //void SetWindSpeed(float windSpeed) {
    //    g_windSpeed = windSpeed;
    //}
    //
    //void SetCrossWindDampingCoefficient(float crossWindDampingCoefficient) {
    //    g_crossWindDampingCoefficient = crossWindDampingCoefficient;
    //}
    //
    //void SetSmallWavesDampingCoefficient(float smallWavesDampingCoefficient) {
    //    g_smallWavesDampingCoefficient = smallWavesDampingCoefficient;
    //}

    glm::vec2 KVector(int x, int z, glm::uvec2 fftResolution, glm::vec2 patchSimSize) {
        return glm::vec2((x - fftResolution.x / 2.0f) * (2.0f * HELL_PI / patchSimSize.x), (z - fftResolution.y / 2.0f) * (2.0f * HELL_PI / patchSimSize.y));
    }

    float PhillipsSpectrum(const glm::vec2& k, FFTBand& fftBand) {
        const float lengthK = glm::length(k);
        const float lengthKSquared = lengthK * lengthK;
        const float dotKWind = glm::dot(k / lengthK, fftBand.windDir);
        const float L = g_windSpeed * g_windSpeed / g_gravity;

        float phillips = fftBand.amplitude * expf(-1.0f / (lengthKSquared * L * L)) * dotKWind * dotKWind / (lengthKSquared * lengthKSquared);

        if (dotKWind < 0.0f) {
            phillips *= fftBand.crossWindDampingCoefficient;
        }

        return phillips * expf(-lengthKSquared * L * L * fftBand.smallWavesDampingCoefficient);
    }


    std::vector<std::complex<float>> ComputeH0(FFTBand& fftBand, uint32_t seed) {
        std::vector<std::complex<float>> h0(fftBand.fftResolution.x * fftBand.fftResolution.y);
        std::mt19937 randomGen(seed);
        std::normal_distribution<float> normalDist(0.0f, 1.0f);

        for (unsigned int z = 0; z < fftBand.fftResolution.y; ++z) {
            for (unsigned int x = 0; x < fftBand.fftResolution.x; ++x) {
                int idx = z * fftBand.fftResolution.x + x;
                glm::vec2 k = KVector(x, z, fftBand.fftResolution, fftBand.patchSimSize);

                if (k == glm::vec2(0.0f)) {
                    h0[idx] = { 0.0f, 0.0f };
                }
                else {
                    float amp = sqrt(PhillipsSpectrum(k, fftBand)) * HELL_SQRT_OF_HALF;
                    float a = normalDist(randomGen) * amp;
                    float b = normalDist(randomGen) * amp;
                    h0[idx] = { a, b };

                    // Enforce Hermitian symmetry for real, periodic heights
                    int ix = (fftBand.fftResolution.x - x) % fftBand.fftResolution.x;
                    int iz = (fftBand.fftResolution.y - z) % fftBand.fftResolution.y;
                    h0[iz * fftBand.fftResolution.x + ix] = std::conj(h0[idx]);
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

    const glm::uvec2 GetBaseFFTResolution() {
        return glm::uvec2(g_baseFftResolution);
    }

    const float GetModelMatrixScale() {
        return g_modelMatrixScale;
    }

    const glm::vec2 GetPatchSimSize(int bandIndex) {
        return g_fftBands[0].patchSimSize;
    }

    const glm::uvec2 GetTesslationMeshSize() {
        return Ocean::GetBaseFFTResolution() / glm::uvec2(g_meshSubdivisionFactor) + glm::uvec2(1);
    }

    const std::vector<std::complex<float>>& GetH0(int bandIndex) {
        return g_fftBands[bandIndex].h0;
    }

    const glm::uvec2 GetFFTResolution(int bandIndex) {
        return g_fftBands[bandIndex].fftResolution;
    }

    FFTBand& GetFFTBandByIndex(int bandIndex) {
        return g_fftBands[bandIndex];
    }
}