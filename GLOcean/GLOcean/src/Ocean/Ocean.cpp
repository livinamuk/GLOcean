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

    const float g_spectrumScaleFactor = 0.45f;
    const float g_meshSubdivisionFactor = 16.0f;         // This is the ratio of the mesh to the original FFT grid. 8.0 means g_FFTGridSize / 8
    //glm::uvec2 g_oceanSize = glm::uvec2(128, 128);
    glm::uvec2 g_fftGridSize = glm::uvec2(256, 256);
    glm::vec2 g_oceanLength = glm::vec2(g_fftGridSize.x * g_spectrumScaleFactor, g_fftGridSize.y * g_spectrumScaleFactor);
    glm::vec2 g_mWindDir = glm::normalize(glm::vec2(1.0f, 0.0f));
    float g_windSpeed = 75.0f;
    float g_gravity = 9.8f;
    float g_amplitude = 0.00001f;
    float g_crossWindDampingCoefficient = 1.0f;         // Controls the presence of waves perpendicular to the wind direction
    float g_smallWavesDampingCoefficient = 0.0000001f;  // controls the presence of waves of small wave longitude
    float g_displacementFactor = -1.0f;                 // Controls the choppiness of the ocean waves
    FFTSolver g_FFTSolver;

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

    void SetDisplacementFactor(float displacementFactor) {
        g_displacementFactor = displacementFactor;
    }

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

    std::vector<std::complex<float>> ComputeH0() {
        uint32_t seed = 1337;
        std::vector<std::complex<float>> h0(g_fftGridSize.x * g_fftGridSize.y);
        std::mt19937 randomGen(seed);
        std::normal_distribution<float> normalDist(0.0f, 1.0f);

        for (unsigned int z = 0; z < g_fftGridSize.y; ++z) {
            for (unsigned int x = 0; x < g_fftGridSize.x; ++x) {
                int idx = z * g_fftGridSize.x + x;
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
                    int ix = (g_fftGridSize.x - x) % g_fftGridSize.x;
                    int iz = (g_fftGridSize.y - z) % g_fftGridSize.y;
                    h0[iz * g_fftGridSize.x + ix] = std::conj(h0[idx]);
                }
            }
        }
        
        return h0;
    }

    const float GetDisplacementFactor() {
        return g_displacementFactor;
    }

    const float GetMeshSubdivisionFactor() {
        return g_meshSubdivisionFactor;
    }

    const float GetGravity() {
        return g_gravity;
    }

    const glm::uvec2 GetFFTGridSize() {
        return g_fftGridSize;
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