#include "bnServerAssetManager.h"

#include "../bnLogger.h"
#include <fstream>
#include <sstream>
#include <string_view>
#include <iterator>

#ifndef __APPLE__
  // TODO: mac os < 10.15 file system support
  #include<filesystem>
#endif

constexpr std::string_view CACHE_FOLDER = "cache";

static char encodeHexChar(char c) {
  if (c < 10) {
    return c + '0';
  }

  return c - 10 + 'A';
}

// prevents a server from doing something dangerous by abusing special characters that we may be unaware of
// also allows us to have a flat file structure for easy reading
static std::string URIEncode(std::string name) {
  std::stringstream encodedName;

  for (auto i = 0; i < name.length(); i++) {
    auto c = name[i];

    if (std::isalpha(c) || std::isdigit(c) || c == '.' || c == ' ' || c == '-' || c == '_') {
      encodedName << c;
      continue;
    }

    encodedName << "%" << encodeHexChar(c >> 4) << encodeHexChar(c & 0b00001111);
  }

  return encodedName.str();
}

static char decodeHexChar(char h) {
  if (isalpha(h)) {
    return h - 'A' + 10;
  }

  return h - '0';
}

static std::string URIDecode(std::string encodedName) {
  std::stringstream decodedName;

  auto len = encodedName.size();

  for (auto i = 0; i < len; i++) {
    char c = encodedName[i];

    if (c != '%') {
      decodedName << c;
      continue;
    }

    if (i + 2 >= len) {
      break;
    }

    char high = decodeHexChar(encodedName[i + 1]);
    char low = decodeHexChar(encodedName[i + 2]);
    i += 2;

    c = (high << 4) | low;

    decodedName << c;
  }

  return decodedName.str();
}

static std::string encodeName(const std::string& name, uint64_t lastModified) {
  return std::to_string(lastModified) + "-" + URIEncode(name);
}

static std::tuple<std::string, uint64_t> decodeName(const std::string& name) {
  auto dashIndex = name.find('-');

  if (dashIndex == std::string::npos) {
    return { URIDecode(name), 0 };
  }

  uint64_t lastModifiedTime;

  try {
    lastModifiedTime = stoull(name);
  }
  catch (std::exception&) {
    lastModifiedTime = 0;
  }

  return { URIDecode(name.substr(dashIndex + 1)), lastModifiedTime };
}


Overworld::ServerAssetManager::ServerAssetManager(const std::string& address, uint16_t port) :
  cachePath(std::string(CACHE_FOLDER) + '/' + URIEncode(address + "_p" + std::to_string(port)))
{
  // prefix with cached- to avoid reserved names such as COM
  cachePrefix = cachePath + "/cached-";

  #ifndef __APPLE__
    try {
      // make sure this directory exists for caching
      std::filesystem::create_directories(cachePath);

      for (auto& entry : std::filesystem::directory_iterator(cachePath)) {
        auto path = entry.path().string();

        if (path.length() < cachePrefix.length()) {
          // delete invalid file
          std::filesystem::remove(path);
          continue;
        }

        auto [name, lastModified] = decodeName(path.substr(cachePrefix.length()));

        CacheMeta meta{
          path,
          lastModified,
          entry.file_size()
        };

        cachedAssets.emplace(name, meta);
      }
    }
    catch (std::filesystem::filesystem_error& err) {
      Logger::Log("Error occured while reading assets");
      Logger::Log(err.what());
    }
  #else 
    Logger::Log("std::filesystem not supported on Mac OSX at this time.");
  #endif
}

std::string Overworld::ServerAssetManager::GetPath(const std::string& name) {
  auto it = cachedAssets.find(name);

  if (it == cachedAssets.end()) {
    // fallback
    return cachePrefix + URIEncode(name);
  }

  return it->second.path;
}

const std::unordered_map<std::string, Overworld::ServerAssetManager::CacheMeta>& Overworld::ServerAssetManager::GetCachedAssetList() {
  return cachedAssets;
}

