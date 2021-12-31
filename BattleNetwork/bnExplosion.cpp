#include "bnExplosion.h"
#include "bnTextureResourceManager.h"
#include "bnAudioResourceManager.h"
#include "bnField.h"
#include "bnTile.h"
#include "bnRandom.h"

using sf::IntRect;

Explosion::Explosion(int _numOfExplosions, double _playbackSpeed) : 
  Artifact()
{
  root = this;
  SetLayer(-1000);
  numOfExplosions = _numOfExplosions;
  playbackSpeed = _playbackSpeed;
  count = 0;
  setTexture(Textures().LoadFromFile(TexturePaths::MOB_EXPLOSION));
  setScale(2.f, 2.f);
}

Explosion::Explosion(const Explosion & copy) : Artifact()
{
  root = copy.root;

  count = 0; // uneeded for this copy
  SetLayer(-1000);
  field = copy.GetField();
  team = copy.GetTeam();
  numOfExplosions = copy.numOfExplosions-1;
  playbackSpeed = copy.playbackSpeed;
  setTexture(Textures().LoadFromFile(TexturePaths::MOB_EXPLOSION));
  setScale(2.f, 2.f);

  SetOffsetArea(copy.offsetArea);
}

void Explosion::Init() {
  Artifact::Init();

  animationComponent = CreateComponent<AnimationComponent>(weak_from_this());
  animationComponent->SetPath("resources/scenes/battle/mob_explosion.animation");
  animationComponent->Reload();

  Audio().Play(AudioType::EXPLODE, AudioPriority::low);

  animationComponent->SetAnimation("EXPLODE");
  animationComponent->SetPlaybackSpeed(playbackSpeed);
  animationComponent->OnUpdate(0.0f);

  /**
   * Tell root to increment explosion count on frame 12
   * 
   * Similar to the root constructor, if there are more explosions
   * Spawn a copy on frame 8
   */
  animationComponent->AddCallback(12, [this]() {
    root->IncrementExplosionCount();

    if(this == root) {
      setColor(sf::Color(0, 0, 0, 0));
    } else {
      Delete();
    }
  }, true);

  if (numOfExplosions > 1) {
    animationComponent->AddCallback(8, [this]() {
      GetField()->AddEntity(std::shared_ptr<Explosion>(new Explosion(*this)), *GetTile());
    }, true);
  }
  else {
    // Last explosion happens behind entities
    SetLayer(1000); // ensure bottom draw
  }
}

void Explosion::OnUpdate(double _elapsed) {
  /*
   * Keep root alive until all explosions are completed, then delete root
   */
  if (this == root) {
    if (count == numOfExplosions) {
      Delete();
      return;
    }
  }

  // The first explosion spawns inside of the entity 
  // all other explosions use the offset to explode around the entity
  if (numOfExplosions != 1) {
    Entity::drawOffset += {offset.x, offset.y};
  }
}

void Explosion::OnDelete()
{
  Erase();
}

void Explosion::OnSpawn(Battle::Tile& start)
{
  OnUpdate(0); // refresh and position explosion graphic
}

void Explosion::IncrementExplosionCount() {
  count++;
}

void Explosion::SetOffsetArea(sf::Vector2f area)
{
  if ((int)area.x == 0) area.x = 1;
  if ((int)area.y == 0) area.y = 1;

  offsetArea = area;

  int randX = SyncedRand() % (int)(area.x+0.5f);
  int randY = SyncedRand() % (int)(area.y+0.5f);

  int randNegX = 1;
  int randNegY = 1;

  if (SyncedRand() % 10 > 5) randNegX = -1;
  if (SyncedRand() % 10 > 5) randNegY = -1;

  randX *= randNegX;
  randY = -randY;

  offset = sf::Vector2f((float)randX, (float)randY);
}

Explosion::~Explosion()
{
}