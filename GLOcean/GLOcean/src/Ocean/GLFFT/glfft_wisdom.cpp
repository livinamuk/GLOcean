/* Copyright (C) 2015 Hans-Kristian Arntzen <maister@archlinux.us>
 *
 * Permission is hereby granted, free of charge,
 * to any person obtaining a copy of this software and associated documentation files (the "Software"),
 * to deal in the Software without restriction, including without limitation the rights to
 * use, copy, modify, merge, publish, distribute, sublicense, and/or sell copies of the Software,
 * and to permit persons to whom the Software is furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR IMPLIED,
 * INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.
 * IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY,
 * WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
 */

#include "glfft_wisdom.hpp"
#include "glfft_interface.hpp"
#include "glfft.hpp"
#include <utility>
#include <algorithm>
#include <stdexcept>

#ifdef GLFFT_SERIALIZATION
#include "rapidjson/include/rapidjson/reader.h"
#include "rapidjson/include/rapidjson/prettywriter.h"
#include "rapidjson/include/rapidjson/stringbuffer.h"
#include "rapidjson/include/rapidjson/document.h"
using namespace rapidjson;
#endif

#ifdef GLFFT_CLI_ASYNC
#include "glfft_cli.hpp"
#endif

using namespace std;
using namespace GLFFT;

FFTStaticWisdom FFTWisdom::get_static_wisdom_from_renderer(Context *context)
{
    FFTStaticWisdom res;

    const char *renderer = context->get_renderer_string();
    unsigned threads = context->get_max_work_group_threads();

    if (strstr(renderer, "GeForce") || strstr(renderer, "Quadro"))
    {
        context->log("Detected GeForce/Quadro GPU.\n");
        res.min_workgroup_size = 32; // Warp threads.
        res.min_workgroup_size_shared = 32;
        res.max_workgroup_size = min(threads, 256u); // Very unlikely that more than 256 threads will do anything good.
        res.min_vector_size = 2;
        res.max_vector_size = 2;
        res.shared_banked = FFTStaticWisdom::True;
    }
    else if (strstr(renderer, "Radeon"))
    {
        context->log("Detected Radeon GPU.\n");
        res.min_workgroup_size = 64; // Wavefront threads (GCN).
        res.min_workgroup_size_shared = 128;
        res.max_workgroup_size = min(threads, 256u); // Very unlikely that more than 256 threads will do anything good.
        // TODO: Find if we can restrict this to 2 or 4 always.
        res.min_vector_size = 2;
        res.max_vector_size = 4;
        res.shared_banked = FFTStaticWisdom::True;
    }
    else if (strstr(renderer, "Mali"))
    {
        context->log("Detected Mali GPU.\n");

        res.min_workgroup_size = 4;
        res.min_workgroup_size_shared = 4;
        res.max_workgroup_size = 64; // Going beyond 64 threads per WG is not a good idea.
        res.min_vector_size = 4;
        res.max_vector_size = 4;
        res.shared_banked = FFTStaticWisdom::False;
    }
    // TODO: Add more GPUs.

    return res;
}

pair<double, FFTOptions::Performance> FFTWisdom::learn_optimal_options(
        Context *context, unsigned Nx, unsigned Ny, unsigned radix,
        Mode mode, Target input_target, Target output_target,
        const FFTOptions::Type &type)
{
    WisdomPass pass = {
        {
            Nx, Ny, radix, mode, input_target, output_target,
            type,
        },
        0.0,
    };

    auto itr = library.find(pass);
    if (itr != end(library))
    {
        return make_pair(itr->first.cost, itr->second);
    }
    else
    {
        auto result = study(context, pass, type);
        pass.cost = result.first;
        library[pass] = result.second;

        return result;
    }
}

