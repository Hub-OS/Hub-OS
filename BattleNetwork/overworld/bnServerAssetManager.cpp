#include "bnServerAssetManager.h"

bool Overworld::ServerAssetManager::HasText(const std::string& name) {
  return textAssets.find(name) != textAssets.end();
}

std::string Overworld::ServerAssetManager::GetText(const std::string& name) {
  return textAssets[name];
}

bool Overworld::ServerAssetManager::HasTexture(const std::string& name) {
  return textureAssets.find(name) != textureAssets.end();
}

std::shared_ptr<sf::Texture> Overworld::ServerAssetManager::GetTexture(const std::string& name) {
  return textureAssets[name];
}

bool Overworld::ServerAssetManager::HasAudio(const std::string& name) {
  return audioAssets.find(name) != audioAssets.end();
}

std::shared_ptr<sf::SoundBuffer> Overworld::ServerAssetManager::GetAudio(const std::string& name) {
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
