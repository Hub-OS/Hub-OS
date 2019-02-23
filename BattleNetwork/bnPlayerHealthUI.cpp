#include <string>
using std::to_string;

#include "bnTile.h"
#include "bnPlayerHealthUI.h"
#include "bnTextureResourceManager.h"
#include "bnAudioResourceManager.h"

PlayerHealthUI::PlayerHealthUI(Player* _player)
  : player(_player), 
    SceneNode(),
    BattleOverTrigger<Player>(_player, [this](BattleScene& scene, Player& player) { this->isBattleOver = true; }) {
  font = TEXTURES.LoadFontFromFile("resources/fonts/mgm_nbr_pheelbert.ttf");
  texture = TEXTURES.LoadTextureFromFile("resources/ui/img_health.png");
  sprite.setTexture(*texture);
  sprite.setPosition(3.f, 0.0f);
  sprite.setScale(2.f, 2.f);

  lastHP = currHP = startHP = _player->GetHealth();
  text = Text(to_string(currHP), *font);
  text.setLetterSpacing(4.0f);
  loaded = false;
  cooldown = 0;

  isBattleOver = false;
}

PlayerHealthUI::~PlayerHealthUI() {
}

void PlayerHealthUI::draw(sf::RenderTarget& target, sf::RenderStates states) const {
  states.transform *= this->getTransform();

  target.draw(sprite, states);
  target.draw(text, states);
  SceneNode::draw(target, states);
}

void PlayerHealthUI::Update(float elapsed) {
  this->BattleOverTrigger<Player>::Update(elapsed);

  if (player) {
    if (!loaded) {
      lastHP = currHP = player->GetHealth();
      loaded = true;
    }

    if (lastHP != player->GetHealth()) {
      if (currHP > player->GetHealth()) {
        currHP -= 1;
        cooldown = 0.5; // seconds
      } else if (currHP < player->GetHealth()) {
        currHP += 1;
      } else {
        lastHP = currHP;
      }
    }

    if (cooldown <= 0) { cooldown = 0; }
    else { cooldown -= elapsed; }

    text.setFillColor(sf::Color::White);
    text.setString(to_string(currHP));
    text.setOrigin(text.getLocalBounds().width, 0);
    text.setPosition(75.0f, -4.f);
    //text.setScale(0.8f, 0.8f);

    bool isBurning = false;
    bool isPoisoned = false;

    if (player->GetTile()) {
      isBurning = player->GetTile()->GetState() == TileState::LAVA;
      isBurning = isBurning && player->GetElement() != Element::FIRE;
      isBurning = isBurning && !player->HasFloatShoe();
      isPoisoned = player->GetTile()->GetState() == TileState::POISON;
    }

    if (currHP > player->GetHealth() || isBurning || isPoisoned || cooldown > 0 || player->GetHealth() <= startHP * 0.5) {
      text.setFillColor(sf::Color(255, 165, 0));

      if (player->GetHealth() <= startHP * 0.5 && !isBattleOver) {
        AUDIO.Play(AudioType::LOW_HP, AudioPriority::HIGH);
      }
    } else if (currHP < player->GetHealth()) {
      text.setFillColor(sf::Color(0, 255, 80));
    }

    text.setOutlineColor(sf::Color(48, 56, 80));
    text.setOutlineThickness(2.f);
  }
}

void PlayerHealthUI::OffsetPosition(const sf::Vector2f offset)
{
  //text.setPosition(offset + sf::Vector2f(75.0f, -4.f));
  //sprite.setPosition(offset + sf::Vector2f(3.f, 0.0f));
}
