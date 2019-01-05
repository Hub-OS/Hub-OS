#include <string>
using std::to_string;

#include "bnPlayer.h"
#include "bnField.h"
#include "bnCannon.h"
#include "bnBasicSword.h"
#include "bnTile.h"
#include "bnSelectedChipsUI.h"
#include "bnTextureResourceManager.h"
#include "bnAudioResourceManager.h"
#include "bnChip.h"
#include "bnEngine.h"

SelectedChipsUI::SelectedChipsUI(Player* _player) : ChipUsePublisher(), Component(_player)
  , player(_player) {
  chipCount = curr = 0;
  icon = sf::Sprite(*TEXTURES.GetTexture(CHIP_ICONS));
  icon.setScale(sf::Vector2f(2.f, 2.f));

  font = TEXTURES.LoadFontFromFile("resources/fonts/mmbnthick_regular.ttf");
  components.push_back(&text);
  components.push_back(&dmg);
}

SelectedChipsUI::~SelectedChipsUI(void) {
}

bool SelectedChipsUI::GetNextComponent(Drawable*& out) {
  static int i = 0;
  while (i < (int)components.size()) {
    out = components.at(i);
    i++;
    return true;
  }
  i = 0;
  return false;
}

void SelectedChipsUI::Update(float _elapsed) {
  if (player) {
 
    // TODO: Move draw out of update. Utilize components.
    int chipOrder = 0;
    for (int i = curr; i < chipCount; i++) {
      icon.setPosition(player->getPosition() + sf::Vector2f(((i - curr) * 2.0f) - 4.f, - 58.0f - 63.f - (i - curr) * -2.0f));
      sf::IntRect iconSubFrame = TEXTURES.GetIconRectFromID(selectedChips[curr]->GetIconID());
      icon.setTextureRect(iconSubFrame);
      ENGINE.Draw(icon);
    }

    if (chipCount > 0 && curr < chipCount && selectedChips[curr]) {
      text = Text(sf::String(selectedChips[curr]->GetShortName()), *font);
      text.setOrigin(0, 0);
      text.setScale(0.8f, 0.8f);
      text.setPosition(3.0f, 290.0f);
      text.setOutlineThickness(2.f);
      text.setOutlineColor(sf::Color(48, 56, 80));

      if (selectedChips[curr]->GetDamage() > 0) {
        dmg = Text(to_string(selectedChips[curr]->GetDamage()), *font);
        dmg.setOrigin(0, 0);
        dmg.setScale(0.8f, 0.8f);
        dmg.setPosition((text.getLocalBounds().width*text.getScale().x) + 13.f, 290.f);
        dmg.setFillColor(sf::Color(225, 140, 0));
        dmg.setOutlineThickness(2.f);
        dmg.setOutlineColor(sf::Color(48, 56, 80));
      }
    } else {
      text.setString("");
      dmg.setString("");
    }
  }
}

void SelectedChipsUI::LoadChips(Chip ** incoming, int size) {
  selectedChips = incoming;
  chipCount = size;
  curr = 0;
}

void SelectedChipsUI::UseNextChip() {
  if (curr >= chipCount) {
    return;
  }

  this->Broadcast(*selectedChips[curr], *player);

  curr++;
}

void SelectedChipsUI::Inject(BattleScene& scene) {
  // This player component is manually assigned and 
  // does not need injection at this time
}