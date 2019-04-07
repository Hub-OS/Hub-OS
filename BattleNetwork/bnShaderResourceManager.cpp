#include "bnShaderResourceManager.h"
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
  while (shaderType != ShaderType::SHADER_TYPE_SIZE) {
    status++;

    // TODO: Catch failed resources and try again
    sf::Shader* shader = nullptr;
    shader = LoadShaderFromFile(paths[static_cast<int>(shaderType)]);
    if (shader) shaders.insert(pair<ShaderType, sf::Shader*>(shaderType, shader));
    shaderType = (ShaderType)(static_cast<int>(shaderType) + 1);
  }
}

sf::Shader* ShaderResourceManager::LoadShaderFromFile(string _path) {
  sf::Shader* shader = new sf::Shader();
  if (!shader->loadFromFile(_path, sf::Shader::Fragment)) {

    Logger::GetMutex()->lock();
    Logger::Log("Error loading shader: " + _path);
    Logger::GetMutex()->unlock();

//    exit(EXIT_FAILURE);
    return nullptr;
  }

  shader->setUniform("texture", sf::Shader::CurrentTexture);

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
  std::string version = "glsl_110";

  #ifdef SFML_SYSTEM_ANDROID
    version = "glsl_150";
  #endif

  paths.push_back(std::string() + "resources/shaders/" + version + "/black_fade.frag.txt");
  paths.push_back(std::string() + "resources/shaders/" + version + "/custom_bar.frag.txt");
  paths.push_back(std::string() + "resources/shaders/" + version + "/greyscale.frag.txt");
  paths.push_back(std::string() + "resources/shaders/" + version + "/outline.frag.txt");
  paths.push_back(std::string() + "resources/shaders/" + version + "/pixel_blur.frag.txt");
  paths.push_back(std::string() + "resources/shaders/" + version + "/texel_pixel_blur.frag.txt");
  paths.push_back(std::string() + "resources/shaders/" + version + "/texel_texture_wrap.frag.txt");
  paths.push_back(std::string() + "resources/shaders/" + version + "/white.frag.txt");
  paths.push_back(std::string() + "resources/shaders/" + version + "/white_fade.frag.txt");
  paths.push_back(std::string() + "resources/shaders/" + version + "/yellow.frag.txt");
  paths.push_back(std::string() + "resources/shaders/" + version + "/distortion.frag.txt");
  paths.push_back(std::string() + "resources/shaders/" + version + "/spot_distortion.frag.txt");
  paths.push_back(std::string() + "resources/shaders/" + version + "/spot_reflection.frag.txt");
  paths.push_back(std::string() + "resources/shaders/" + version + "/transition.frag.txt");

  #ifdef SFML_SYSTEM_ANDROID
    // ShaderType::ES2_DEFAULT_FRAG
    paths.push_back(std::string() + "resources/shaders/" + version + "/default.frag.txt");

    // ShaderType::ES2_DEFAULT_VERT
    paths.push_back(std::string() + "resources/shaders/" + version + "/default.vert.txt");
  #endif
}

ShaderResourceManager::~ShaderResourceManager(void) {
  for (auto it = shaders.begin(); it != shaders.end(); ++it) {
    delete it->second;
  }
}