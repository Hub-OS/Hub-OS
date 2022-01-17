/*! \file bnShaderResourceManager.h */

/*! \brief Singleton resource manager for scripts and lua state machine
 *
 * Script resource manager is intended to be a closed-off resource that the battle engine
 * uses to fetch and inject lua into the environment
 * Some Scripted class implementations will use this resource to pass off to the correct callbacks
 */

#pragma once
#include "bnShaderType.h"
#include "bnLogger.h"

#include <SFML/Graphics.hpp>
#include <filesystem>
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
   * @brief Loads all hard-coded shaders
   * @param status Increases the count after each shader loads
   */
  void LoadAllShaders (std::atomic<int> &status);

  /**
   * @brief Given a file path, returns a pointer to the loaded shader
   * @param _path Relative path to the application
   * @return Shader pointer. Must manually delete.
   */
  sf::Shader* LoadShaderFromFile(const std::filesystem::path& _path);

  /**
   * @brief Returns pointer to the pre-loaded shader type
   * @param _ttype shader type to fetch from cache
   * @return Shader pointer.
   * @warning Do not delete! This resource is managed by the manager.
   */
  sf::Shader* GetShader(ShaderType _ttype);

  const bool IsEnabled() const;

  ShaderResourceManager();
  ~ShaderResourceManager();
private:
  std::mutex mutex;
  vector<std::filesystem::path> paths;  /*!< Paths to all shaders. Must be in order of ShaderType @see ShaderType */
  bool isEnabled{};
  map<ShaderType, sf::Shader*> shaders; /*!< cache */
};
