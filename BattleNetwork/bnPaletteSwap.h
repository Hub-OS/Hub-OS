#pragma once
#include <memory>
#include <SFML/Graphics.hpp>
#include "bnComponent.h"

class BattleSceneBase;
class Entity;

class PaletteSwap : public Component {
private:
  std::shared_ptr<sf::Texture> palette;
  std::shared_ptr<sf::Texture> base;
  sf::Shader* paletteSwap;
  bool enabled; /*!< Turn this effect on/off */
public:
  PaletteSwap(Entity* owner, std::shared_ptr<sf::Texture> base);
  ~PaletteSwap();
  void OnUpdate(double _elapsed) override;
  void Inject(BattleSceneBase&) override;
  void LoadPaletteTexture(std::string);
  void SetTexture(const std::shared_ptr<sf::Texture>& texture);
  void Revert();
  void Enable(bool enabled = true);
};
