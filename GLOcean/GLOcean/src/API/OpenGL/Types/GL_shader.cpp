#include "GL_shader.h"
#include <glad/glad.h>
#include <filesystem>
#include <iostream>
#include <fstream>
#include <sstream>
#include <regex>

void ParseFile(const std::string& filepath, std::string& outputString, std::vector<std::string>& lineToFile, std::vector<std::string>& includedPaths);
int GetErrorLineNumber(const std::string& error);
std::string GetErrorMessage(const std::string& line);
std::string GetLinkingErrors(unsigned int shader);
std::string GetShaderCompileErrors(unsigned int shader, const std::string& filename, const std::vector<std::string>& lineToFile);

void Shader::Use() {
    glUseProgram(m_handle);
}

bool Shader::Load(std::vector<std::string> shaderPaths) {

    std::vector<ShaderModule> modules;
    for (std::string& shaderPath : shaderPaths) {
        modules.push_back(shaderPath);
    }

    bool errorsFound = false;
    for (ShaderModule& module : modules) {
        if (module.CompilationFailed()) {
            errorsFound = true;
            break;
        }
    }
    if (errorsFound) {
        std::cout << "\n-------------------------------------------------------------------------\n\n";
        for (ShaderModule& module : modules) {
            if (module.CompilationFailed()) {
                std::cout << " COMPILATION ERROR: " << module.GetFilename() << "\n\n";
                std::cout << module.GetErrors() << "\n";
            }
            glDeleteShader(module.GetHandle());
        }
        std::cout << "-------------------------------------------------------------------------\n";
        return false;
    }

    // Link stage
    int tempHandle = glCreateProgram();
    for (ShaderModule& module : modules) {
        glAttachShader(tempHandle, module.GetHandle());
    }
    glLinkProgram(tempHandle);

    GLint success = 0;
    glGetProgramiv(tempHandle, GL_LINK_STATUS, &success);
    if (success == GL_FALSE) {
        GLint logLength = 0;
        glGetProgramiv(tempHandle, GL_INFO_LOG_LENGTH, &logLength);
        std::vector<char> log(logLength);
        glGetProgramInfoLog(tempHandle, logLength, &logLength, log.data());

        std::cout << "\n-------------------------------------------------------------------------\n\n";
        std::cout << " LINKING ERROR: ";
        for (int i = 0; i < modules.size(); i++) {
            std::cout << modules[i].GetFilename();
            if (i != modules.size() - 1) {
                std::cout << "/";
            }
        }
        std::cout << "\n\n" << log.data() << "\n";
        std::cout << "-------------------------------------------------------------------------\n";

        glDeleteProgram(tempHandle);
        return false;
    }

    if (m_handle != -1) {
        glDeleteProgram(m_handle);
    }
    m_handle = tempHandle;
    m_uniformLocations.clear();

    /*
    // Auto-bind samplers declared with layout(binding=N)
    glUseProgram(m_handle);
    std::regex samplerRe(R"(layout\s*\(\s*binding\s*=\s*(\d+)\s*\)\s*uniform\s*(sampler\w+)\s+(\w+)\s*;)");
    const std::string baseDir = "res/shaders/OpenGL/";

    for (const auto& filename : shaderPaths) {

        if (filename != "GL_ocean_geometry.tese") continue;

        std::string fullPath = baseDir + filename;
       // std::cout << "[DEBUG] Processing: " << fullPath << "\n";
        std::ifstream in(fullPath);
        if (!in) {
            std::cout << "[DEBUG]  <failed to open>\n";
            continue;
        }

        std::string line;
        int lineNum = 1;
        while (std::getline(in, line)) {
            //std::cout << "[DEBUG]  L" << lineNum << ": " << line << "\n";
            std::smatch m;
            if (std::regex_search(line, m, samplerRe)) {
                std::cout << "[DEBUG]   Matched: unit=" << m[1]
                    << ", type=" << m[2]
                    << ", name=" << m[3] << "\n";
                int unit = std::stoi(m[1].str());
                std::string name = m[3].str();
                GLint loc = glGetUniformLocation(m_handle, name.c_str());
                std::cout << "[DEBUG]   Location of '" << name << "' = " << loc << "\n";
                if (loc >= 0) {
                    glUniform1i(loc, unit);
                    std::cout << "[DEBUG]   Bound '" << name << "' to unit " << unit << "\n";
                }
            }
            ++lineNum;
        }
    }*/

    for (ShaderModule& module : modules) {
        glDeleteShader(module.GetHandle());
    }
    return true;
}

void Shader::SetBool(const std::string& name, bool value) {
    if (m_uniformLocations.find(name) == m_uniformLocations.end()) {
        m_uniformLocations[name] = glGetUniformLocation(m_handle, name.c_str());
    }
    glUniform1i(m_uniformLocations[name], (int)value);
}

