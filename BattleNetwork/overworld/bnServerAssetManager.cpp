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

void Overworld::ServerAssetManager::EmplaceText(const std::string& name, const std::string& data) {
  textAssets.emplace(name, data);
}

void Overworld::ServerAssetManager::EmplaceTexture(const std::string& name, std::shared_ptr<sf::Texture>& texture) {
  textureAssets.emplace(name, texture);
}

void Overworld::ServerAssetManager::EmplaceAudio(const std::string& name, std::shared_ptr<sf::SoundBuffer>& audio) {
  audioAssets.emplace(name, audio);
}
