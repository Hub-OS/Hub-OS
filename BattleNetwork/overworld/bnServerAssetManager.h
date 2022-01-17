#pragma once

#include <SFML/Graphics/Texture.hpp>
#include <SFML/Audio/SoundBuffer.hpp>
#include <Poco/Buffer.h>
#include <filesystem>
#include <memory>
#include <unordered_map>

namespace Overworld {
  std::string URIEncode(const std::string& name);

  class ServerAssetManager {
  private:
    struct CacheMeta {
      std::filesystem::path path;
      uint64_t lastModified{};
      size_t size{};
    };

    std::unordered_map<std::string, std::string> textAssets;
    std::unordered_map<std::string, std::shared_ptr<sf::Texture>> textureAssets;
    std::unordered_map<std::string, std::shared_ptr<sf::SoundBuffer>> audioAssets;
    std::unordered_map<std::string, std::vector<char>> dataAssets;
    std::filesystem::path cachePath;
    std::filesystem::path cachePrefix;
    std::unordered_map<std::string, CacheMeta> cachedAssets;

    void CacheAsset(const std::string& name, uint64_t lastModified, const char* data, size_t size);
    std::vector<char> LoadFromCache(const std::string& name);
  public:
    ServerAssetManager(const std::string& host, uint16_t port);

    std::filesystem::path GetPath(const std::string& name);
    const std::unordered_map<std::string, CacheMeta>& GetCachedAssetList();

    void Preload(const std::string& name);
    void PreloadText(const std::string& name);
    void PreloadTexture(const std::string& name);
    void PreloadAudio(const std::string& name);

    std::string GetText(const std::string& name);
    std::shared_ptr<sf::Texture> GetTexture(const std::string& name);
    std::shared_ptr<sf::SoundBuffer> GetAudio(const std::string& name);
    std::vector<char> GetData(const std::string& name);

    void SetText(const std::string& name, uint64_t lastModified, const std::string& data, bool cache);
    void SetTexture(const std::string& name, uint64_t lastModified, const char* data, size_t length, bool cache);
    void SetAudio(const std::string& name, uint64_t lastModified, const char* data, size_t length, bool cache);
    void SetData(const std::string& name, uint64_t lastModified, const char* data, size_t length, bool cache);
    void RemoveAsset(const std::string& name);
  };
}