void FFTWisdom::learn_optimal_options_exhaustive(Context *context,
        unsigned Nx, unsigned Ny,
        Type type, Target input_target, Target output_target, const FFTOptions::Type &fft_type)
{
    bool learn_resolve = type == ComplexToReal || type == RealToComplex;
    Mode vertical_mode = type == ComplexToComplexDual ? VerticalDual : Vertical;
    Mode horizontal_mode = type == ComplexToComplexDual ? HorizontalDual : Horizontal;

    // Create wisdom for horizontal transforms and vertical transform.
    static const unsigned radices[] = { 4, 8, 16, 64 };
    for (auto radix : radices)
    {
        try
        {
            // If we're doing SSBO -> Image or Image -> SSBO. Create wisdom for the two variants.

            // Learn plain transforms.
            if (Ny > 1)
            {
                learn_optimal_options(context, Nx >> static_cast<int>(learn_resolve), Ny, radix, vertical_mode, SSBO, SSBO, fft_type);
            }
            learn_optimal_options(context, Nx >> static_cast<int>(learn_resolve), Ny, radix, horizontal_mode, SSBO, SSBO, fft_type);

            // Learn the first/last pass transforms. Can be fairly significant since accessing textures makes more sense with
            // block interleave and larger WG_Y sizes.
            if (input_target != SSBO)
            {
                if (Ny > 1)
                {
                    learn_optimal_options(context, Nx >> static_cast<int>(learn_resolve), Ny, radix, vertical_mode, input_target, SSBO, fft_type);
                }
                learn_optimal_options(context, Nx >> static_cast<int>(learn_resolve), Ny, radix, horizontal_mode, input_target, SSBO, fft_type);
            }

            if (output_target != SSBO)
            {
                if (Ny > 1)
                {
                    learn_optimal_options(context, Nx >> static_cast<int>(learn_resolve), Ny, radix, vertical_mode, SSBO, output_target, fft_type);
                }
                learn_optimal_options(context, Nx >> static_cast<int>(learn_resolve), Ny, radix, horizontal_mode, SSBO, output_target, fft_type);
            }
        }
#ifdef GLFFT_CLI_ASYNC
        catch (const AsyncCancellation &)
        {
            throw;
        }
#endif
        catch (...)
        {
            // If our default options cannot successfully create the radix pass (i.e. throws),
            // just ignore it for purpose of creating wisdom.
        }
    }

    auto resolve_type = fft_type;
    resolve_type.input_fp16 = resolve_type.output_fp16;
    Mode resolve_mode = type == ComplexToReal ? ResolveComplexToReal : ResolveRealToComplex;
    Target resolve_input_target = SSBO;

    // If we have C2R Nx1 transform, the first pass is resolve, so use those types.
    if (type == ComplexToReal && Ny == 1)
    {
        resolve_type = fft_type;
        resolve_input_target = input_target;
    }

    // If we need to do a resolve pass, train this case as well.
    if (learn_resolve)
    {
        try
        {
            // If Ny == 1 and we're doing RealToComplex, this will be the last pass, so use output_target as target.
            if (Ny == 1 && resolve_mode == ResolveRealToComplex)
            {
                learn_optimal_options(context, Nx >> static_cast<int>(learn_resolve), Ny, 2, resolve_mode, resolve_input_target, output_target, resolve_type);
            }
            else
            {
                learn_optimal_options(context, Nx >> static_cast<int>(learn_resolve), Ny, 2, resolve_mode, resolve_input_target, SSBO, resolve_type);
            }
        }
#ifdef GLFFT_CLI_ASYNC
        catch (const AsyncCancellation &)
        {
            throw;
        }
#endif
        catch (...)
        {
            // If our default options cannot successfully create the radix pass (i.e. throws),
            // just ignore it for purpose of creating wisdom.
        }
    }
}

double FFTWisdom::bench(Context *context, Resource *output, Resource *input,
        const WisdomPass &pass, const FFTOptions &options, const shared_ptr<ProgramCache> &cache) const
{
    FFT fft(context, pass.pass.Nx, pass.pass.Ny, pass.pass.radix, pass.pass.input_target != SSBO ? 1 : pass.pass.radix,
            pass.pass.mode, pass.pass.input_target, pass.pass.output_target,
            cache, options);
    return fft.bench(context,
            output, input, params.warmup, params.iterations, params.dispatches, params.timeout);
}

static inline unsigned mode_to_size(Mode mode)
{
    switch (mode)
    {
        case VerticalDual:
        case HorizontalDual:
        case ResolveRealToComplex:
        case ResolveComplexToReal:
            return 4;

        default:
            return 2;
    }
}

