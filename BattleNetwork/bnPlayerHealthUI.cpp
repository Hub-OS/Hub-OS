#include <string>
using std::to_string;

#include "bnTile.h"
#include "bnPlayerHealthUI.h"
#include "bnTextureResourceManager.h"
#include "bnAudioResourceManager.h"

PlayerHealthUI::PlayerHealthUI(Player* _player)
  : player(_player), UIComponent(_player),
    BattleOverTrigger<Player>(_player, [this](BattleScene& scene, Player& player) { this->isBattleOver = true; }) {
  texture = TEXTURES.LoadTextureFromFile("resources/ui/img_health.png");
  sprite.setTexture(*texture);
  sprite.setPosition(3.f, 0.0f);
  sprite.setScale(2.f, 2.f);

  glyphs.setTexture(LOAD_TEXTURE(PLAYER_HP_NUMSET));
  glyphs.setScale(2.f, 2.f);

  lastHP = currHP = startHP = _player->GetHealth();
  loaded = false;
  cooldown = 0;

  isBattleOver = false;

  color = sf::Color::White;
}

PlayerHealthUI::~PlayerHealthUI() {
}

void PlayerHealthUI::draw(sf::RenderTarget& target, sf::RenderStates states) const {
  auto this_states = states;
  this_states.transform *= this->getTransform();

  // Glyphs are 8x10
  // First glyph is 0 the last is 9
  target.draw(sprite, this_states);

  int size = (int)(std::to_string(currHP).size());
  int hp = currHP;
  float offsetx = -((size)*8.0f)*glyphs.getScale().x;
  int index = 0;
  while (index < size) {
    const char c = std::to_string(currHP)[index];
    int number = std::atoi(&c);

    int col = number*8;

    glyphs.setTextureRect(sf::IntRect(col, 0, 8, 10));
    glyphs.setPosition(sf::Vector2f(offsetx-8.f, 6.0f) + sf::Vector2f(sprite.getLocalBounds().width*sprite.getScale().x, 0.f));
    glyphs.setColor(this->color);

    target.draw(glyphs, this_states);
    //ENGINE.Draw(font);

    offsetx += 8.0f*glyphs.getScale().x;
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
void PlayerHealthUI::Update(float elapsed) {
  this->BattleOverTrigger<Player>::Update(elapsed);

  if (player) {
    if (player->IsDeleted()) {
      player = nullptr;
      return;
    }

    if (!loaded) {
      lastHP = currHP = player->GetHealth();
      loaded = true;
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

    color = sf::Color::White;

    bool isBurning = false;
    bool isPoisoned = false;

    if (player->GetTile()) {
      isBurning = player->GetTile()->GetState() == TileState::LAVA;
      isBurning = isBurning && player->GetElement() != Element::FIRE;
      isBurning = isBurning && !player->HasFloatShoe();
      isPoisoned = player->GetTile()->GetState() == TileState::POISON;
    }

    if (currHP > player->GetHealth() || isBurning || isPoisoned || cooldown > 0 || player->GetHealth() <= startHP * 0.5) {
      color = sf::Color(255, 165, 0);

      if (player->GetHealth() <= startHP * 0.5 && !isBattleOver) {
        AUDIO.Play(AudioType::LOW_HP, AudioPriority::HIGH);
      }
    } else if (currHP < player->GetHealth()) {
      color = sf::Color(0, 255, 80);
    }
  }
}