#include <string>
using std::to_string;

#include "bnTile.h"
#include "bnPlayerHealthUI.h"
#include "bnTextureResourceManager.h"
#include "bnAudioResourceManager.h"
#include "battlescene/bnBattleSceneBase.h"

PlayerHealthUI::PlayerHealthUI(std::weak_ptr<Player> _player) :
  UIComponent(_player),
  glyphs(Font::Style::gradient)
{
  ResourceHandle handle;

  texture = handle.Textures().LoadTextureFromFile("resources/ui/img_health.png");
  uibox.setTexture(texture);
  uibox.setPosition(3.f, 0.0f);
  uibox.setScale(2.f, 2.f);

  glyphs.setScale(2.f, 2.f);
  glyphs.setPosition((uibox.getLocalBounds().width*2.f) - 8.f, 4.f);

  lastHP = currHP = startHP = _player.lock()->GetHealth();

  cooldown = 0;

  isBattleOver = false;

  SetDrawOnUIPass(false);
  OnUpdate(0); // refresh and prepare for the 1st frame
}

PlayerHealthUI::~PlayerHealthUI() {
  this->Eject();
}

void PlayerHealthUI::Inject(BattleSceneBase& scene)
{
  scene.Inject(this);
}

void PlayerHealthUI::draw(sf::RenderTarget& target, sf::RenderStates states) const {
  if (this->IsHidden()) return;

  auto this_states = states;
  this_states.transform *= getTransform();

  target.draw(this->uibox, states);

  // Draw using transforms from parent so we can attach this to the card cust
  target.draw(glyphs, this_states);

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

  if (auto player = GetOwnerAs<Player>()) {
    if (player->WillRemoveLater()) {
      this->Eject();
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

    glyphs.SetFont(Font::Style::gradient);

    bool isBurning = false;
    bool isPoisoned = false;

    // If the player is burning or poisoned, turn red to alert them
    if (player->GetTile() && !(player->HasAirShoe() || player->HasFloatShoe())) {
      isBurning = player->GetTile()->GetState() == TileState::lava;
      isBurning = isBurning && player->GetElement() != Element::fire;
      isBurning = isBurning && !player->HasFloatShoe();
      isPoisoned = player->GetTile()->GetState() == TileState::poison;
    }

    if (currHP > player->GetHealth() || isBurning || isPoisoned || cooldown > 0 || player->GetHealth() <= startHP * 0.25) {
      glyphs.SetFont(Font::Style::gradient_gold);


      // If HP is low, play beep with high priority
      if (player->GetHealth() <= startHP * 0.25 && !isBattleOver) {
        ResourceHandle().Audio().Play(AudioType::LOW_HP, AudioPriority::high);
      }
    } else if (currHP < player->GetHealth()) {
      glyphs.SetFont(Font::Style::gradient_green);
    }
  }

  glyphs.SetString(std::to_string(currHP));
  glyphs.setOrigin(glyphs.GetLocalBounds().width, 0.f);
}
