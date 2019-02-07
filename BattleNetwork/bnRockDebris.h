#pragma once
#include "bnArtifact.h"
#include "bnField.h"

class RockDebris : public Artifact
{
public:
  enum class Type : int {
    LEFT = 1,
    RIGHT,
    LEFT_ICE,
    RIGHT_ICE
  };

private:
  Animation animation;

  sf::Sprite leftRock;
  sf::Sprite rightRock;

  double duration;
  double progress;
  double intensity;
  Type type;
public:
  RockDebris(Type type, double intensity);
  ~RockDebris();

  virtual void Update(float _elapsed);
  virtual bool Move(Direction _direction) { return false; }
  vector<Drawable*> GetMiscComponents();

};

