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
  PaletteSwap(Entity* owner);
  ~PaletteSwap();
  void OnUpdate(double _elapsed) override;
  void Inject(BattleSceneBase&) override;
  void LoadPaletteTexture(std::string);
  void CopyFrom(PaletteSwap& other);
  void SetTexture(const std::shared_ptr<sf::Texture>& texture);
  void SetBase(const std::shared_ptr<sf::Texture>& base);
  void Revert();
  void Enable(bool enabled = true);
  void Apply();
  const bool IsEnabled() const;
};
