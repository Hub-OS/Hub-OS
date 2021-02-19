#pragma once

#include <SFML/Graphics/Texture.hpp>
#include <SFML/Audio/SoundBuffer.hpp>
#include <memory>

namespace Overworld {
  class ServerAssetManager {
  private:
    std::unordered_map<std::string, std::string> textAssets;
    std::unordered_map<std::string, std::shared_ptr<sf::Texture>> textureAssets;
    std::unordered_map<std::string, std::shared_ptr<sf::SoundBuffer>> audioAssets;
  public:
    std::string GetText(const std::string& name);
    std::shared_ptr<sf::Texture> GetTexture(const std::string& name);
    std::shared_ptr<sf::SoundBuffer> GetAudio(const std::string& name);

    void SetText(const std::string& name, const std::string& data);
    void SetTexture(const std::string& name, std::shared_ptr<sf::Texture>& texture);
    void SetAudio(const std::string& name, std::shared_ptr<sf::SoundBuffer>& audio);
  };
}
