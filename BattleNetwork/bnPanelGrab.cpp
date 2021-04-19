#include "bnPanelGrab.h"
#include "bnTextureResourceManager.h"
#include "bnAudioResourceManager.h"
#include <cmath>
#include <Swoosh/Ease.h>
#include <Swoosh/Game.h>

PanelGrab::PanelGrab(Team _team, Direction facing, float _duration) : duration(_duration), Spell(_team) {
  SetLayer(0);
  SetFacing(facing);
  setTexture(Textures().GetTexture(TextureType::SPELL_AREAGRAB));
  setScale(2.f, 2.f);

  progress = 0.0f;

  Audio().Play(AudioType::AREA_GRAB, AudioPriority::lowest);
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
  // TODO: tile movement AdoptTile() resets the position of this entity
  // movement API should be more mature so until then, just hide the panel grab sprite
  // so the Update() can move it above the grid appropriately
  //auto startPos = sf::Vector2f(start.getPosition().x, 0);
  //setPosition(startPos);
  Hide();
}

void PanelGrab::OnUpdate(double _elapsed) {
  if (GetTile()) {
    if (IsHidden()) Reveal();

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
      tile->SetFacing(GetFacing());

      // Show the panel grab spread animation
      if (animationComponent->GetAnimationString() != "HIT") {
        Audio().Play(AudioType::AREA_GRAB_TOUCHDOWN, AudioPriority::lowest);
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

void PanelGrab::Attack(Character* _entity) {
  _entity->Hit(GetHitboxProperties());
}

void PanelGrab::OnDelete()
{
  Remove();
}