void Shader::SetInt(const std::string& name, int value) {
    if (m_uniformLocations.find(name) == m_uniformLocations.end()) {
        m_uniformLocations[name] = glGetUniformLocation(m_handle, name.c_str());
    }
    glUniform1i(m_uniformLocations[name], value);
}

void Shader::SetFloat(const std::string& name, float value) {
    if (m_uniformLocations.find(name) == m_uniformLocations.end()) {
        m_uniformLocations[name] = glGetUniformLocation(m_handle, name.c_str());
    }
    glUniform1f(m_uniformLocations[name], value);
}

void Shader::SetMat2(const std::string& name, const glm::mat2& mat) {
    if (m_uniformLocations.find(name) == m_uniformLocations.end()) {
        m_uniformLocations[name] = glGetUniformLocation(m_handle, name.c_str());
    }
    glUniformMatrix2fv(m_uniformLocations[name], 1, GL_FALSE, &mat[0][0]);
}

void Shader::SetMat3(const std::string& name, const glm::mat3& mat) {
    if (m_uniformLocations.find(name) == m_uniformLocations.end()) {
        m_uniformLocations[name] = glGetUniformLocation(m_handle, name.c_str());
    }
    glUniformMatrix3fv(m_uniformLocations[name], 1, GL_FALSE, &mat[0][0]);
}

void Shader::SetMat4(const std::string& name, glm::mat4 value) {
    if (m_uniformLocations.find(name) == m_uniformLocations.end()) {
        m_uniformLocations[name] = glGetUniformLocation(m_handle, name.c_str());
    }
    glUniformMatrix4fv(m_uniformLocations[name], 1, GL_FALSE, &value[0][0]);
}

void Shader::SetUvec2(const std::string& name, const glm::uvec2& value) {
    if (m_uniformLocations.find(name) == m_uniformLocations.end()) {
        m_uniformLocations[name] = glGetUniformLocation(m_handle, name.c_str());
    }
    glUniform2uiv(m_uniformLocations[name], 1, &value[0]);
}

void Shader::SetVec2(const std::string& name, const glm::vec2& value) {
    if (m_uniformLocations.find(name) == m_uniformLocations.end()) {
        m_uniformLocations[name] = glGetUniformLocation(m_handle, name.c_str());
    }
    glUniform2fv(m_uniformLocations[name], 1, &value[0]);
}

void Shader::SetVec3(const std::string& name, const glm::vec3& value) {
    if (m_uniformLocations.find(name) == m_uniformLocations.end()) {
        m_uniformLocations[name] = glGetUniformLocation(m_handle, name.c_str());
    }
    glUniform3fv(m_uniformLocations[name], 1, &value[0]);
}

void Shader::SetVec4(const std::string& name, const glm::vec4& value) {
    if (m_uniformLocations.find(name) == m_uniformLocations.end()) {
        m_uniformLocations[name] = glGetUniformLocation(m_handle, name.c_str());
    }
    glUniform4fv(m_uniformLocations[name], 1, &value[0]);
}

void Shader::SetVec2(const std::string& name, float x, float y) {
    if (m_uniformLocations.find(name) == m_uniformLocations.end()) {
        m_uniformLocations[name] = glGetUniformLocation(m_handle, name.c_str());
    }
    glUniform2f(m_uniformLocations[name], x, y);
}

void Shader::SetVec3(const std::string& name, float x, float y, float z) {
    if (m_uniformLocations.find(name) == m_uniformLocations.end()) {
        m_uniformLocations[name] = glGetUniformLocation(m_handle, name.c_str());
    }
    glUniform3f(m_uniformLocations[name], x, y, z);
}

void Shader::SetVec4(const std::string& name, float x, float y, float z, float w) {
    if (m_uniformLocations.find(name) == m_uniformLocations.end()) {
        m_uniformLocations[name] = glGetUniformLocation(m_handle, name.c_str());
    }
    glUniform4f(m_uniformLocations[name], x, y, z, w);
}

int Shader::GetHandle() {
    return m_handle;
}

