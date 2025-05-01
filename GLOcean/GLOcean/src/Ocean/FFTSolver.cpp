#include "FFTSolver.h"
#include <iostream>
#include <fstream>

namespace {
    const bool g_peformBenchmark = false;
    const bool g_loadBenchmarkFromFile = true;
} 

bool FFTSolver::KeyExsits(int sizeX, int sizeY) {
    int64_t key = GetKey(sizeX, sizeY);
    return (m_fftCache.find(key) != m_fftCache.end());
}

int64_t FFTSolver::GetKey(int sizeX, int sizeY) {
    return (static_cast<int64_t>(sizeX) << 32) | static_cast<uint32_t>(sizeY);
}

void FFTSolver::CreateFTT(int sizeX, int sizeY) {
    int64_t key = GetKey(sizeX, sizeY);

    GLFFT::Type type = GLFFT::ComplexToComplex;
    GLFFT::Direction direction = GLFFT::Direction::Inverse;
    GLFFT::Target input_target = GLFFT::SSBO;
    GLFFT::Target output_target = GLFFT::SSBO;
    std::shared_ptr<GLFFT::ProgramCache> cache = std::make_shared<GLFFT::ProgramCache>();

    GLFFT::FFTOptions options;

    options.type.fp16 = false;
    options.type.input_fp16 = false;  // Use FP32 input.
    options.type.output_fp16 = false; // Use FP32 output.
    options.type.normalize = false;

    options.performance.shared_banked = true;
    options.performance.vector_size = 2;
    options.performance.workgroup_size_x = 32;
    options.performance.workgroup_size_y = 1;


    if (g_peformBenchmark) {
        PerformBenchmark(sizeX, sizeY, options);
    }

    GLFFT::go = true;

    if (g_loadBenchmarkFromFile) {
        LoadBenchmarkFromFile();
        auto tuned = m_wisdom.find_optimal_options_or_default(
            sizeX, sizeY, /*radix=*/64,
            GLFFT::Mode::Horizontal, input_target, GLFFT::SSBO, options
        );

        options.performance.workgroup_size_x = tuned.workgroup_size_x;
        options.performance.workgroup_size_y = tuned.workgroup_size_y;
        options.performance.shared_banked = tuned.shared_banked;
        options.performance.vector_size = tuned.vector_size;
    }

    GLFFT::FFT* new_fft_ptr = new GLFFT::FFT(&m_glContext, sizeX, sizeY, type, direction, input_target, output_target, cache, options, m_wisdom);
    m_fftCache[key] = new_fft_ptr;

    std::cout << "Create FTT for size " << sizeX << ", " << sizeY << "\n";
}

void FFTSolver::fftInv2D(GLuint inputHandle, GLuint outputHandle, int sizeX, int sizeY) {
    if (!KeyExsits(sizeX, sizeY)) {
        CreateFTT(sizeX, sizeY);
    }

    int64_t key = GetKey(sizeX, sizeY);
    GLFFT::FFT* fft = m_fftCache[key];

    // Wrap DeviceMemory IDs in GLFFT::GLBuffer
    GLFFT::GLBuffer inputBuffer(inputHandle);
    GLFFT::GLBuffer outputBuffer(outputHandle);

    // Execute the inverse FFT
    GLFFT::CommandBuffer* command = m_glContext.request_command_buffer();
    if (!command) {
        std::cout << "Failed to request command buffer from GLContext\n";
    }
    fft->process(command, &outputBuffer, &inputBuffer);
    m_glContext.submit_command_buffer(command);
}

void FFTSolver::PerformBenchmark(int sizeX, int sizeY, GLFFT::FFTOptions options) {
    
   // int maxConstantDataSize = 64;
   //
   // auto* cmd = m_glContext.request_command_buffer();
   //
   // const int warmup_iterations = 50000;
   // for (int i = 0; i < warmup_iterations; ++i) {
   //     struct FFTConstantData {};
   //     FFTConstantData constant_data;
   //
   //     cmd->push_constant_data(3, &constant_data, sizeof(constant_data));
   // }

    
    m_wisdom.set_static_wisdom(GLFFT::FFTWisdom::get_static_wisdom_from_renderer(&m_glContext));
    m_wisdom.learn_optimal_options_exhaustive(&m_glContext, sizeX, sizeY, GLFFT::ComplexToComplex, GLFFT::SSBO, GLFFT::SSBO, options.type);

    std::string wisdom_json = m_wisdom.archive();

    std::cout << "==== FFT WISDOM JSON START ====\n" << wisdom_json << "\n==== FFT WISDOM JSON END ====\n";

    // Write File
    std::ofstream temp_wisdom_file("ftt_wisdom.json");
    if (temp_wisdom_file.is_open()) {
        temp_wisdom_file << wisdom_json;
        temp_wisdom_file.close();
        std::cout << ">>> Successfully wrote wisdom to ftt_wisdom.json <<<\n";
    }
    else {
        std::cerr << ">>> ERROR: Could not write ftt_wisdom.json <<<\n";
    }
}

void FFTSolver::LoadBenchmarkFromFile() {
    const char* wisdom_filepath = "ftt_wisdom.json";
    std::ifstream wisdom_file_stream(wisdom_filepath);

    std::cout << "[FFTSolver] Attempting to load wisdom from: " << wisdom_filepath << "\n";

    if (wisdom_file_stream.is_open()) {
        std::string loaded_wisdom_json((std::istreambuf_iterator<char>(wisdom_file_stream)), std::istreambuf_iterator<char>());
        wisdom_file_stream.close();

        if (!loaded_wisdom_json.empty()) {
            try {
                #ifdef GLFFT_SERIALIZATION
                m_wisdom.extract(loaded_wisdom_json.c_str());
                std::cout << "[FFTSolver] Successfully loaded and extracted wisdom.\n";
                #else
                std::cerr << "[FFTSolver] Warning: GLFFT_SERIALIZATION not defined during build. Cannot load wisdom from file.\n";
                #endif
            }
            catch (const std::exception& e) {
                std::cerr << "[FFTSolver] Error parsing wisdom file '" << wisdom_filepath << "': " << e.what() << ". Using default FFT parameters.\n";
            }
        }
        else {
            std::cerr << "[FFTSolver] Warning: Wisdom file '" << wisdom_filepath << "' was empty. Using default FFT parameters.\n";
        }
    }
    else {
        std::cerr << "[FFTSolver] Warning: Could not open wisdom file '" << wisdom_filepath << "'. Using default FFT parameters.\n";
    }
}