#include "bnPanelGrab.h"
#include "bnTextureResourceManager.h"
#include "bnAudioResourceManager.h"
#include <cmath>
#include <Swoosh/Ease.h>
#include <Swoosh/Game.h>

PanelGrab::PanelGrab(Field* _field, Team _team, float _duration) : duration(_duration), Spell(_field, _team) {
  SetLayer(0);
  
  setTexture(TEXTURES.GetTexture(TextureType::SPELL_AREAGRAB));
  setScale(2.f, 2.f);

  progress = 0.0f;

  AUDIO.Play(AudioType::AREA_GRAB, AudioPriority::lowest);
  animationComponent = CreateComponent<AnimationComponent>(this);
  animationComponent->SetPath("resources/spells/areagrab.animation");
  animationComponent->Reload();
  animationComponent->SetAnimation("FALLING", Animator::Mode::Loop);
  animationComponent->OnUpdate(0);

  auto props = Hit::DefaultProperties;
  props.damage = 10;

  SetHitboxProperties(props);
}

PanelGrab::~PanelGrab() {
}

void PanelGrab::OnSpawn(Battle::Tile& start)
{
  auto startPos = sf::Vector2f(start.getPosition().x, 0);
  setPosition(startPos);
}

void PanelGrab::OnUpdate(float _elapsed) {
  if (GetTile()) {
    start = sf::Vector2f(tile->getPosition().x, 0);

    double beta = swoosh::ease::linear(progress, duration, 1.0);

    // interpolate linearly from the top down to the tile position
    double posX = (beta * tile->getPosition().x) + ((1.0f - beta)*start.x);
    double posY = (beta * tile->getPosition().y) + ((1.0f - beta)*start.y);

    setPosition((float)posX, (float)posY);

    // When at the end of the line
    if (progress >= duration) {
      // Deal any damage
      tile->AffectEntities(this);
      
      // Change the team
      tile->SetTeam(GetTeam());

      // Show the panel grab spread animation
      if (animationComponent->GetAnimationString() != "HIT") {
        AUDIO.Play(AudioType::AREA_GRAB_TOUCHDOWN, AudioPriority::lowest);
        animationComponent->SetAnimation("HIT");
      }
    }

    if (progress > duration*2.0) {
      Delete();
    }

    progress += _elapsed;
  }
  else {
    Delete();
  }
}

bool PanelGrab::Move(Direction _direction) {
  return false;
}

void PanelGrab::Attack(Character* _entity) {
  _entity->Hit(GetHitboxProperties());
}

void PanelGrab::OnDelete()
{
  Remove();
}
