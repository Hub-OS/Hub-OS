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

  AUDIO.Play(AudioType::AREA_GRAB, AudioPriority::LOWEST);
  this->animationComponent = new AnimationComponent(this);
  this->RegisterComponent(this->animationComponent);
  this->animationComponent->Setup("resources/spells/areagrab.animation");
  this->animationComponent->Reload();
  this->animationComponent->SetAnimation("FALLING", Animator::Mode::Loop);
  this->animationComponent->OnUpdate(0);

  auto props = Hit::DefaultProperties;
  props.damage = 10;

  this->SetHitboxProperties(props);
}

PanelGrab::~PanelGrab() {
}

void PanelGrab::OnUpdate(float _elapsed) {
  if (this->GetTile()) {
    start = sf::Vector2f(this->tile->getPosition().x, 0);

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
      tile->SetTeam(this->GetTeam());

      // Show the panel grab spread animation
      if (this->animationComponent->GetAnimationString() != "HIT") {
        AUDIO.Play(AudioType::AREA_GRAB_TOUCHDOWN, AudioPriority::LOWEST);
        this->animationComponent->SetAnimation("HIT");
      }
    }

    if (progress > duration*2.0) {
      this->Delete();
    }

    progress += _elapsed;
  }
  else {
    this->Delete();
  }
}

bool PanelGrab::Move(Direction _direction) {
  return false;
}

void PanelGrab::Attack(Character* _entity) {
  _entity->Hit(GetHitboxProperties());
}
