#include <string>
using std::to_string;

#include "bnTile.h"
#include "bnCharacter.h"
#include "bnAuraHealthUI.h"
#include "bnTextureResourceManager.h"
#include "bnAudioResourceManager.h"

AuraHealthUI::AuraHealthUI(Character* owner) : UIComponent(owner) {
   currHP = owner->GetHealth();
   this->owner = owner;
   font.setTexture(*LOAD_TEXTURE(AURA_NUMSET));
   font.setScale(2.f, 2.f);
}

AuraHealthUI::~AuraHealthUI() {
}

void AuraHealthUI::draw(sf::RenderTarget& target, sf::RenderStates states) const {
  auto this_states = states;
  this_states.transform *= this->getTransform();

  // 0 - 5 are on first row
  // Glyphs are 8x15 
  // 2nd row starts +1 pixel down

  if (currHP > 0) {
    int size = (int)(std::to_string(currHP).size());
    int hp = currHP;
    float offsetx = -(((size)*8.0f) / 2.0f)*font.getScale().x;
    int index = 0;
    while (index < size) {
      const char c = std::to_string(currHP)[index];
      int number = std::atoi(&c);
      int rowstart = 0;

      if (number > 4) {
        rowstart = 16;
      }

      int col = 8 * (number % 5);

      font.setTextureRect(sf::IntRect(col, rowstart, 8, 15));
      font.setPosition(sf::Vector2f(offsetx, -100.0f) + owner->getPosition());

      target.draw(font, this_states);
      //ENGINE.Draw(font);

      offsetx += 8.0f*font.getScale().x;
      index++;
    }
  }

  UIComponent::draw(target, states);
}

void AuraHealthUI::Update(float elapsed) {

  if (GetOwner()) {
    currHP = GetOwnerAs<Character>()->GetHealth();
  }
}
