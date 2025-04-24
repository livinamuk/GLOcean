#pragma once
#include "Common.h"
#include <random>
#include <regex>

namespace Util {

    std::string Lowercase(std::string& str);
    std::string Uppercase(std::string& str);
    
    inline std::string Vec3ToString(glm::vec3 v) {
        return std::string("(" + std::format("{:.2f}", v.x) + ", " + std::format("{:.2f}", v.y) + ", " + std::format("{:.2f}", v.z) + ")");
    }
    
    inline glm::vec3 Vec3Min(const glm::vec3& a, const glm::vec3& b) {
        return glm::vec3(std::min(a.x, b.x), std::min(a.y, b.y), std::min(a.z, b.z));
    }

    inline glm::vec3 Vec3Max(const glm::vec3& a, const glm::vec3& b) {
        return glm::vec3(std::max(a.x, b.x), std::max(a.y, b.y), std::max(a.z, b.z));
    }

    inline const std::string GetFilename(const std::string& filepath) {
        std::string result = filepath.substr(filepath.rfind("/") + 1);
        result = result.substr(0, result.length() - 4);
        return result;
    }

    inline FileInfoOld GetFileInfo(std::string filepath) {
        std::string filename = filepath.substr(filepath.rfind("/") + 1);
        filename = filename.substr(0, filename.length() - 4);
        std::string filetype = filepath.substr(filepath.length() - 3);
        std::string directory = filepath.substr(0, filepath.rfind("/") + 1);
        std::string materialType = "NONE";
        if (filename.length() > 5) {
            std::string query = filename.substr(filename.length() - 3);
            if (query == "ALB" || query == "RMA" || query == "NRM")
                materialType = query;
        }
        FileInfoOld info;
        info.fullpath = filepath;
        info.filename = filename;
        info.filetype = filetype;
        info.directory = directory;
        info.materialType = materialType;
        return info;
    }

    inline FileInfoOld GetFileInfo(const std::filesystem::directory_entry& filepath) {
        const auto& path{ filepath.path() };
        static const auto get_material_type{ [](std::string_view filename) {
            if (filename.size() > 5) {
                filename.remove_prefix(filename.size() - 3);
                if (filename == "ALB" || filename == "RMA" || filename == "NRM") {
                    return std::string{ filename };
                }
            }
            return std::string{ "NONE" };
        } };
        const auto stem{ path.has_stem() ? path.stem().string() : "" };
        std::string filetype = path.has_extension() ? path.extension().string().substr(1) : "";  // remove dot
        return FileInfoOld{
            path.string(),
            path.parent_path().string(),
            stem,
            Lowercase(filetype),
            get_material_type(stem)
        };
    }


    inline std::string Lowercase(std::string& str) {
        std::string result = "";
        for (auto& c : str) {
            result += std::tolower(c);
        }
        return result;
    }

    inline std::string Uppercase(std::string& str) {
        std::string result = "";
        for (auto& c : str) {
            result += std::toupper(c);
        }
        return result;
    }

    inline bool FileExists(const std::string_view name) {
        struct stat buffer;
        return (stat(name.data(), &buffer) == 0);
    }

    inline float FInterpTo(float current, float target, float deltaTime, float interpSpeed) {
        // If no interp speed, jump to target value
        if (interpSpeed <= 0.f)
            return target;
        // Distance to reach
        const float Dist = target - current;
        // If distance is too small, just set the desired location
        if (Dist * Dist < (float)9.99999993922529e-9)
            return target;
        // Delta Move, Clamp so we do not over shoot.
        const float DeltaMove = Dist * glm::clamp(deltaTime * interpSpeed, 0.0f, 1.0f);
        return current + DeltaMove;
    }

    inline std::string GetFileName(const std::string& filepath) {
        // Find the last slash or backslash to remove directory paths
        std::string::size_type pos = filepath.find_last_of("/\\");
        std::string filename = (pos == std::string::npos) ? filepath : filepath.substr(pos + 1);
        // Find the last dot to remove the file extension
        pos = filename.find_last_of('.');
        if (pos != std::string::npos) {
            filename = filename.substr(0, pos);
        }
        return filename;
    }

    inline std::string RemoveFileExtension(const std::string& filename) {
        size_t pos = filename.find_last_of('.');
        if (pos != std::string::npos) {
            return filename.substr(0, pos);
        }
        return filename;
    }

    inline std::string GetFullPath(const std::filesystem::directory_entry& entry) {
        return entry.path().string();
    }

    inline std::string GetFileName(const std::filesystem::directory_entry& entry) {
        return entry.path().filename().string();
    }

    inline std::string GetFileNameWithoutExtension(const std::filesystem::directory_entry& entry) {
        return entry.path().stem().string(); // stem() removes the extension
    }

    inline std::string GetFileExtension(const std::filesystem::directory_entry& entry) {
        return entry.path().extension().string().substr(1);
    }

