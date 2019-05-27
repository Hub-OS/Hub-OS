#include "bnPanelGrab.h"
#include "bnTextureResourceManager.h"
#include "bnAudioResourceManager.h"
#include <cmath>
#include <Swoosh/Ease.h>
#include <Swoosh/Game.h>

PanelGrab::PanelGrab(Field* _field, Team _team, float _duration) : duration(_duration), Spell() {
  SetLayer(0);
  field = _field;
  team = _team;
  direction = Direction::NONE;
<<<<<<< HEAD
  deleted = false;
  hit = false;
  texture = TEXTURES.GetTexture(TextureType::SPELL_AREAGRAB);
  setTexture(*texture);
  swoosh::game::setOrigin(*this, 0.5, 0.8);
  setScale(2.f, 2.f);

  progress = 0.0f;
  hitHeight = 0.0f;
=======
  
  auto texture = TEXTURES.GetTexture(TextureType::SPELL_AREAGRAB);
  setTexture(*texture);
  setScale(2.f, 2.f);

  progress = 0.0f;
>>>>>>> b486e21e11627262088deae73097eaa7af56791c

  AUDIO.Play(AudioType::AREA_GRAB, AudioPriority::LOWEST);
  this->animationComponent = AnimationComponent(this);
  this->animationComponent.Setup("resources/spells/areagrab.animation");
  this->animationComponent.Reload();
  this->animationComponent.SetAnimation("FALLING", Animate::Mode::Loop);
  this->animationComponent.Update(0);

  auto props = Hit::DefaultProperties;
  props.damage = 10;
<<<<<<< HEAD
  // props.flags |= Hit::impact;
  this->SetHitboxProperties(props);
}

PanelGrab::~PanelGrab(void) {
=======

  this->SetHitboxProperties(props);
}

PanelGrab::~PanelGrab() {
>>>>>>> b486e21e11627262088deae73097eaa7af56791c
}

void PanelGrab::Update(float _elapsed) {
  this->animationComponent.Update(_elapsed);

  if (this->tile) {
    start = sf::Vector2f(this->tile->getPosition().x, 0);

    double beta = swoosh::ease::linear(progress, duration, 1.0);

<<<<<<< HEAD
=======
    // interpolate linearly from the top down to the tile position
>>>>>>> b486e21e11627262088deae73097eaa7af56791c
    double posX = (beta * tile->getPosition().x) + ((1.0f - beta)*start.x);
    double posY = (beta * tile->getPosition().y) + ((1.0f - beta)*start.y);

    setPosition((float)posX, (float)posY);

<<<<<<< HEAD
    // When at the end of the arc
    if (progress >= duration) {
      // update tile to target tile 
      tile->AffectEntities(this);
      tile->SetTeam(this->GetTeam());

=======
    // When at the end of the line
    if (progress >= duration) {
      // Deal any damage
      tile->AffectEntities(this);
      
      // Change the team
      tile->SetTeam(this->GetTeam());

      // Show the panel grab spread animation
>>>>>>> b486e21e11627262088deae73097eaa7af56791c
      if (this->animationComponent.GetAnimationString() != "HIT") {
        AUDIO.Play(AudioType::AREA_GRAB_TOUCHDOWN, AudioPriority::LOWEST);
        this->animationComponent.SetAnimation("HIT");
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
<<<<<<< HEAD
  return true;
=======
  return false;
>>>>>>> b486e21e11627262088deae73097eaa7af56791c
}

void PanelGrab::Attack(Character* _entity) {
  _entity->Hit(GetHitboxProperties());
}