std::pair<double, FFTOptions::Performance> FFTWisdom::study(Context *context, const WisdomPass &pass, FFTOptions::Type type) const
{
    auto cache = make_shared<ProgramCache>();

    unique_ptr<Resource> output;
    unique_ptr<Resource> input;

    unsigned mode_size = mode_to_size(pass.pass.mode);
    vector<float> tmp(mode_size * pass.pass.Nx * pass.pass.Ny);

    if (pass.pass.input_target == SSBO)
    {
        input = context->create_buffer(tmp.data(), tmp.size() * sizeof(float) >> static_cast<int>(type.input_fp16), AccessStaticCopy);
    }
    else
    {
        Format format = FormatUnknown;
        unsigned Nx = pass.pass.Nx;
        unsigned Ny = pass.pass.Ny;

        switch (pass.pass.mode)
        {
            case VerticalDual:
            case HorizontalDual:
                format = FormatR32G32B32A32Float;
                break;

            case Vertical:
            case Horizontal:
                format = FormatR32G32Float;
                break;

            case ResolveComplexToReal:
                format = FormatR32G32Float;
                Nx *= 2;
                break;

            default:
                throw logic_error("Invalid input mode.\n");
        }

        input = context->create_texture(tmp.data(), Nx, Ny, format);
    }

    if (pass.pass.output_target == SSBO)
    {
        output = context->create_buffer(nullptr, tmp.size() * sizeof(float) >> static_cast<int>(type.output_fp16), AccessStreamCopy);
    }
    else
    {
        Format format = FormatUnknown;
        unsigned Nx = pass.pass.Nx;
        unsigned Ny = pass.pass.Ny;

        switch (pass.pass.mode)
        {
            case VerticalDual:
            case HorizontalDual:
                format = FormatR32G32B32A32Float;
                break;

            case Vertical:
            case Horizontal:
                format = FormatR32G32Float;
                break;

            case ResolveRealToComplex:
                format = FormatR32G32Float;
                Nx *= 2;
                break;

            default:
                throw logic_error("Invalid output mode.\n");
        }

        output = context->create_texture(nullptr, Nx, Ny, format);
    }

    // Exhaustive search, look for every sensible combination, and find fastest parameters.
    // Get initial best cost with defaults.
    FFTOptions::Performance best_perf;
    double minimum_cost = bench(context, output.get(), input.get(), pass, { best_perf, type }, cache);

    static const FFTStaticWisdom::Tristate shared_banked_values[] = { FFTStaticWisdom::False, FFTStaticWisdom::True };
    static const unsigned vector_size_values[] = { 2, 4, 8 };
    static const unsigned workgroup_size_x_values[] = { 4, 8, 16, 32, 64, 128, 256 };
    static const unsigned workgroup_size_y_values[] = { 1, 2, 4, 8, };

    bool test_resolve = pass.pass.mode == ResolveComplexToReal || pass.pass.mode == ResolveRealToComplex;
    bool test_dual = pass.pass.mode == VerticalDual || pass.pass.mode == HorizontalDual;
    unsigned bench_count = 0;

    for (auto shared_banked : shared_banked_values)
    {
        // Useless test, since shared banked is only relevant for radix 16/64.
        if (pass.pass.radix < 16 && shared_banked)
        {
            continue;
        }

        bool fair_shared_banked = (pass.pass.radix < 16) ||
                                  (static_wisdom.shared_banked == FFTStaticWisdom::DontCare) ||
                                  (shared_banked == static_wisdom.shared_banked);

        if (!fair_shared_banked)
        {
            continue;
        }

        for (auto vector_size : vector_size_values)
        {
            // Resolve passes currently only support vector size 2. Shared banked makes no sense either.
            if (test_resolve && (vector_size != 2 || shared_banked))
            {
                continue;
            }

            // We can only use vector_size 8 with FP16.
            if (vector_size == 8 && (!type.fp16 || !type.input_fp16 || !type.output_fp16))
            {
                continue;
            }

            // Makes little sense to test since since vector_size will be bumped to 4 anyways.
            if (test_dual && vector_size < 4)
            {
                continue;
            }

            for (auto workgroup_size_x : workgroup_size_x_values)
            {
                for (auto workgroup_size_y : workgroup_size_y_values)
                {
                    unsigned workgroup_size  = workgroup_size_x * workgroup_size_y;

                    unsigned min_workgroup_size = pass.pass.radix >= 16 ? static_wisdom.min_workgroup_size_shared :
                                                                          static_wisdom.min_workgroup_size;

                    unsigned min_vector_size = test_dual ? max(4u, static_wisdom.min_vector_size) : static_wisdom.min_vector_size;
                    unsigned max_vector_size = test_dual ? max(4u, static_wisdom.max_vector_size) : static_wisdom.max_vector_size;

                    bool fair_workgroup_size = workgroup_size <= static_wisdom.max_workgroup_size &&
                                               workgroup_size >= min_workgroup_size;
                    if (pass.pass.Ny == 1 && workgroup_size_y > 1)
                    {
                        fair_workgroup_size = false;
                    }

                    if (!fair_workgroup_size)
                    {
                        continue;
                    }

                    // If we have dual mode, accept vector sizes larger than max.
                    bool fair_vector_size = test_resolve || (vector_size <= max_vector_size &&
                                                             vector_size >= min_vector_size);

                    if (!fair_vector_size)
                    {
                        continue;
                    }

                    FFTOptions::Performance perf;
                    perf.shared_banked = (shared_banked != 0);
                    perf.vector_size = vector_size;
                    perf.workgroup_size_x = workgroup_size_x;
                    perf.workgroup_size_y = workgroup_size_y;

                   
                    try
                    {
                        // If workgroup sizes are too big for our test, this will throw.
                        double cost = bench(context, output.get(), input.get(), pass, { perf, type }, cache);
                        bench_count++;

#if 1
                        context->log("\nWisdom run (mode = %u, radix = %u):\n", pass.pass.mode, pass.pass.radix);
                        context->log("  Width:            %4u\n", pass.pass.Nx);
                        context->log("  Height:           %4u\n", pass.pass.Ny);
                        context->log("  Shared banked:     %3s\n", shared_banked ? "yes" : "no");
                        context->log("  Vector size:         %u\n", vector_size);
                        context->log("  Workgroup size: (%u, %u)\n", workgroup_size_x, workgroup_size_y);
                        context->log("  Cost:         %8.3g\n", cost);
#endif

                        if (cost < minimum_cost)
                        {
#if 1
                            context->log("  New optimal solution! (%g -> %g)\n", minimum_cost, cost);
#endif
                            best_perf = perf;
                            minimum_cost = cost;
                        }
                    }
#ifdef GLFFT_CLI_ASYNC
                    catch (const AsyncCancellation &)
                    {
                        throw;
                    }
#endif
                    catch (...)
                    {
                        // If we pass in bogus parameters,
                        // FFT will throw and we just ignore this.
                    }
                }
            }
        }
    }

    context->log("Tested %u variants!\n", bench_count);
    return make_pair(minimum_cost, best_perf);
}

