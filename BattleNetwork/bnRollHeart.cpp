#include <random>
#include <time.h>

#include "bnRollHeart.h"
#include "bnParticleHeal.h"
#include "bnTile.h"
#include "bnField.h"
#include "bnPlayer.h"
#include "bnTextureResourceManager.h"
#include "bnAudioResourceManager.h"

#define RESOURCE_PATH "resources/spells/spell_heart.animation"

RollHeart::RollHeart(std::shared_ptr<Character> user, int _heal) : 
  heal(_heal), 
  user(user),
  Spell(user->GetTeam())
{
  user->Reveal();
  SetLayer(-10);

  SetPassthrough(true);

  HighlightTile(Battle::Tile::Highlight::none);

  height = 200;

  setTexture(Textures().LoadTextureFromFile("resources/spells/spell_heart.png"), true);

  animationComponent = CreateComponent<AnimationComponent>(weak_from_this());
  animationComponent->SetPath(RESOURCE_PATH);
  animationComponent->Reload();
  animationComponent->SetAnimation("HEART");
  Update(0);
}

RollHeart::~RollHeart() {
}

void RollHeart::OnUpdate(double _elapsed) {
  Entity::drawOffset.y = -height - 10.0f;
  height -= static_cast<float>(_elapsed * 300.0);
  
  if (height <= 0) height = 0;

  if (height == 0 && !spawnFX) {
    Audio().Play(AudioType::RECOVER);
    spawnFX = true;

    this->Hide();
    user->SetHealth(user->GetHealth() + heal);
    
    healFX = std::make_shared<ParticleHeal>();
    user->GetField()->AddEntity(healFX, tile->GetX(), tile->GetY());
  }
  else if (spawnFX) {
    if (healFX->WillRemoveLater()) {
      Delete();
    }
  }
}

void RollHeart::Attack(std::shared_ptr<Character> _entity) {
}

void RollHeart::OnDelete()
{
  Remove();
}
