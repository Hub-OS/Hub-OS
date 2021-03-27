#pragma once

#include <SFML/Graphics/Texture.hpp>
#include <SFML/Audio/SoundBuffer.hpp>
#include <Poco/Buffer.h>
#include <memory>
#include <unordered_map>

namespace Overworld {
  class ServerAssetManager {
  private:
    struct CacheMeta {
      const std::string path;
      const uint64_t lastModified{};
      const size_t size{};
    };

    std::unordered_map<std::string, std::string> textAssets;
    std::unordered_map<std::string, std::shared_ptr<sf::Texture>> textureAssets;
    std::unordered_map<std::string, std::shared_ptr<sf::SoundBuffer>> audioAssets;
    std::string cachePath;
    std::string cachePrefix;
    std::unordered_map<std::string, CacheMeta> cachedAssets;

    void CacheAsset(const std::string& name, uint64_t lastModified, const char* data, size_t size);
    std::vector<char> LoadFromCache(const std::string& name);
  public:
    ServerAssetManager(const std::string& cachePath);

    std::string GetPath(const std::string& name);
    const std::unordered_map<std::string, CacheMeta>& GetCachedAssetList();

    std::string GetText(const std::string& name);
    std::shared_ptr<sf::Texture> GetTexture(const std::string& name);
    std::shared_ptr<sf::SoundBuffer> GetAudio(const std::string& name);

    void SetText(const std::string& name, uint64_t lastModified, const std::string& data, bool cache);
    void SetTexture(const std::string& name, uint64_t lastModified, const char* data, size_t length, bool cache);
    void SetAudio(const std::string& name, uint64_t lastModified, const char* data, size_t length, bool cache);
    void RemoveAsset(const std::string& name);
  };
}