const pair<const WisdomPass, FFTOptions::Performance>* FFTWisdom::find_optimal_options(unsigned Nx, unsigned Ny, unsigned radix,
        Mode mode, Target input_target, Target output_target, const FFTOptions::Type &type) const
{
    WisdomPass pass = {
        {
            Nx, Ny, radix, mode, input_target, output_target,
            type,
        },
        0.0,
    };

    auto itr = library.find(pass);
    return itr != end(library) ? (&(*itr)) : nullptr;
}



const FFTOptions::Performance& FFTWisdom::find_optimal_options_or_default(unsigned Nx, unsigned Ny, unsigned radix,
        Mode mode, Target input_target, Target output_target, const FFTOptions &base_options) const
{
    WisdomPass pass = {
        {
            Nx, Ny, radix, mode, input_target, output_target,
            base_options.type,
        },
        0.0,
    };

    auto itr = library.find(pass);

#if 0
    if (itr == end(library))
    {
        context->log("Didn't find options for (%u x %u, radix %u, mode %u, input_target %u, output_target %u)\n",
                Nx, Ny, radix, unsigned(mode), unsigned(input_target), unsigned(output_target));
    }
#endif

    return itr != end(library) ? itr->second : base_options.performance;
}


/*
bool compareWisdomPassKeysForLog(const GLFFT::WisdomPass& a, const GLFFT::WisdomPass& b) {
    if (a.pass.radix != b.pass.radix) return a.pass.radix < b.pass.radix;
    if (a.pass.mode != b.pass.mode) return static_cast<int>(a.pass.mode) < static_cast<int>(b.pass.mode);
    if (a.pass.Nx != b.pass.Nx) return a.pass.Nx < b.pass.Nx;
    if (a.pass.Ny != b.pass.Ny) return a.pass.Ny < b.pass.Ny;
    if (a.pass.input_target != b.pass.input_target) return static_cast<int>(a.pass.input_target) < static_cast<int>(b.pass.input_target);
    if (a.pass.output_target != b.pass.output_target) return static_cast<int>(a.pass.output_target) < static_cast<int>(b.pass.output_target);
    // Add type comparisons if needed for sorting order
    if (a.pass.type.fp16 != b.pass.type.fp16) return a.pass.type.fp16 < b.pass.type.fp16;
    if (a.pass.type.input_fp16 != b.pass.type.input_fp16) return a.pass.type.input_fp16 < b.pass.type.input_fp16;
    if (a.pass.type.output_fp16 != b.pass.type.output_fp16) return a.pass.type.output_fp16 < b.pass.type.output_fp16;
    if (a.pass.type.normalize != b.pass.type.normalize) return a.pass.type.normalize < b.pass.type.normalize;
    return false; // Equal according to this sort
}

#include <iostream>


// --- Replace the existing function in FFTWisdom.cpp with this: ---
const FFTOptions::Performance& FFTWisdom::find_optimal_options_or_default(unsigned Nx, unsigned Ny, unsigned radix,
                                                                          Mode mode, Target input_target, Target output_target, const FFTOptions& base_options) const
{
    // Construct the key for the lookup based on requested parameters
    WisdomPass pass = {}; // Use zero-initialization
    pass.pass.Nx = Nx;
    pass.pass.Ny = Ny;
    pass.pass.radix = radix;
    pass.pass.mode = mode;
    pass.pass.input_target = input_target;
    pass.pass.output_target = output_target;
    pass.pass.type = base_options.type; // *** CRITICAL: Type comes from base_options ***

    // --- START DEBUG LOGGING ---
    std::cout << "[Wisdom Lookup] Querying Wisdom Object Address: " << this << std::endl;
    std::cout << "[Wisdom Lookup] Requesting Config For:"
        << " Nx=" << Nx << " Ny=" << Ny
        << " Radix=" << radix << " Mode=" << static_cast<int>(mode)
        << " InTarget=" << static_cast<int>(input_target)
        << " OutTarget=" << static_cast<int>(output_target)
        // Log the type info from the 'pass' key we constructed
        << " Type(fp16=" << pass.pass.type.fp16
        << " InFp16=" << pass.pass.type.input_fp16
        << " OutFp16=" << pass.pass.type.output_fp16
        << " Norm=" << pass.pass.type.normalize << ")"
        << std::endl;

// Print current map contents for comparison
    std::cout << "[Wisdom Lookup] Current map state (" << library.size() << " entries):" << std::endl;
    // Sort keys for consistent log output (optional but helpful)
    std::vector<WisdomPass> sorted_keys;
    sorted_keys.reserve(library.size()); // Reserve space
    for (const auto& pair : library) {
        sorted_keys.push_back(pair.first);
    }
    std::sort(sorted_keys.begin(), sorted_keys.end(), compareWisdomPassKeysForLog);

    for (const auto& key : sorted_keys) {
        std::cout << "  Stored Key: "
            << " Nx=" << key.pass.Nx << " Ny=" << key.pass.Ny
            << " Radix=" << key.pass.radix << " Mode=" << static_cast<int>(key.pass.mode)
            << " InT=" << static_cast<int>(key.pass.input_target)
            << " OutT=" << static_cast<int>(key.pass.output_target)
            << " Type(fp16=" << key.pass.type.fp16
            << " InFp16=" << key.pass.type.input_fp16
            << " OutFp16=" << key.pass.type.output_fp16
            << " Norm=" << key.pass.type.normalize << ")"
            << std::endl;
  // Direct comparison test using your operator==
        if (pass == key) {
            std::cout << "    --> DIRECT COMPARISON (pass == key) SUCCEEDED!" << std::endl;
        }
    }
    std::cout << "[Wisdom Lookup] --- End map state ---" << std::endl;

    // Perform the actual lookup
    auto itr = library.find(pass);

    // Report result
    if (itr != end(library)) {
        const FFTOptions::Performance& found_perf = itr->second;
        std::cout << "[Wisdom Lookup]   FOUND entry via map.find! Using saved perf: "
            << "Banked=" << found_perf.shared_banked
            << ", Vec=" << found_perf.vector_size
            << ", WG=(" << found_perf.workgroup_size_x << "," << found_perf.workgroup_size_y << ")"
            << std::endl;
        return found_perf;
    }
    else {
        const FFTOptions::Performance& default_perf = base_options.performance;
        std::cout << "[Wisdom Lookup]   NOT FOUND entry via map.find! Returning DEFAULT perf: "
            << "Banked=" << default_perf.shared_banked
            << ", Vec=" << default_perf.vector_size
            << ", WG=(" << default_perf.workgroup_size_x << "," << default_perf.workgroup_size_y << ")"
            << std::endl;
        return default_perf;
    }
    // --- END DEBUG LOGGING ---
}*/

