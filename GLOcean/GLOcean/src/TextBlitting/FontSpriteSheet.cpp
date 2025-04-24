#include "FontSpriteSheet.h"
#include <fstream>
#include <sstream>
#include <vector>
#include <iostream>
#include <filesystem>
#include <algorithm>
#include <regex>
#pragma warning(push)
#pragma warning(disable : 4996)
#include "stb_image.h"
#include "stb_image_write.h"
#pragma warning(pop)

namespace FontSpriteSheetPacker {

    struct ImageData {
        int m_width;
        int m_height;
        int m_channelCount;
        void* m_data;
    };

    ImageData LoadImageData(const std::string& filepath);
    std::string EscapeString(const std::string& str);
    std::string UnescapeString(const std::string& str);
    int FindInt(const std::string& jsonChunk, const std::string& key);
    std::string FindString(const std::string& json, const std::string& key);
    std::vector<std::string> FindArray(const std::string& json, const std::string& key);
    std::vector<std::string> GetSortedFilePaths(const std::string& directory);

    void ExampleUsage() {
        std::string name = "StandardFont";
        std::string characters = R"(!"#$%&'*+,-./0123456789:;<=>?_ABCDEFGHIJKLMNOPQRSTUVWXYZ\^_`abcdefghijklmnopqrstuvwxyz )";
        std::string textureSourcePath = "res/fonts/raw_images/standard_font/";
        std::string outputPath = "res/fonts/";
        Export(name, characters, textureSourcePath, outputPath);
        FontSpriteSheet fontSpriteSheet = Import("res/fonts/StandardFont.json");
    }

    void Export(const std::string& name, const std::string& characters, const std::string& textureSourcePath, const std::string& outputPath) {
        // Configure filepaths
        std::filesystem::path outputDir = outputPath;
        std::filesystem::path outputImagePath = outputDir / (name + ".png");
        std::filesystem::path outputJSONPath = outputDir / (name + ".json"); 

        // Load the image data
        std::vector<std::string> filePaths = GetSortedFilePaths(textureSourcePath);
        std::vector<ImageData> imageDataList;
        for (std::string& filePath : filePaths) {
            imageDataList.push_back(LoadImageData(filePath));
        }
        // Dirty error check
        for (ImageData& imageData : imageDataList) {
            if (!imageData.m_data) {
                std::cout << "Failed to load font sprite sheet image data\n";
            }
        }
        // Calculate total area and max character height
        int totalArea = 0;
        int maxCharHeight = 0;
        for (const ImageData& imageData : imageDataList) {
            totalArea += imageData.m_width * imageData.m_height;
            maxCharHeight = std::max(maxCharHeight, imageData.m_height);
        }
        // Determine the minimum texture width for roughly a square (the ceiling of the square root)
        int textureWidth = static_cast<int>(std::ceil(std::sqrt(totalArea)));

        // Calculate the first character's data
        int charCount = imageDataList.size();
        std::vector<FontSpriteSheet::CharData> charDataList(charCount);
        charDataList[0].width = imageDataList[0].m_width;
        charDataList[0].height = imageDataList[0].m_height;
        charDataList[0].offsetX = 0;
        charDataList[0].offsetY = 0;

        // Calculate the remaining character's data
        int cursorX = imageDataList[0].m_width;
        int cursorY = 0;
        for (int i = 1; i < charCount; i++) {
            int charWidth = imageDataList[i].m_width;
            if (cursorX + charWidth > textureWidth) {
                cursorX = 0;
                cursorY += maxCharHeight;
            }
            charDataList[i].width = imageDataList[i].m_width;
            charDataList[i].height = imageDataList[i].m_height;
            charDataList[i].offsetX = cursorX;
            charDataList[i].offsetY = cursorY;
            cursorX += charWidth;
        }
        // Calculate texture height
        int textureHeight = cursorY + maxCharHeight;
        textureWidth = textureHeight;

        // Create an empty transparent image
        std::vector<unsigned char> finalImage(textureWidth * textureHeight * 4, 0);

        // Fill the pixel data
        for (size_t i = 0; i < charCount; i++) {
            unsigned char* srcPixels = static_cast<unsigned char*>(imageDataList[i].m_data);
            for (int y = 0; y < imageDataList[i].m_height; y++) {
                for (int x = 0; x < imageDataList[i].m_width; x++) {
                    int srcIndex = (y * imageDataList[i].m_width + x) * 4;
                    int destIndex = ((charDataList[i].offsetY + y) * textureWidth + (charDataList[i].offsetX + x)) * 4;
                    finalImage[destIndex + 0] = srcPixels[srcIndex + 0]; // R
                    finalImage[destIndex + 1] = srcPixels[srcIndex + 1]; // G
                    finalImage[destIndex + 2] = srcPixels[srcIndex + 2]; // B
                    finalImage[destIndex + 3] = srcPixels[srcIndex + 3]; // A
                }
            }
        }
        // Ensure the directory exists, create it if it doesn't
        if (!std::filesystem::exists(outputDir)) {
            std::filesystem::create_directories(outputDir);
        }

        // Save the image
        if (stbi_write_png(outputImagePath.string().c_str(), textureWidth, textureHeight, 4, finalImage.data(), textureWidth * 4)) {
            //std::cout << "Spritesheet saved successfully: " << outputImagePath << "\n";
        }
        else {
            std::cout << "Failed to save image: " << outputImagePath << "\n";
        }
        // Write the JSON
        std::ofstream jsonFile(outputJSONPath);
        if (jsonFile.is_open()) {
            jsonFile << "{\n";
            jsonFile << "  \"name\": \"" << EscapeString(name) << "\",\n";
            jsonFile << "  \"textureWidth\": " << textureWidth << ",\n";
            jsonFile << "  \"textureHeight\": " << textureHeight << ",\n";
            jsonFile << "  \"lineHeight\": " << maxCharHeight << ",\n";
            jsonFile << "  \"characters\": \"" << EscapeString(characters) << "\",\n";
            jsonFile << "  \"charDataList\": [\n";
            for (size_t i = 0; i < charDataList.size(); i++) {
                int width = charDataList[i].width;
                int height = charDataList[i].height;
                int offsetX = charDataList[i].offsetX;
                int offsetY = charDataList[i].offsetY;
                jsonFile << "    { \"width\": " << width
                    << ", \"height\": " << height
                    << ", \"offsetX\": " << offsetX
                    << ", \"offsetY\": " << offsetY << " }";
                if (i < charDataList.size() - 1) jsonFile << ",";
                jsonFile << "\n";
            }
            jsonFile << "  ]\n";
            jsonFile << "}\n";
            jsonFile.close();
        }
        else {
            std::cout << "Failed to save JSON: " << outputJSONPath << "\n";
        }
        // Free the image data
        for (ImageData& imageData : imageDataList) {
            if (imageData.m_data) {
                stbi_image_free(imageData.m_data);
            }
        }
    }

