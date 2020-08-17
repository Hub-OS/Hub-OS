#include <string>
#include <Swoosh/Ease.h>
#include "bnWebClientMananger.h"
#include "bnPlayer.h"
#include "bnField.h"
#include "bnSelectedCardsUI.h"
#include "bnTextureResourceManager.h"
#include "bnInputManager.h"
#include "bnCurrentTime.h"
#include "bnCard.h"
#include "bnCardAction.h"

using std::to_string;

SelectedCardsUI::SelectedCardsUI(Player* _player) : CardUsePublisher(), UIComponent(_player)
  , player(_player) {
  player->RegisterComponent(this);
  cardCount = curr = 0;
  auto iconRect = sf::IntRect(0, 0, 14, 14);
  icon.setTextureRect(iconRect);
  icon.setScale(sf::Vector2f(2.f, 2.f));

  frame.setTexture(TEXTURES.GetTexture(CHIP_FRAME));
  frame.setScale(sf::Vector2f(2.f, 2.f));

  font = TEXTURES.LoadFontFromFile("resources/fonts/mmbnthick_regular.ttf");

  interpolTimeFlat = interpolTimeDest = 0;
  interpolDur = sf::seconds(0.2f);

  firstFrame = true; // first time drawn, update positions
  spread = false;
}

SelectedCardsUI::~SelectedCardsUI() {
}

void SelectedCardsUI::draw(sf::RenderTarget & target, sf::RenderStates states) const {
    text.setString("");
    dmg.setString("");
    multiplier.setString("");

    if (player) {
      int cardOrder = 0;

      // i = curr so we only see the cards that are left
      for (int i = curr; i < cardCount; i++) {
        // The first card appears in front
        // But the trick is that we start at i which is a remainder of the whole list
        // We first draw the last card in the list down to i
        int drawOrderIndex = cardCount - i + curr - 1;

        if (spread) {
          // If we spread the cards, this algorithm is simple:
          // x = (i_drawOrderIndex - curr ) * size of card) - spacing
          // y = Place -121 screen pixels above the user
          sf::Vector2f flat = player->getPosition() + sf::Vector2f(((drawOrderIndex - curr) * 32.0f) - 4.f, -player->GetHeight() - 20.f);

          // We want to smoothly move from above the head to the spread position
          double alpha = swoosh::ease::linear(interpolTimeFlat, (double)interpolDur.asSeconds(), 1.0);

          // interpolate
          icon.setPosition(((float)alpha*flat) + ((float)(1.0-alpha)*icon.getPosition()));

          interpolTimeDest = 0;
          interpolTimeFlat += elapsed;
        }
        else {
          // If stacked, the algorithm makes a jagged pattern that goes up and to the left:
          // x = ( ( i - curr ) * spacing ) - spacing
          // y = -121.f - (i - curr) * (-spacing )
          sf::Vector2f dest = player->getPosition() + sf::Vector2f(((i - curr) * 4.0f) - 4.f, -player->GetHeight() - 20.f - (i - curr) * -4.0f);

          // We want to smoothly move from the spread position to the stacked position
          double alpha = swoosh::ease::linear(interpolTimeDest, (double)interpolDur.asSeconds(), 1.0);

          if (alpha >= 1.0f) {
            if (firstFrame) {
              firstFrame = false;
            }
          }
          // interpolate
          icon.setPosition(((float)alpha*dest) + ((float)(1.0 - alpha)*icon.getPosition()));

          interpolTimeDest += elapsed;

          interpolTimeFlat = 0;
        }

        if (!firstFrame) {
          // The black border needs to sit 1 pixel outside of the icon
          // 1px * 2 (scale) = 2px
          frame.setPosition(icon.getPosition());
          frame.setPosition(frame.getPosition() - sf::Vector2f(2.f, 2.f));
          target.draw(frame);

          // Grab the ID of the card and draw that icon from the spritesheet
          icon.setTexture(WEBCLIENT.GetIconForCard(selectedCards[drawOrderIndex]->GetUUID()));

          target.draw(icon);
        }
      }

      // If we have a valid card, update and draw the data
      if (cardCount > 0 && curr < cardCount && selectedCards[curr]) {

        // Text sits at the bottom-left of the screen
        text = Text(sf::String(selectedCards[curr]->GetShortName()), *font);
        text.setOrigin(0, 0);
        text.setPosition(3.0f, 290.0f);
        text.setOutlineThickness(2.f);
        text.setOutlineColor(sf::Color(48, 56, 80));

        // Text sits at the bottom-left of the screen
        int unmodDamage = selectedCards[curr]->GetUnmoddedProps().damage;
        int delta = selectedCards[curr]->GetDamage() - unmodDamage;
        sf::String dmgText = std::to_string(unmodDamage);

        if (delta != 0) {
          dmgText = dmgText + sf::String(" + ") + sf::String(std::to_string(delta));
        }

        // attacks that normally show no damage will show if the modifer adds damage
        if (delta != 0 || unmodDamage != 0) {
          dmg = Text(dmgText, *font);
          dmg.setOrigin(0, 0);
          dmg.setPosition((text.getLocalBounds().width*text.getScale().x) + 13.f, 290.f);
          dmg.setFillColor(sf::Color(225, 140, 0));
          dmg.setOutlineThickness(2.f);
          dmg.setOutlineColor(sf::Color(48, 56, 80));
        }

        if (multiplierValue != 1) {
          // add "x N" where N is the multiplier
          std::string multStr = "x " + std::to_string(multiplierValue);
          multiplier = Text(sf::String(multStr), *font);
          multiplier.setOrigin(0, 0);
          multiplier.setPosition(dmg.getPosition().x + dmg.getLocalBounds().width + 3.0f, 290.0f);
          multiplier.setOutlineThickness(2.f);
          multiplier.setOutlineColor(sf::Color(48, 56, 80));
        }
      }
    }

    target.draw(text);
    target.draw(dmg);
    target.draw(multiplier);

    UIComponent::draw(target, states);
};

void SelectedCardsUI::OnUpdate(float _elapsed) {
  if (INPUTx.Has(EventTypes::HELD_QUICK_OPT)) {
    spread = true;
  }
  else {
    spread = false;
  }

  elapsed = _elapsed;
}

void SelectedCardsUI::LoadCards(Battle::Card ** incoming, int size) {
  selectedCards = incoming;
  cardCount = size;
  curr = 0;
}

const bool SelectedCardsUI::UseNextCard() {
  auto actions = player->GetComponentsDerivedFrom<CardAction>();
  bool hasNextCard = curr < cardCount;
  bool canUseCard = true;

  // We could be using an ability, just make sure one of these actions are not from a card
  // Cards cannot be used if another card is still active
  for (auto&& action : actions) {
    canUseCard = canUseCard && action->GetLockoutGroup() != ActionLockoutGroup::card;
  }

  canUseCard = canUseCard && hasNextCard;

  if (!canUseCard) {
    return false;
  }

  auto card = selectedCards[curr];

  card->MultiplyDamage(multiplierValue);
  multiplierValue = 1; // reset 

  // Broadcast to all subscribed CardUseListeners
  Broadcast(*card, *player, CurrentTime::AsMilli());

  return ++curr;
}

void SelectedCardsUI::Inject(BattleScene& scene) {
  // This component is manually assigned in the battle scene
  // and does not need injection at this time
}

void SelectedCardsUI::SetMultiplier(unsigned mult)
{
  multiplierValue = mult;
}