#ifdef GLFFT_SERIALIZATION
std::string FFTWisdom::archive() const
{
    StringBuffer s;
    PrettyWriter<StringBuffer> writer{s};

    writer.StartObject();
    writer.String("library");

    // Serialize all wisdom accumulated to a string.
    writer.StartArray();
    for (auto &entry : library)
    {
        writer.StartObject();

        writer.String("scenario");
        writer.StartObject();
        writer.String("nx");
        writer.Uint(entry.first.pass.Nx);
        writer.String("ny");
        writer.Uint(entry.first.pass.Ny);
        writer.String("radix");
        writer.Uint(entry.first.pass.radix);
        writer.String("mode");
        writer.Uint(entry.first.pass.mode);
        writer.String("input_target");
        writer.Uint(entry.first.pass.input_target);
        writer.String("output_target");
        writer.Uint(entry.first.pass.output_target);
        writer.EndObject();

        writer.String("type");
        writer.StartObject();
        writer.String("fp16");
        writer.Bool(entry.first.pass.type.fp16);
        writer.String("input_fp16");
        writer.Bool(entry.first.pass.type.input_fp16);
        writer.String("output_fp16");
        writer.Bool(entry.first.pass.type.output_fp16);
        writer.String("normalize");
        writer.Bool(entry.first.pass.type.normalize);
        writer.EndObject();

        writer.String("performance");
        writer.StartObject();
        writer.String("shared_banked");
        writer.Bool(entry.second.shared_banked);
        writer.String("vector_size");
        writer.Uint(entry.second.vector_size);
        writer.String("workgroup_size_x");
        writer.Uint(entry.second.workgroup_size_x);
        writer.String("workgroup_size_y");
        writer.Uint(entry.second.workgroup_size_y);
        writer.EndObject();

        writer.String("cost");
        writer.Double(entry.first.cost);

        writer.EndObject();
    }
    writer.EndArray();
    writer.EndObject();
    return s.GetString();
}

