#pragma once
#include <memory>
#include <SFML/Graphics.hpp>
#include "bnSmartShader.h"
#include "bnComponent.h"

class BattleSceneBase;
class Entity;
class Character;

class PaletteSwap : public Component {
private:
  bool enabled{}; /*!< Turn this effect on/off */
  std::shared_ptr<sf::Texture> palette;
  std::shared_ptr<sf::Texture> base;
  SmartShader paletteSwap;
  Character* asCharacter{ nullptr };
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
