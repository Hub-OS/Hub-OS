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
  animationComponent = CreateComponent<AnimationComponent>(weak_from_this());
  animationComponent->SetPath("resources/spells/areagrab.animation");
  animationComponent->Reload();
  animationComponent->SetAnimation("FALLING", Animator::Mode::Loop);
  animationComponent->OnUpdate(0);

  auto props = Hit::DefaultProperties;
  props.damage = 10;

  SetHitboxProperties(props);

  Entity::drawOffset = {0, -480};
}

PanelGrab::~PanelGrab() {
}

void PanelGrab::OnSpawn(Battle::Tile& start)
{
  auto startPos = sf::Vector2f(start.getPosition().x, 0);
  setPosition(startPos);
}

void PanelGrab::OnUpdate(double _elapsed) {
  start = sf::Vector2f(0, -480);

  double beta = swoosh::ease::linear(progress, duration, 1.0);

  // interpolate linearly from the top down to the tile position
  double posX = (1.0f - beta)*start.x;
  double posY = (1.0f - beta)*start.y;

  Entity::drawOffset = { (float)posX, (float)posY };

  // When at the end of the line
  if (progress >= duration) {
    // Deal any damage
    tile->AffectEntities(*this);
      
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

void PanelGrab::Attack(std::shared_ptr<Character> _entity) {
  _entity->Hit(GetHitboxProperties());
}

void PanelGrab::OnDelete()
{
  Remove();
}