    FontSpriteSheet Import(const std::string& filepath) {
        FontSpriteSheet fontSpriteSheet;

        // Read the JSON file
        std::ifstream file(filepath);
        if (!file.is_open()) throw std::runtime_error("Failed to open file: " + filepath);
        std::stringstream buffer;
        buffer << file.rdbuf();
        std::string json = buffer.str();

        // Parse the JSON
        fontSpriteSheet.m_name = FindString(json, "name");
        fontSpriteSheet.m_characters = FindString(json, "characters");
        fontSpriteSheet.m_textureWidth = FindInt(json, "textureWidth");
        fontSpriteSheet.m_textureHeight = FindInt(json, "textureHeight");
        fontSpriteSheet.m_lineHeight = FindInt(json, "lineHeight");
        std::vector<std::string> charDataList = FindArray(json, "charDataList");
        for (const auto& charData : charDataList) {
            FontSpriteSheet::CharData data;
            data.width = FindInt(charData, "width");
            data.height = FindInt(charData, "height");
            data.offsetX = FindInt(charData, "offsetX");
            data.offsetY = FindInt(charData, "offsetY");
            fontSpriteSheet.m_charDataList.push_back(data);
        }
        return fontSpriteSheet;
    }

    std::string FindString(const std::string& json, const std::string& key) {
        size_t keyPos = json.find("\"" + key + "\":");
        if (keyPos == std::string::npos) {
            throw std::runtime_error("Key not found: " + key);
        }
        size_t start = json.find_first_of("\"", keyPos + key.length() + 2) + 1;
        if (start == std::string::npos) {
            throw std::runtime_error("Invalid JSON format: missing value for key: " + key);
        }
        size_t end = start;
        while (end < json.size()) {
            end = json.find("\"", end);
            if (end == std::string::npos) {
                throw std::runtime_error("Invalid JSON format: unterminated string for key: " + key);
            }
            if (end > 0 && json[end - 1] == '\\') {
                size_t backslashCount = 0;
                for (size_t i = end - 1; i >= start && json[i] == '\\'; --i) {
                    ++backslashCount;
                }
                if (backslashCount % 2 == 1) {
                    end++;
                    continue;
                }
            }
            break;
        }
        return UnescapeString(json.substr(start, end - start));
    }

