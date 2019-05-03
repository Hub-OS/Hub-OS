#include "bnShaderResourceManager.h"
#include "bnShaderType.h"
#include <stdlib.h>
#include <sstream>
using std::stringstream;
#include <fstream>
using std::ifstream;

ShaderResourceManager& ShaderResourceManager::GetInstance() {
  static ShaderResourceManager instance;
  return instance;
}

void ShaderResourceManager::LoadAllShaders(std::atomic<int> &status) {

    ShaderType shaderType = static_cast<ShaderType>(0);
    while (shaderType != ShaderType::SHADER_TYPE_SIZE)
    {
        status++;

        // TODO: Catch failed resources and try again
        sf::Shader* shader = LoadShaderFromFile(paths[static_cast<int>(shaderType)]);
        if (shader)
        {
            shaders.insert(pair<ShaderType, sf::Shader*>(shaderType, shader));
        }
        shaderType = (ShaderType)(static_cast<int>(shaderType) + 1);
    }
}

sf::Shader* ShaderResourceManager::LoadShaderFromFile(string _path)
{
    sf::Shader* shader = new sf::Shader();
    bool result = false;

    if(shader->loadFromFile(_path + ".vert", _path + ".frag"))
    {
        result = true;
    }
    else // default vert shader
    {
        result = shader->loadFromFile(paths[static_cast<int>(ShaderType::DEFAULT)] + ".vert", _path + ".frag");
    }

    if (!result)
    {
        Logger::GetMutex()->lock();
        Logger::Log("Error loading shader: " + _path);
        Logger::GetMutex()->unlock();

        return nullptr;
    }

    //shader->setUniform("texture", sf::Shader::CurrentTexture);

    Logger::GetMutex()->lock();
    Logger::Log("Loaded shader: " + _path);
    Logger::GetMutex()->unlock();

    return shader;
}

sf::Shader* ShaderResourceManager::GetShader(ShaderType _stype) {
    return shaders.at(_stype);
}

const int ShaderResourceManager::GetSize() {
    return shaders.size();
}

ShaderResourceManager::ShaderResourceManager(void) {

#ifdef SFML_SYSTEM_ANDROID
    std::string version = "glsl_150";
#else
    std::string version = "glsl_110";
#endif

#ifdef SFML_SYSTEM_ANDROID
    paths.push_back(std::string() + "resources/shaders/" + version + "/default");
#endif
    paths.push_back(std::string() + "resources/shaders/" + version + "/black_fade");
    paths.push_back(std::string() + "resources/shaders/" + version + "/custom_bar");
    paths.push_back(std::string() + "resources/shaders/" + version + "/greyscale");
    paths.push_back(std::string() + "resources/shaders/" + version + "/outline");
    paths.push_back(std::string() + "resources/shaders/" + version + "/pixel_blur");
    paths.push_back(std::string() + "resources/shaders/" + version + "/texel_pixel_blur");
    paths.push_back(std::string() + "resources/shaders/" + version + "/texel_texture_wrap");
    paths.push_back(std::string() + "resources/shaders/" + version + "/white");
    paths.push_back(std::string() + "resources/shaders/" + version + "/white_fade");
    paths.push_back(std::string() + "resources/shaders/" + version + "/yellow");
    paths.push_back(std::string() + "resources/shaders/" + version + "/distortion");
    paths.push_back(std::string() + "resources/shaders/" + version + "/spot_distortion");
    paths.push_back(std::string() + "resources/shaders/" + version + "/spot_reflection");
    paths.push_back(std::string() + "resources/shaders/" + version + "/transition");
}

ShaderResourceManager::~ShaderResourceManager(void) {
  for (auto it = shaders.begin(); it != shaders.end(); ++it) {
    delete it->second;
  }
}