#include <string>
using std::to_string;

#include "bnTile.h"
#include "bnPlayerHealthUI.h"
#include "bnTextureResourceManager.h"
#include "bnAudioResourceManager.h"
#include "battlescene/bnBattleSceneBase.h"

PlayerHealthUI::PlayerHealthUI(Player* _player)
  : player(_player), UIComponent(_player)
{
  ResourceHandle handle;

  // TODO: move this to the preloaded textures      
  texture = handle.Textures().LoadTextureFromFile("resources/ui/img_health.png");
  uibox.setTexture(texture);
  uibox.setPosition(3.f, 0.0f);
  uibox.setScale(2.f, 2.f);

  glyphs.setTexture(handle.Textures().GetTexture(TextureType::PLAYER_HP_NUMSET));
  glyphs.setScale(2.f, 2.f);

  lastHP = currHP = startHP = _player->GetHealth();

  cooldown = 0;

  isBattleOver = false;
  color = Color::normal;

  SetDrawOnUIPass(false);
}

PlayerHealthUI::~PlayerHealthUI() {
}

void PlayerHealthUI::Inject(BattleSceneBase& scene)
{
  scene.Inject(this);
}

void PlayerHealthUI::draw(sf::RenderTarget& target, sf::RenderStates states) const {
  if (this->IsHidden()) return;

  auto this_states = states;
  this_states.transform *= getTransform();

  // Glyphs are 8x11
  // First glyph is 0 the last is 9
  // 1px space between colors
  target.draw(uibox, this_states);

  const std::string currHPStr = std::to_string(currHP);

  int size = (int)(currHPStr.size());
  int hp = currHP;
  float offsetx = -((size)*8.0f)*glyphs.getScale().x;
  int index = 0;
  while (index < size) {
    const std::string digitStr = std::string(1, currHPStr[index]);
    const char* digit = digitStr.c_str();
    int number = std::atoi(digit);

    int col = number*8;
    int row = 0;

    if (color == Color::orange) {
      row = 11;
    }
    else if (color == Color::green) {
      row = 22;
    }

    auto glyphIcon = sf::IntRect(col, row, 8, 11);
    glyphs.setTextureRect(glyphIcon);
    glyphs.setPosition(sf::Vector2f(offsetx-8.f, 6.0f) + sf::Vector2f(uibox.getSprite().getLocalBounds().width*uibox.getScale().x, 0.f));

    // Draw using transforms from parent so we can attach this to the card cust
    target.draw(glyphs, this_states);

    offsetx += 8.0f*glyphs.getScale().x;

    // Move onto the next number
    index++;
  }

  UIComponent::draw(target, states);
}

/*
HP drop is not 1 unit per frame. It is:
10 per frame if difference is 100 or more
~5 per frame if difference is 99-40 range
-3 per frame for anything lower
*/
void PlayerHealthUI::OnUpdate(double elapsed) {
  // if battle is ongoing and valid, play high pitch sound when hp is low
  isBattleOver = this->Injected()? this->Scene()->IsCleared() : true;

  if (player) {
    if (player->WillRemoveLater()) {
      player->FreeComponentByID(GetID());
      player = nullptr;
      return;
    }

    if (lastHP != player->GetHealth()) {
      if (currHP > player->GetHealth()) {
        int diff = currHP - player->GetHealth();

        if (diff >= 100) {
          currHP -= 10;
        }
        else if (diff >= 40) {
          currHP -= 5;
        }
        else if (diff >= 3) {
          currHP -= 3;
        }
        else {
          currHP--;
        }

        cooldown = 0.5; // seconds
      } else if (currHP < player->GetHealth()) {
        int diff = player->GetHealth() - currHP;

        if (diff >= 100) {
          currHP += 10;
        }
        else if (diff >= 40) {
          currHP += 5;
        }
        else if (diff >= 3) {
          currHP += 3;
        }
        else {
          currHP++;
        }
      } else {
        lastHP = currHP;
      }
    }

    if (cooldown <= 0) { cooldown = 0; }
    else { cooldown -= elapsed; }

    color = Color::normal;

    bool isBurning = false;
    bool isPoisoned = false;

    // If the player is burning or poisoned, turn red to alert them
    if (player->GetTile() && !(player->HasAirShoe() || player->HasFloatShoe())) {
      isBurning = player->GetTile()->GetState() == TileState::lava;
      isBurning = isBurning && player->GetElement() != Element::fire;
      isBurning = isBurning && !player->HasFloatShoe();
      isPoisoned = player->GetTile()->GetState() == TileState::poison;
    }

    if (currHP > player->GetHealth() || isBurning || isPoisoned || cooldown > 0 || player->GetHealth() <= startHP * 0.5) {
      color = Color::orange;

      // If HP is low, play beep with high priority
      if (player->GetHealth() <= startHP * 0.5 && !isBattleOver) {
        ResourceHandle().Audio().Play(AudioType::LOW_HP, AudioPriority::high);
      }
    } else if (currHP < player->GetHealth()) {
      color = Color::green;
    }
  }
}