    inline std::vector<FileInfo> IterateDirectory(const std::string& directory, std::vector<std::string> extensions = std::vector<std::string>()) {
        std::vector<FileInfo> fileInfoList;

        if (!std::filesystem::exists(directory) || !std::filesystem::is_directory(directory)) {
            std::cout << "Util::IterateDirectory() failed: " << directory << " does not exist\n";
            return fileInfoList;
        }

        auto entries = std::filesystem::directory_iterator(directory);
        for (const auto& entry : entries) {
            if (!std::filesystem::is_regular_file(entry)) {
                continue;
            }
            FileInfo fileInfo;
            fileInfo.path = Util::GetFullPath(entry);
            fileInfo.name = Util::GetFileNameWithoutExtension(entry);
            fileInfo.ext = Util::GetFileExtension(entry);
            fileInfo.dir = directory;
            if (extensions.empty()) {
                fileInfoList.push_back(fileInfo);
            }
            else {
                for (std::string& ext : extensions) {
                    if (ext == fileInfo.ext) {
                        fileInfoList.push_back(fileInfo);
                        break;
                    }
                }
            }
        }
        return fileInfoList;
    }

    inline std::vector<FileInfo> IterateDirectorySorted(const std::string& directory, std::vector<std::string> extensions = std::vector<std::string>()) {
        std::vector<FileInfo> fileInfoList;
        auto entries = std::filesystem::directory_iterator(directory);
        for (const auto& entry : entries) {
            if (!std::filesystem::is_regular_file(entry)) {
                continue;
            }
            FileInfo fileInfo;
            fileInfo.path = Util::GetFullPath(entry);
            fileInfo.name = Util::GetFileNameWithoutExtension(entry);
            fileInfo.ext = Util::GetFileExtension(entry);
            fileInfo.dir = directory;
            if (extensions.empty()) {
                fileInfoList.push_back(fileInfo);
            }
            else {
                for (std::string& ext : extensions) {
                    if (ext == fileInfo.ext) {
                        fileInfoList.push_back(fileInfo);
                        break;
                    }
                }
            }
        }

        // Sort the fileInfoList numerically by filename
        std::sort(fileInfoList.begin(), fileInfoList.end(), [](const FileInfo& a, const FileInfo& b) {
            std::regex numberRegex("(\\d+)"); // Regex to extract numbers
            std::smatch matchA, matchB;
            bool foundA = std::regex_search(a.name, matchA, numberRegex);
            bool foundB = std::regex_search(b.name, matchB, numberRegex);
            if (foundA && foundB) {
                int numA = std::stoi(matchA.str());
                int numB = std::stoi(matchB.str());
                if (numA != numB) return numA < numB; // Compare numbers if found
            }
            return a.name < b.name; // Fall back to lexicographic order
        });

        return fileInfoList;
    }

    inline FileInfo GetFileInfoFromPath(const std::string& filepath) {
        FileInfo fileInfo;
        std::filesystem::path path(filepath);
        if (!std::filesystem::exists(path) || !std::filesystem::is_regular_file(path)) {
            throw std::runtime_error("Invalid file path or not a regular file: " + filepath);
        }
        fileInfo.path = path.string();
        fileInfo.name = path.stem().string();
        fileInfo.ext = path.has_extension() ? path.extension().string().substr(1) : "";
        fileInfo.dir = path.parent_path().string();

        return fileInfo;
    }

    inline float RandomFloat(float min, float max) {
        return min + static_cast <float> (rand()) / (static_cast <float> (RAND_MAX / (max - min)));
    }

    inline int RandomInt(int min, int max) {
        static std::random_device dev;
        static std::mt19937 rng(dev());
        std::uniform_int_distribution<std::mt19937::result_type> dist6(min, max);
        return dist6(rng);
    }

    inline std::string Mat4ToString(glm::mat4 m) {
        std::string result;
        result += std::format("{:.2f}", m[0][0]) + ", " + std::format("{:.2f}", m[1][0]) + ", " + std::format("{:.2f}", m[2][0]) + ", " + std::format("{:.2f}", m[3][0]) + "\n";
        result += std::format("{:.2f}", m[0][1]) + ", " + std::format("{:.2f}", m[1][1]) + ", " + std::format("{:.2f}", m[2][1]) + ", " + std::format("{:.2f}", m[3][1]) + "\n";
        result += std::format("{:.2f}", m[0][2]) + ", " + std::format("{:.2f}", m[1][2]) + ", " + std::format("{:.2f}", m[2][2]) + ", " + std::format("{:.2f}", m[3][2]) + "\n";
        result += std::format("{:.2f}", m[0][3]) + ", " + std::format("{:.2f}", m[1][3]) + ", " + std::format("{:.2f}", m[2][3]) + ", " + std::format("{:.2f}", m[3][3]);
        return result;
    }

    inline int CeilSqrt(int number) {
        if (number <= 0) {
            return 0;
        }
        else {
            return static_cast<int>(std::ceil(std::sqrt(number)));
        }
    }
}
