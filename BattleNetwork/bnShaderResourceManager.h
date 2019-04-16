#pragma once
#include "bnShaderType.h"
#include "bnMemory.h"
#include "bnLogger.h"

#include <SFML/Graphics.hpp>
using std::string;
#include <map>
using std::map;
using std::pair;
#include <vector>
using std::vector;
#include <iostream>
using std::cerr;
using std::endl;
#include <atomic>

class ShaderResourceManager {
public:
  static ShaderResourceManager& GetInstance();
  void LoadAllShaders (std::atomic<int> &status);
  sf::Shader* LoadShaderFromFile(string _path);
  sf::Shader* GetShader(ShaderType _ttype);

private:
  ShaderResourceManager(void);
  ~ShaderResourceManager(void);
  vector<string> paths;
  map<ShaderType, sf::Shader*> shaders;
};

#define SHADERS ShaderResourceManager::GetInstance()
#define LOAD_SHADER(x) *SHADERS.GetShader(ShaderType::x)