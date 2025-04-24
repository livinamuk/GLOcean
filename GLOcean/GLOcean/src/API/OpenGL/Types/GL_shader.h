#pragma once
#include <glm/glm.hpp>
#include <unordered_map>
#include <string>
#include <vector>

struct ShaderModule {
public:
    ShaderModule(const std::string& filename);
    int GetHandle();
    bool CompilationFailed();
    std::string& GetFilename();
    std::string& GetErrors();
    std::vector<std::string>& GetLineMap();
private:
    int m_handle = -1;
    std::string m_filename = "";
    std::string m_errors = "";
    std::vector<std::string> m_lineMap;
};

struct Shader {
public:
    void Use();
    bool Load(std::vector<std::string> shaderPaths);
    void SetInt(const std::string& name, int value);
    void SetBool(const std::string& name, bool value);
    void SetFloat(const std::string& name, float value);
    void SetMat2(const std::string& name, const glm::mat2& mat);
    void SetMat3(const std::string& name, const glm::mat3& mat);
    void SetMat4(const std::string& name, glm::mat4 value);
    void SetVec2(const std::string& name, const glm::vec2& value);
    void SetVec4(const std::string& name, const glm::vec4& value);
    void SetVec3(const std::string& name, const glm::vec3& value);
    void SetVec2(const std::string& name, float x, float y);
    void SetVec3(const std::string& name, float x, float y, float z);
    void SetVec4(const std::string& name, float x, float y, float z, float w);
    void SetUvec2(const std::string& name, const glm::uvec2& value);
    int GetHandle();
private:
    std::unordered_map<std::string, int> m_uniformLocations;
    int m_handle = -1;
};