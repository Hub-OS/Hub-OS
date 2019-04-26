/*! \file bnTextureResourceManager.h */

/*! \brief Singleton resource manager for texture data
 * 
 * Texture resource manager provides utilities to load textures from disc 
 * as well as hard-coded textures loaded at startup.
 * 
 * NOTE: This is legacy code that can be refactored. Could be renamed to 
 * Graphics Resource Manager. It also has methods to get chip rectangles
 * from the ID when the chips were intended to be hard-coded and used a 
 * sprite sheet. Now chips are going to be scripted and this is no 
 * longer needed and should be removed.
 */

#pragma once
#include "bnTextureType.h"
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
using sf::Texture;
using sf::Font;
using std::string;

class TextureResourceManager {
public:
  /**
   * @brief If this is the first call, initializes the resource manager. 
   * @return Returns reference to texture resource manager.
   */
  static TextureResourceManager& GetInstance();
  
  /**
   * @brief Loads all hard-coded textures
   * @param status Increases the count after each texture loads
   */
  void LoadAllTextures(std::atomic<int> &status);
  
  /**
   * @brief Given a file path, returns a pointer to the loaded texture
   * @param _path Relative path to the application
   * @return Texture pointer. Must manually delete.
   */
  Texture* LoadTextureFromFile(string _path);
  
  /**
   * @brief Returns pointer to the pre-loaded texture type
   * @param _ttype Texture type to fetch from cache
   * @return Texture pointer. 
   * @warning Do not delete! This resource is managed by the manager.
   */
  Texture* GetTexture(TextureType _ttype);
  
  /**
   * @brief Legacy code. Returns card rectangle for spritesheet.
   * @param ID ID of chip
   * @return IntRect
   */
  sf::IntRect GetCardRectFromID(unsigned ID);
  
  /**
   * @brief Legacy code. Returns icon rectangle for spritesheet.
   * @param ID ID of chip
   * @return IntRect
   */
  sf::IntRect GetIconRectFromID(unsigned ID);
  
  /**
   * @brief Given a file path, returns a pointer to the loaded font
   * @param _path Relative to the application
   * @return Font pointer. Must manually delete.
   */
  Font* LoadFontFromFile(string _path);

private:
  TextureResourceManager();
  ~TextureResourceManager();
  vector<string> paths; /**< Paths to all textures. Must be in order of TextureType @see TextureType */
  map<TextureType, Texture*> textures; /**< Cache */
};

/*! \brief Shorthand to get instance of the manager */
#define TEXTURES TextureResourceManager::GetInstance()

/*! \brief Shorthand to get a preloaded texture */
#define LOAD_TEXTURE(x) *TEXTURES.GetTexture(TextureType::x)