std::vector<char> Overworld::ServerAssetManager::LoadFromCache(const std::string& name) {
  auto meta = cachedAssets[name];

  std::vector<char> data;

  if (meta.size == 0) {
    return data;
  }

  try {
    std::ifstream fin(meta.path, std::ios::binary);
    // prevents newlines from being skipped
    fin.unsetf(std::ios::skipws);

    data.reserve(meta.size);
    data.insert(data.begin(), std::istream_iterator<char>(fin), std::istream_iterator<char>());
  }
  catch (std::ifstream::failure& e) {
    Logger::Logf("Failed to read cached data \"%s\": %s", meta.path.c_str(), e.what());
  }

  return data;
}

void Overworld::ServerAssetManager::Preload(const std::string& name) {
  auto extensionPos = name.rfind('.');

  if (extensionPos == std::string::npos) {
    PreloadText(name);
    return;
  }

  auto extension = name.substr(extensionPos);

  if (extension == ".ogg") {
    PreloadAudio(name);
  }
  else if (extension == ".png" || extension == ".bmp") {
    PreloadTexture(name);
  }
  else {
    PreloadText(name);
  }
}

void Overworld::ServerAssetManager::PreloadText(const std::string& name) {
  if (textAssets.find(name) == textAssets.end()) {
    auto data = LoadFromCache(name);
    std::string text(data.data(), data.size());
    textAssets[name] = text;
  }
}

void Overworld::ServerAssetManager::PreloadTexture(const std::string& name) {
  if (textureAssets.find(name) == textureAssets.end()) {
    auto data = LoadFromCache(name);
    auto texture = std::make_shared<sf::Texture>();
    texture->loadFromMemory(data.data(), data.size());
    textureAssets[name] = texture;
  }
}

void Overworld::ServerAssetManager::PreloadAudio(const std::string& name) {
  if (audioAssets.find(name) == audioAssets.end()) {
    auto data = LoadFromCache(name);
    auto audio = std::make_shared<sf::SoundBuffer>();
    audio->loadFromMemory(data.data(), data.size());
    audioAssets[name] = audio;
  }
}

std::string Overworld::ServerAssetManager::GetText(const std::string& name) {
  PreloadText(name);
  return textAssets[name];
}

std::shared_ptr<sf::Texture> Overworld::ServerAssetManager::GetTexture(const std::string& name) {
  PreloadTexture(name);
  return textureAssets[name];
}

std::shared_ptr<sf::SoundBuffer> Overworld::ServerAssetManager::GetAudio(const std::string& name) {
  PreloadAudio(name);
  return audioAssets[name];
}

void Overworld::ServerAssetManager::CacheAsset(const std::string& name, uint64_t lastModified, const char* data, size_t size) {
  auto path = cachePrefix + encodeName(name, lastModified);

  std::ofstream fout;
  fout.open(path, std::ios::out | std::ios::binary);

  if (!fout.is_open()) {
    Logger::Logf("Failed to cache server asset to file: %s", path.c_str());
    return;
  }

  fout.write(data, size);
  fout.close();

  CacheMeta meta{
    path,
    lastModified,
    size
  };

  cachedAssets[name] = meta;
}

void Overworld::ServerAssetManager::SetText(const std::string& name, uint64_t lastModified, const std::string& data, bool cache) {
  if (cache) {
    CacheAsset(name, lastModified, data.c_str(), data.size());
  }

  textAssets.erase(name);
  textAssets.emplace(name, data);
}

void Overworld::ServerAssetManager::SetTexture(const std::string& name, uint64_t lastModified, const char* data, size_t length, bool cache) {
  if (cache) {
    CacheAsset(name, lastModified, data, length);
  }

  auto texture = std::make_shared<sf::Texture>();
  texture->loadFromMemory(data, length);

  textureAssets.erase(name);
  textureAssets.emplace(name, texture);
}

void Overworld::ServerAssetManager::SetAudio(const std::string& name, uint64_t lastModified, const char* data, size_t length, bool cache) {
  if (cache) {
    CacheAsset(name, lastModified, data, length);
  }

  auto audio = std::make_shared<sf::SoundBuffer>();
  audio->loadFromMemory(data, length);

  audioAssets.erase(name);
  audioAssets.emplace(name, audio);
}

void Overworld::ServerAssetManager::RemoveAsset(const std::string& name) {

  #ifndef __APPLE__
    try {
      std::filesystem::remove(GetPath(name));
    }
    catch (std::filesystem::filesystem_error& err) {
      Logger::Log("Error occured while removing asset");
      Logger::Log(err.what());
    }
  #endif
  
  cachedAssets.erase(name);
}
