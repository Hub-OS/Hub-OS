#include <string>
#include <Swoosh/Ease.h>
#include "bnPlayer.h"
#include "bnField.h"
#include "bnSelectedChipsUI.h"
#include "bnTextureResourceManager.h"
#include "bnInputManager.h"
#include "bnChip.h"
#include "bnChipAction.h"

using std::to_string;

SelectedChipsUI::SelectedChipsUI(Player* _player) : ChipUsePublisher(), UIComponent(_player)
  , player(_player) {
  player->RegisterComponent(this);
  chipCount = curr = 0;
  icon = sf::Sprite(*TEXTURES.GetTexture(CHIP_ICONS));
  icon.setScale(sf::Vector2f(2.f, 2.f));

  frame = sf::Sprite(*TEXTURES.GetTexture(CHIP_FRAME));
  frame.setScale(sf::Vector2f(2.f, 2.f));

  font = TEXTURES.LoadFontFromFile("resources/fonts/mmbnthick_regular.ttf");

  interpolTimeFlat = interpolTimeDest = 0;
  interpolDur = sf::seconds(0.2f);

  firstFrame = true; // first time drawn, update positions
  spread = false;
}

SelectedChipsUI::~SelectedChipsUI() {
}

void SelectedChipsUI::draw(sf::RenderTarget & target, sf::RenderStates states) const {    
    text.setString("");
    dmg.setString("");

    if (player) {
      int chipOrder = 0;
      
      // i = curr so we only see the chips that are left
      for (int i = curr; i < chipCount; i++) {
        // The first chip appears in front 
        // But the trick is that we start at i which is a remainder of the whole list
        // We first draw the last chip in the list down to i
        int drawOrderIndex = chipCount - i + curr - 1;

        if (spread) {
          // If we spread the chips, this algorithm is simple: 
          // x = (i_drawOrderIndex - curr ) * size of chip) - spacing
          // y = Place -121 screen pixels above the user
          sf::Vector2f flat = player->getPosition() + sf::Vector2f(((drawOrderIndex - curr) * 32.0f) - 4.f, -player->GetHitHeight() - 20.f);
          
          // We want to smoothly move from above the head to the spread position
          double alpha = swoosh::ease::linear(interpolTimeFlat, (double)interpolDur.asSeconds(), 1.0);
          
          // interpolate
          icon.setPosition(((float)alpha*flat) + ((float)(1.0-alpha)*icon.getPosition()));

          interpolTimeDest = 0;
          interpolTimeFlat += this->elapsed;
        }
        else {
          // If stacked, the algorithm makes a jagged pattern that goes up and to the left:
          // x = ( ( i - curr ) * spacing ) - spacing
          // y = -121.f - (i - curr) * (-spacing )
          sf::Vector2f dest = player->getPosition() + sf::Vector2f(((i - curr) * 4.0f) - 4.f, -player->GetHitHeight() - 20.f - (i - curr) * -4.0f);
          
          // We want to smoothly move from the spread position to the stacked position
          double alpha = swoosh::ease::linear(interpolTimeDest, (double)interpolDur.asSeconds(), 1.0);
          
          if (alpha >= 1.0f) {
            if (firstFrame) {
              firstFrame = false;
            }
          }
          // interpolate
          icon.setPosition(((float)alpha*dest) + ((float)(1.0 - alpha)*icon.getPosition()));

          interpolTimeDest += this->elapsed;

          interpolTimeFlat = 0;
        }

        if (!firstFrame) {
          // The black border needs to sit 1 pixel outside of the icon
          // 1px * 2 (scale) = 2px
          frame.setPosition(icon.getPosition());
          frame.setPosition(frame.getPosition() - sf::Vector2f(2.f, 2.f));
          target.draw(frame);

          // Grab the ID of the chip and draw that icon from the spritesheet
          sf::IntRect iconSubFrame = TEXTURES.GetIconRectFromID(selectedChips[drawOrderIndex]->GetIconID());
          icon.setTextureRect(iconSubFrame);
          target.draw(icon);
        }
      }

      // If we have a valid chip, update and draw the data
      if (chipCount > 0 && curr < chipCount && selectedChips[curr]) {
          
        // Text sits at the bottom-left of the screen
        text = Text(sf::String(selectedChips[curr]->GetShortName()), *font);
        text.setOrigin(0, 0);
        text.setScale(0.8f, 0.8f);
        text.setPosition(3.0f, 290.0f);
        text.setOutlineThickness(2.f);
        text.setOutlineColor(sf::Color(48, 56, 80));

        // Text sits at the bottom-left of the screen
        sf::String dmgText = std::to_string(selectedChips[curr]->unmodDamage);
        int delta = selectedChips[curr]->damage - selectedChips[curr]->unmodDamage;

        if (selectedChips[curr]->unmodDamage != selectedChips[curr]->damage) {
          dmgText = dmgText + sf::String(" + ") + sf::String(std::to_string(delta));
        }

        // attacks that normally show no damage will show if the modifer adds damage
        if (delta != 0 || selectedChips[curr]->unmodDamage != 0) {
          dmg = Text(dmgText, *font);
          dmg.setOrigin(0, 0);
          dmg.setScale(0.8f, 0.8f);
          dmg.setPosition((text.getLocalBounds().width*text.getScale().x) + 13.f, 290.f);
          dmg.setFillColor(sf::Color(225, 140, 0));
          dmg.setOutlineThickness(2.f);
          dmg.setOutlineColor(sf::Color(48, 56, 80));
        }
      }
    }

    target.draw(text);
    target.draw(dmg);

    UIComponent::draw(target, states);
};

void SelectedChipsUI::OnUpdate(float _elapsed) {
  if (INPUT.Has(EventTypes::HELD_QUICK_OPT)) {
    spread = true;
  }
  else {
    spread = false;
  }

  this->elapsed = _elapsed;
}

void SelectedChipsUI::LoadChips(Chip ** incoming, int size) {
  selectedChips = incoming;
  chipCount = size;
  curr = 0;
}

void SelectedChipsUI::UseNextChip() {
  if (curr >= chipCount || player->GetComponentsDerivedFrom<ChipAction>().size()) {
    return;
  }

  // Broadcast to all subscribed ChipUseListeners
  this->Broadcast(*selectedChips[curr], *player);

  curr++;
}

void SelectedChipsUI::Inject(BattleScene& scene) {
  // This component is manually assigned in the battle scene 
  // and does not need injection at this time
}
