#include "bnTextureResourceManager.h"

#include <stdlib.h>
#include <atomic>
#include <sstream>
#include <fstream>
#include <mutex>

#include "bnFileUtil.h"

using std::ifstream;
using std::stringstream;

void TextureResourceManager::LoadAllTextures(std::atomic<int>& status)
{
  /*
  todo: add perm textures here to load at boot
  */
}

void TextureResourceManager::HandleExpiredTextureCache()
{
  // std::scoped_lock lock(mutex);

  auto iter = texturesFromPath.begin();
  while (iter != texturesFromPath.end()) {
    if (iter->second.GetSecondsSinceLastRequest() > 60.0f) {
      if (iter->second.IsInUse()) {
        iter++; continue;
      }

      // 1 minute is long enough
      Logger::Logf(LogLevel::debug, "Texture data %s expired", iter->first.c_str());
      iter = texturesFromPath.erase(iter);
      continue;
    }

    iter++;
  }
}

std::shared_ptr<Texture> TextureResourceManager::LoadFromFile(const std::filesystem::path& _path) {
  //std::scoped_lock lock(mutex);

  auto iter = texturesFromPath.find(_path);

  // check cache first
  if (iter != texturesFromPath.end()) {
    return iter->second.GetResource();
  }

  auto pathsIter = std::find(paths.begin(), paths.end(), _path);

  bool skipCaching = false;

  if (pathsIter != paths.end()) {
    skipCaching = true;
  }

  std::shared_ptr<Texture> texture = std::make_shared<Texture>();

  StdFilesystemInputStream stream(_path);
  if (!texture->loadFromStream(stream)) {
    Logger::Log(LogLevel::critical, "Failed loading texture: " + _path.u8string());
  } else {
    Logger::Log(LogLevel::info, "Loaded texture: " + _path.u8string());
  }

  if (!skipCaching) {
    texturesFromPath.insert(std::make_pair(_path, texture));
  }

  return texture;
}

TextureResourceManager::TextureResourceManager() {
}

TextureResourceManager::~TextureResourceManager() {
}
