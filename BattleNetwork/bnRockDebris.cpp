#include "bnTextureResourceManager.h"
#include "bnAudioResourceManager.h"
#include "bnField.h"
#include "bnRockDebris.h"

#include <Swoosh\Ease.h>

using sf::IntRect;

#define RESOURCE_PATH "resources/mobs/cube/cube.animation"

RockDebris::RockDebris(RockDebris::Type type, double intensity) : type(type), intensity(intensity), duration(0.5*intensity), progress(0)
{
  SetLayer(0);
  this->setTexture(*TEXTURES.GetTexture(TextureType::MISC_CUBE));
  this->setScale(2.f, 2.f);
  rightRock = (sf::Sprite)*this;

  //Components setup and load
  animation = Animation(RESOURCE_PATH);
  animation.Reload();
  animation.SetAnimation("DEBRIS");

  animation.SetFrame((int)type, this);
}

void RockDebris::Update(float _elapsed) {
  progress += _elapsed;

  double alpha = swoosh::ease::wideParabola(progress, duration, 1.0);
  double beta = swoosh::ease::linear(progress, duration, 1.0);

  double posX = (beta * (tile->getPosition().x-(5.0*intensity))) + ((1.0 - beta)*tile->getPosition().x);
  double height = -(alpha * 60.0 * intensity);
  double posY = height + (beta * tile->getPosition().y) + ((1.0f - beta)*tile->getPosition().y);

  if (type == RockDebris::Type::LEFT) {
    this->setPosition((float)posX, (float)posY);
  }

  posX = (beta * (tile->getPosition().x + (5.0*intensity))) + ((1.0 - beta)*tile->getPosition().x);

  if (type == RockDebris::Type::RIGHT) {
    this->setPosition((float)posX, (float)posY);
  }

  setRotation((-std::min(1.0,(progress / duration))*90.0f * intensity)*(((int)type)-2));

  // std::cout << "beta: " << beta << std::endl;

  if (progress >= duration*2.0f) {
    //progress = 0;

    //if (--intensity == 0) {
      this->Delete();
    //}
  } 

  Entity::Update(_elapsed);
}

vector<Drawable*> RockDebris::GetMiscComponents() {
  vector<Drawable*> drawables = vector<Drawable*>();

  return drawables;
}

RockDebris::~RockDebris()
{
}