    std::vector<std::string> FindArray(const std::string& json, const std::string& key) {
        size_t start = json.find("\"" + key + "\": [") + key.length() + 4;
        size_t end = json.find("]", start);
        std::string arrayContent = json.substr(start, end - start);
        std::vector<std::string> items;
        size_t pos = 0;
        while ((pos = arrayContent.find("{", pos)) != std::string::npos) {
            size_t close = arrayContent.find("}", pos);
            items.push_back(arrayContent.substr(pos, close - pos + 1));
            pos = close + 1;
        }
        return items;
    }

    int FindInt(const std::string& jsonChunk, const std::string& key) {
        size_t start = jsonChunk.find("\"" + key + "\":") + key.length() + 3;
        size_t end = jsonChunk.find_first_of(",", start);
        if (end == std::string::npos) end = jsonChunk.find_first_of("}", start);
        return std::stoi(jsonChunk.substr(start, end - start));
    }

    std::string EscapeString(const std::string& str) {
        std::string escaped;
        for (char c : str) {
            switch (c) {
            case '\"': escaped += "\\\""; break; // Escape double quote
            case '\\': escaped += "\\\\"; break; // Escape backslash
            case '/': escaped += "\\/"; break;  // Escape forward slash (optional)
            case '\b': escaped += "\\b"; break; // Escape backspace
            case '\f': escaped += "\\f"; break; // Escape form feed
            case '\n': escaped += "\\n"; break; // Escape newline
            case '\r': escaped += "\\r"; break; // Escape carriage return
            case '\t': escaped += "\\t"; break; // Escape tab
            default: escaped += c; break;       // Append other characters as-is
            }
        }
        return escaped;
    }

    std::string UnescapeString(const std::string& str) {
        std::string unescaped;
        size_t i = 0;
        while (i < str.size()) {
            if (str[i] == '\\' && i + 1 < str.size()) {
                switch (str[i + 1]) {
                case '\"': unescaped += '\"'; break; // Unescape double quote
                case '\\': unescaped += '\\'; break; // Unescape backslash
                case '/': unescaped += '/'; break;  // Unescape forward slash
                case 'b': unescaped += '\b'; break; // Unescape backspace
                case 'f': unescaped += '\f'; break; // Unescape form feed
                case 'n': unescaped += '\n'; break; // Unescape newline
                case 'r': unescaped += '\r'; break; // Unescape carriage return
                case 't': unescaped += '\t'; break; // Unescape tab
                default: unescaped += str[i + 1]; break; // Handle unknown sequences as-is
                }
                i += 2; // Skip the backslash and the escaped character
            }
            else {
                unescaped += str[i];
                i++;
            }
        }
        return unescaped;
    }

    std::vector<std::string> GetSortedFilePaths(const std::string& directory) {
        std::vector<std::string> filePaths;
        auto entries = std::filesystem::directory_iterator(directory);

        for (const auto& entry : entries) {
            if (std::filesystem::is_regular_file(entry)) {
                filePaths.push_back(entry.path().string());
            }
        }
        // Sort file paths numerically by filename
        std::sort(filePaths.begin(), filePaths.end(), [](const std::string& a, const std::string& b) {
            std::regex numberRegex("(\\d+)"); // Regex to extract numbers
            std::smatch matchA, matchB;
            bool foundA = std::regex_search(a, matchA, numberRegex);
            bool foundB = std::regex_search(b, matchB, numberRegex);
            if (foundA && foundB) {
                int numA = std::stoi(matchA.str());
                int numB = std::stoi(matchB.str());
                if (numA != numB) return numA < numB; // Compare numbers if found
            }
            return a < b; // Fall back to lexicographic order
        });
        return filePaths;
    }

    ImageData LoadImageData(const std::string& filepath) {
        stbi_set_flip_vertically_on_load(false);
        ImageData imageData;
        imageData.m_data = stbi_load(filepath.data(), &imageData.m_width, &imageData.m_height, &imageData.m_channelCount, 0);
        return imageData;
    }
}