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
  while (shaderType != SHADER_TYPE_SIZE) {
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

    exit(EXIT_FAILURE);
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

ShaderResourceManager::ShaderResourceManager(void) {
  paths.push_back("resources/shaders/black_fade.frag.txt");
  paths.push_back("resources/shaders/custom_bar.frag.txt");
  paths.push_back("resources/shaders/greyscale.frag.txt");
  paths.push_back("resources/shaders/outline.frag.txt");
  paths.push_back("resources/shaders/pixel_blur.frag.txt");
  paths.push_back("resources/shaders/texel_pixel_blur.frag.txt");
  paths.push_back("resources/shaders/texel_texture_wrap.frag.txt");
  paths.push_back("resources/shaders/white.frag.txt");
  paths.push_back("resources/shaders/white_fade.frag.txt");
  paths.push_back("resources/shaders/yellow.frag.txt");
  paths.push_back("resources/shaders/distortion.frag.txt");
  paths.push_back("resources/shaders/spot_distortion.frag.txt");
  paths.push_back("resources/shaders/spot_reflection.frag.txt");
  paths.push_back("resources/shaders/transition.frag.txt");
  paths.push_back("resources/shaders/chip_reveal.frag.txt");
}

ShaderResourceManager::~ShaderResourceManager(void) {
  for (auto it = shaders.begin(); it != shaders.end(); ++it) {
    delete it->second;
  }
}