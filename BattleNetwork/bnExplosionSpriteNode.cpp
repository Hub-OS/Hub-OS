#include "bnExplosionSpriteNode.h"
#include "bnTextureResourceManager.h"
#include "bnAudioResourceManager.h"
#include "bnField.h"
#include "bnTile.h"

using sf::IntRect;

const char* ANIM_PATH = "resources/mobs/mob_explosion.animation";

ExplosionSpriteNode::ExplosionSpriteNode(SceneNode* parent, int _numOfExplosions, double _playbackSpeed) : 
  SpriteProxyNode(), animation(ANIM_PATH), parent(parent)
{
  root = this;
  this->parent->AddNode(this);
  SetLayer(-1000);
  numOfExplosions = _numOfExplosions;
  playbackSpeed = _playbackSpeed;
  count = 0;
  setTexture(LOAD_TEXTURE(MOB_EXPLOSION));

  SetOffsetArea(offsetArea);

  AUDIO.Play(AudioType::EXPLODE, AudioPriority::low);

  animation.SetAnimation("EXPLODE");
  animation << [this]() {
    this->done = true;
    this->Hide();
  };
  
  animation.Refresh(getSprite());

  /*
   * On the 12th frame, increment the explosion count, and turn the first 
   * explosion transpatent.
   * 
   * If there are more explosions expected, spawn a copy on frame 8
   */
  animation << Animator::On(12, 
    [this]() {
      root->IncrementExplosionCount();
    },
    true
  );

  if (_numOfExplosions > 1) {
    animation << Animator::On(8, [this, _numOfExplosions]() {
      auto parent = GetParent();
      if (parent) {
        auto child = new ExplosionSpriteNode(*this);
        child->EnableParentShader(false);
        chain.push_back(child);
      }
    }, true);
  }

  this->EnableParentShader(false);
  Update(0);
}

ExplosionSpriteNode::ExplosionSpriteNode(const ExplosionSpriteNode& copy) 
  : SpriteProxyNode(), animation(ANIM_PATH)
{
  root = copy.root;
  parent = copy.parent;
  count = 0; // uneeded for this copy
  SetLayer(-1000);
  numOfExplosions = copy.numOfExplosions-1;
  playbackSpeed = copy.playbackSpeed;
  setTexture(LOAD_TEXTURE(MOB_EXPLOSION));

  parent->AddNode(this);

  animation.SetAnimation("EXPLODE");
  animation << [this]() {
    this->done = true;
    this->Hide();
  };

  animation.Refresh(getSprite());

  SetOffsetArea(copy.offsetArea);

  AUDIO.Play(AudioType::EXPLODE, AudioPriority::low);

  /**
   * Tell root to increment explosion count on frame 12
   * 
   * Similar to the root constructor, if there are more explosions
   * Spawn a copy on frame 8
   */
  animation << Animator::On(12, [this]() {
    root->IncrementExplosionCount();
  }, true);

  if (numOfExplosions > 1) {
    animation << Animator::On(8, [this]() {
      auto child = new ExplosionSpriteNode(*this);
      child->EnableParentShader(false);
      root->chain.push_back(child);
    }, true);
  }
  else {
    // Last explosion happens behind entities
    SetLayer(1000); // ensure bottom draw
  }

  Update(0);
}

void ExplosionSpriteNode::Update(float _elapsed) {
  animation.Update(_elapsed*playbackSpeed, getSprite());


  /*
   * Keep root alive until all explosions are completed, then delete root
   */
  if (this == root) {
    for (auto element : chain) {
      element->Update(_elapsed);
    }

    if (count == numOfExplosions) {
      return;
    }
  }

  if(numOfExplosions != 1) {
    setPosition(offset.x, offset.y);
  } else {
    setPosition(0,0);
  }
}

void ExplosionSpriteNode::IncrementExplosionCount() {
  count++;
}

void ExplosionSpriteNode::SetOffsetArea(sf::Vector2f area)
{
  if ((int)area.x == 0) area.x = 1;
  if ((int)area.y == 0) area.y = 1;

  offsetArea = area;

  int randX = rand() % (int)(area.x+0.5f);
  int randY = rand() % (int)(area.y+0.5f);

  int randNegX = 1;
  int randNegY = 1;

  if (rand() % 10 > 5) randNegX = -1;
  if (rand() % 10 > 5) randNegY = -1;

  randX *= randNegX;
  randY = -randY;

  offset = sf::Vector2f((float)randX, (float)randY);
}

const bool ExplosionSpriteNode::IsSequenceComplete() const
{
  bool done = this->done;

  for (auto element : chain) {
    done = done && element->done;
  }

  return done;
}

std::vector<ExplosionSpriteNode*> ExplosionSpriteNode::GetChain()
{
  return chain;
}

ExplosionSpriteNode::~ExplosionSpriteNode()
{
}