#include "bnExplosion.h"
#include "bnTextureResourceManager.h"
#include "bnAudioResourceManager.h"
#include "bnField.h"

using sf::IntRect;

Explosion::Explosion(Field* _field, Team _team, int _numOfExplosions, double _playbackSpeed) : animationComponent(this)
{
  root = this;
  SetLayer(0);
  field = _field;
  team = _team;
  numOfExplosions = _numOfExplosions;
  playbackSpeed = _playbackSpeed;
  count = 0;
  setTexture(LOAD_TEXTURE(MOB_EXPLOSION));
  setScale(2.f, 2.f);
  animationComponent.Setup("resources/mobs/mob_explosion.animation");
  animationComponent.Reload();

  int randNegX = 1;
  int randNegY = 1;
  int randX = rand() % 20;
  int randY = rand() % 20;

  if (rand() % 10 > 5) randNegX = -1;
  if (rand() % 10 > 5) randNegY = -1;

  randX *= randNegX;
  randY *= randNegY;

  offset = sf::Vector2f((float)randX, (float)randY);

  AUDIO.Play(AudioType::EXPLODE);

  animationComponent.SetAnimation("EXPLODE", [this]() { this->setColor(sf::Color(255, 255, 255, 0)); });
  animationComponent.SetPlaybackSpeed(playbackSpeed);
  animationComponent.Update(0.1f);

  if (_numOfExplosions > 0) {
    animationComponent.AddCallback(9, [this, _field, _team, _numOfExplosions]() {
      this->GetField()->OwnEntity(new Explosion(*this), this->GetTile()->GetX(), this->GetTile()->GetY());
    }, std::function<void()>(), true);
  }
}

Explosion::Explosion(const Explosion & copy) : animationComponent(this)
{
  if (copy.root == &copy)
    root = &const_cast<Explosion&>(copy);
  else
    root = copy.root;

  count = 0; // uneeded for this copy
  SetLayer(1);
  field = copy.GetField();
  team = copy.GetTeam();
  numOfExplosions = copy.numOfExplosions-1;
  playbackSpeed = copy.playbackSpeed;
  setTexture(LOAD_TEXTURE(MOB_EXPLOSION));
  setScale(2.f, 2.f);
  animationComponent.Setup("resources/mobs/mob_explosion.animation");
  animationComponent.Reload();

  int randNegX = 1;
  int randNegY = 1;
  int randX = rand() % 20;
  int randY = rand() % 20;

  if (rand() % 10 > 5) randNegX = -1;
  if (rand() % 10 > 5) randNegY = -1;

  randX *= randNegX;
  randY *= randNegY;

  offset = sf::Vector2f((float)randX, (float)randY);

  AUDIO.Play(AudioType::EXPLODE, AudioPriority::LOW);

  animationComponent.SetAnimation("EXPLODE", [this]() { this->Delete(); root->IncrementExplosionCount(); });
  animationComponent.SetPlaybackSpeed(playbackSpeed);
  animationComponent.Update(0.1f);

  if (numOfExplosions > 1) {
    animationComponent.AddCallback(9, [this]() {
      this->GetField()->OwnEntity(new Explosion(*this), this->GetTile()->GetX(), this->GetTile()->GetY());
    }, std::function<void()>(), true);
  }
}

void Explosion::Update(float _elapsed) {
  if (this == root) {
    if (count == numOfExplosions - 1) {
      Delete();
      return;
    }
  }

  animationComponent.Update(_elapsed);
  setPosition((tile->getPosition().x + offset.x), (tile->getPosition().y + offset.y));
  Entity::Update(_elapsed);
}

vector<Drawable*> Explosion::GetMiscComponents() {
  vector<Drawable*> drawables = vector<Drawable*>();
  return drawables;
}

void Explosion::IncrementExplosionCount() {
  count++;
}

Explosion::~Explosion()
{
}