void FFTWisdom::extract(const char *json)
{
    Document document;
    document.Parse(json);

    // Exception safe, we don't want to risk throwing in the middle of the
    // loop, leaving the library is broken state.
    unordered_map<WisdomPass, FFTOptions::Performance> new_library;

    auto &lib = document["library"];

    // y u no begin(), end() :(
    for (Value::ConstValueIterator itr = lib.Begin(); itr != lib.End(); ++itr)
    {
        auto &v = *itr;

        WisdomPass pass;
        FFTOptions::Performance perf;

        pass.cost = v["cost"].GetDouble();

        auto &scenario = v["scenario"];
        pass.pass.Nx = scenario["nx"].GetUint();
        pass.pass.Ny = scenario["ny"].GetUint();
        pass.pass.radix = scenario["radix"].GetUint();
        pass.pass.mode = static_cast<Mode>(scenario["mode"].GetUint());
        pass.pass.input_target = static_cast<Target>(scenario["input_target"].GetUint());
        pass.pass.output_target = static_cast<Target>(scenario["output_target"].GetUint());

        auto &type = v["type"];
        pass.pass.type.fp16 = type["fp16"].GetBool();
        pass.pass.type.input_fp16 = type["input_fp16"].GetBool();
        pass.pass.type.output_fp16 = type["output_fp16"].GetBool();
        pass.pass.type.normalize = type["normalize"].GetBool();

        auto &performance = v["performance"];
        perf.shared_banked = performance["shared_banked"].GetBool();
        perf.vector_size = performance["vector_size"].GetUint();
        perf.workgroup_size_x = performance["workgroup_size_x"].GetUint();
        perf.workgroup_size_y = performance["workgroup_size_y"].GetUint();

        new_library[pass] = perf;
    }

    // Exception safe.
    swap(library, new_library);
}
#endif

