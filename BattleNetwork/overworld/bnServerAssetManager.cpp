#include "bnServerAssetManager.h"

std::string Overworld::ServerAssetManager::GetText(const std::string& name) {
  if (textAssets.find(name) == textAssets.end()) {
    return "";
  }
  return textAssets[name];
}

std::shared_ptr<sf::Texture> Overworld::ServerAssetManager::GetTexture(const std::string& name) {
  if (textureAssets.find(name) == textureAssets.end()) {
    return std::make_shared<sf::Texture>();
  }

  return textureAssets[name];
}

std::shared_ptr<sf::SoundBuffer> Overworld::ServerAssetManager::GetAudio(const std::string& name) {
  if (audioAssets.find(name) == audioAssets.end()) {
    return std::make_shared<sf::SoundBuffer>();
  }

  return audioAssets[name];
}

void Overworld::ServerAssetManager::SetText(const std::string& name, const std::string& data) {
  textAssets.erase(name);
  textAssets.emplace(name, data);
}

void Overworld::ServerAssetManager::SetTexture(const std::string& name, std::shared_ptr<sf::Texture>& texture) {
  textureAssets.erase(name);
  textureAssets.emplace(name, texture);
}

void Overworld::ServerAssetManager::SetAudio(const std::string& name, std::shared_ptr<sf::SoundBuffer>& audio) {
  audioAssets.erase(name);
  audioAssets.emplace(name, audio);
}