ShaderModule::ShaderModule(const std::string& filename) {
    // Parse the source code
    std::vector<std::string> lineMap;
    std::vector<std::string> includedPaths;
    std::string prasedShaderSource = "";
    ParseFile("res/shaders/OpenGL/" + filename, prasedShaderSource, lineMap, includedPaths);

    // Get type based on extension
    std::string extension = std::filesystem::path(filename).extension().string(); 
    static const std::unordered_map<std::string, int> shaderTypeMap = {
        {".vert", GL_VERTEX_SHADER},
        {".frag", GL_FRAGMENT_SHADER},
        {".geom", GL_GEOMETRY_SHADER},
        {".tesc", GL_TESS_CONTROL_SHADER},
        {".tese", GL_TESS_EVALUATION_SHADER},
        {".comp", GL_COMPUTE_SHADER}
    };
    int shaderType = shaderTypeMap.contains(extension) ? shaderTypeMap.at(extension) : GL_NONE;

    // Check for errors
    const char* shaderCode = prasedShaderSource.c_str();
    m_handle = glCreateShader(shaderType);
    glShaderSource(m_handle, 1, &shaderCode, NULL);
    glCompileShader(m_handle);
    m_errors = GetShaderCompileErrors(m_handle, filename, lineMap);
    m_filename = filename;
}

int ShaderModule::GetHandle() {
    return m_handle;
}

bool ShaderModule::CompilationFailed() {
    return m_errors.length();
}

std::string& ShaderModule::GetFilename() {
    return m_filename;
}

std::string& ShaderModule::GetErrors() {
    return m_errors;
}

std::vector<std::string>& ShaderModule::GetLineMap() {
    return m_lineMap;
}

void ParseFile(const std::string& filepath, std::string& outputString, std::vector<std::string>& lineToFile, std::vector<std::string>& includedPaths) {
    std::string baseDir = std::filesystem::path(filepath).parent_path().string();
    std::string filename = std::filesystem::path(filepath).filename().string();
    std::ifstream file(filepath);
    std::string line;
    int lineNumber = 0;
    while (std::getline(file, line)) {
        // Handle includes
        if (line.find("#include") != std::string::npos) {
            size_t start = line.find("\"") + 1;
            size_t end = line.find("\"", start);
            std::string includeFile = line.substr(start, end - start);
            std::string includePath = std::filesystem::weakly_canonical(baseDir + "/" + includeFile).string();
            // Check if the included file is already in includedPaths
            if (std::find(includedPaths.begin(), includedPaths.end(), includePath) == includedPaths.end()) {
                includedPaths.push_back(includePath);
                ParseFile(includePath, outputString, lineToFile, includedPaths);
            }
        }
        else {
            outputString += line + "\n";
            lineToFile.emplace_back(filename + " (line " + std::to_string(lineNumber++) + ")");
        }
    }
}

std::string GetShaderCompileErrors(unsigned int shader, const std::string& filename, const std::vector<std::string>& lineToFile) {
    int success;
    char infoLog[1024];
    std::string result = "";
    glGetShaderiv(shader, GL_COMPILE_STATUS, &success);
    if (!success) {
        glGetShaderInfoLog(shader, 1024, nullptr, infoLog);
        // Parse error log to extract line numbers
        std::stringstream logStream(infoLog);
        std::string line;
        while (std::getline(logStream, line)) {
            if ((line.substr(0, 7) == "ERROR: ")) {
                int lineNumber = GetErrorLineNumber(line);
                if (lineNumber >= 0 && lineNumber < lineToFile.size()) {
                    result += "  " + lineToFile[lineNumber] + ": " + GetErrorMessage(line) + "\n";
                }
            }
        }
    }
    return result;
}

std::string GetLinkingErrors(unsigned int shader) {
    int success;
    char infoLog[1024];
    std::string result = "";
    glGetProgramiv(shader, GL_LINK_STATUS, &success);
    if (!success) {
        glGetProgramInfoLog(shader, 1024, NULL, infoLog);
        std::stringstream logStream(infoLog);
        std::string line;
        while (std::getline(logStream, line)) {
            result = "  " + line + "\n";
        }
    }
    return result;
}

int GetErrorLineNumber(const std::string& error) {
    size_t firstColon = error.find(':');
    if (firstColon != std::string::npos) {
        size_t secondColon = error.find(':', firstColon + 1);
        if (secondColon != std::string::npos) {
            size_t thirdColon = error.find(':', secondColon + 1);
            if (thirdColon != std::string::npos) {
                std::string lineNumberStr = error.substr(secondColon + 1, thirdColon - secondColon - 1);
                return std::stoi(lineNumberStr);
            }
        }
    }
    return -1;
}

std::string GetErrorMessage(const std::string& line) {
    size_t firstColon = line.find(':');
    if (firstColon != std::string::npos) {
        size_t secondColon = line.find(':', firstColon + 1);
        if (secondColon != std::string::npos) {
            size_t thirdColon = line.find(':', secondColon + 1);
            if (thirdColon != std::string::npos) {
                size_t messageStart = thirdColon + 2; // Skip the colon and space
                if (messageStart < line.length()) {
                    return line.substr(messageStart);
                }
            }
        }
    }
    return ""; // Return empty string if parsing fails
}