#include <string>
using std::to_string;

#include <Swoosh/Game.h>
#include "bnBattleScene.h"
#include "bnMobHealthUI.h"
#include "bnCharacter.h"
#include "bnTextureResourceManager.h"
#include "bnLogger.h"

MobHealthUI::MobHealthUI(Character* _mob)
  : mob(_mob), UIComponent(_mob) {
  font = TEXTURES.LoadFontFromFile("resources/fonts/mgm_nbr_pheelbert.ttf");
  text.setFont(*font);
  text.setOutlineColor(sf::Color(48,56,80));
  text.setOutlineThickness(2.f);
  text.setScale(1.f, 0.8f);
  text.setLetterSpacing(3.0f);
  healthCounter = mob->GetHealth();
  loaded = false;
  cooldown = 0;
}

MobHealthUI::~MobHealthUI(void) {
  delete font;
}

void MobHealthUI::Update(float elapsed) {
  if (mob) {
    if (!loaded) {
      healthCounter = mob->GetHealth();
      loaded = true;
    }

    setOrigin(text.getLocalBounds().width/2.0f, 0);

    if (mob->GetHealth() <= 0) {
      text.setString("");
      return;
    }

    if (cooldown <= 0) { cooldown = 0; }
    else { cooldown -= elapsed; }
   
    if (healthCounter > mob->GetHealth()) {
      healthCounter--;
      cooldown = 0.5; //seconds
    }
    else if (healthCounter < mob->GetHealth()) {
      healthCounter++;
      text.setFillColor(sf::Color(0, 255, 80));
    }
    else {
      text.setFillColor(sf::Color::White);
    }

    if (cooldown > 0) {
      text.setFillColor(sf::Color(255, 165, 0));
    }

    text.setString(to_string(healthCounter));
    swoosh::game::setOrigin(text, 0.0f, 0.0f);

    setPosition(mob->getPosition().x, mob->getPosition().y);
  }
}

void MobHealthUI::Inject(BattleScene & scene)
{
  // Todo: add freedome to inject step? It's manadatory. No sense repeating this every time
  GetOwner()->FreeComponentByID(this->GetID()); // We are owned by the scene now 
  scene.Inject(*this);
}

void MobHealthUI::draw(sf::RenderTarget & target, sf::RenderStates states) const
{
  sf::RenderStates this_states = states.transform * this->getTransform();
  target.draw(text, this_states);
  UIComponent::draw(target, states);
}
