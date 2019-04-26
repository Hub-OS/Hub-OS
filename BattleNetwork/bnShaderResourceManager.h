/*! \file bnShaderResourceManager.h */

/*! \brief Singleton resource manager for shader data
 * 
 * Shader resource manager provides utilities to load shaders from disc 
 * as well as hard-coded shaders loaded at startup.
 * 
 * NOTE: This is legacy code that can be refactored.
 */

#pragma once
#include "bnShaderType.h"
#include "bnMemory.h"
#include "bnLogger.h"

#include <SFML/Graphics.hpp>
#include <map>
#include <vector>
#include <iostream>
#include <atomic>

using std::cerr;
using std::endl;
using std::vector;
using std::map;
using std::pair;
using std::string;

class ShaderResourceManager {
public:
  /**
   * @brief If this is the first call, initializes the resource manager. 
   * @return Returns reference to shader resource manager.
   */
  static ShaderResourceManager& GetInstance();
  
  /**
   * @brief Loads all hard-coded shaders
   * @param status Increases the count after each shader loads
   */
  void LoadAllShaders (std::atomic<int> &status);
  
  /**
   * @brief Given a file path, returns a pointer to the loaded shader
   * @param _path Relative path to the application
   * @return Shader pointer. Must manually delete.
   */
  sf::Shader* LoadShaderFromFile(string _path);
  
  /**
   * @brief Returns pointer to the pre-loaded shader type
   * @param _ttype shader type to fetch from cache
   * @return Shader pointer. 
   * @warning Do not delete! This resource is managed by the manager.
   */
  sf::Shader* GetShader(ShaderType _ttype);

private:
  ShaderResourceManager();
  ~ShaderResourceManager();
  vector<string> paths;  /*!< Paths to all shaders. Must be in order of ShaderType @see ShaderType */
  map<ShaderType, sf::Shader*> shaders; /*!< cache */
};

/*! \brief Shorthand to get instance of the manager */
#define SHADERS ShaderResourceManager::GetInstance()

/*! \brief Shorthand to get a preloaded shader */
#define LOAD_SHADER(x) *SHADERS.GetShader(ShaderType::x)