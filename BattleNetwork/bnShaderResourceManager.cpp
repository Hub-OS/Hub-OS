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
#ifdef __ANDROID__
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
#else 
    sf::Shader* shader = new sf::Shader();
    if (!shader->loadFromFile(_path + ".frag", sf::Shader::Fragment)) {

      Logger::GetMutex()->lock();
      Logger::Log("Error loading shader: " + _path + ".frag");
      Logger::GetMutex()->unlock();

      exit(EXIT_FAILURE);
      return nullptr;
    }
#endif

    //shader->setUniform("texture", sf::Shader::CurrentTexture);

    Logger::GetMutex()->lock();
    Logger::Log("Loaded shader: " + _path);
    Logger::GetMutex()->unlock();

    return shader;
}

sf::Shader* ShaderResourceManager::GetShader(ShaderType _stype) {
  return shaders.at(_stype);
}

ShaderResourceManager::ShaderResourceManager(void) {

#ifdef SFML_SYSTEM_ANDROID
    std::string version = "glsl_150";
#else
    std::string version = "glsl_110";
#endif

    paths.resize((size_t)ShaderType::SHADER_TYPE_SIZE);

#ifdef SFML_SYSTEM_ANDROID
    paths[(int)ShaderType::DEFAULT] = std::string() + "resources/shaders/" + version + "/default";
#endif
    paths[(int)ShaderType::BLACK_FADE] = std::string() + "resources/shaders/" + version + "/black_fade";
    paths[(int)ShaderType::CUSTOM_BAR] = std::string() + "resources/shaders/" + version + "/custom_bar";
    paths[(int)ShaderType::GREYSCALE] = std::string() + "resources/shaders/" + version + "/greyscale";
    paths[(int)ShaderType::OUTLINE] = std::string() + "resources/shaders/" + version + "/outline";
    paths[(int)ShaderType::PIXEL_BLUR] = std::string() + "resources/shaders/" + version + "/pixel_blur";
    paths[(int)ShaderType::TEXEL_PIXEL_BLUR] = std::string() + "resources/shaders/" + version + "/texel_pixel_blur";
    paths[(int)ShaderType::TEXEL_TEXTURE_WRAP] = std::string() + "resources/shaders/" + version + "/texel_texture_wrap";
    paths[(int)ShaderType::WHITE] = std::string() + "resources/shaders/" + version + "/white";
    paths[(int)ShaderType::WHITE_FADE] = std::string() + "resources/shaders/" + version + "/white_fade";
    paths[(int)ShaderType::YELLOW] = std::string() + "resources/shaders/" + version + "/yellow";
    paths[(int)ShaderType::DISTORTION] = std::string() + "resources/shaders/" + version + "/distortion";
    paths[(int)ShaderType::SPOT_DISTORTION] = std::string() + "resources/shaders/" + version + "/spot_distortion";
    paths[(int)ShaderType::SPOT_REFLECTION] = std::string() + "resources/shaders/" + version + "/spot_reflection";
    paths[(int)ShaderType::CHIP_REVEAL] = std::string() + "resources/shaders/" + version + "/chip_reveal";
    paths[(int)ShaderType::BADGE_WIRE] = std::string() + "resources/shaders/" + version + "/wire";
    paths[(int)ShaderType::TRANSITION] = std::string() + "resources/shaders/" + version + "/transition";
    paths[(int)ShaderType::COLORIZE] = std::string() + "resources/shaders/" + version + "/colorize";
}

ShaderResourceManager::~ShaderResourceManager(void) {
  for (auto it = shaders.begin(); it != shaders.end(); ++it) {
    delete it->second;
  }
}