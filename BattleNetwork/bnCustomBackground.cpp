#include "bnCustomBackground.h"
#include "bnGame.h"

CustomBackground::CustomBackground(const std::shared_ptr<Texture>& texture, const Animation& animation, sf::Vector2f velocity) :
  animation(animation),
  velocity(velocity),
  progress(0.0f),
  IBackground<CustomBackground>(texture, 240, 180)
{
  this->animation.SetAnimation("BG");
  this->animation << Animator::Mode::Loop;

  sf::Vector2u textureSize = texture->getSize();

  if (this->animation.HasAnimation("BG")) {
    auto& frameList = this->animation.GetFrameList("BG");

    if (frameList.GetFrameCount() > 0) {
      const auto& frame = frameList.GetFrame(0);
      textureSize.x = (unsigned)frame.subregion.width;
      textureSize.y = (unsigned)frame.subregion.height;
    }
  }

  FillScreen(textureSize);
}

void CustomBackground::Update(double _elapsed) {
  animation.Update(_elapsed, dummy);

  position -= velocity * static_cast<float>(_elapsed);

  if (position.x < 0) {
    position.x = 1 - std::fmod(-position.x, 1.f);
  }

  if (position.y < 0) {
    position.y = 1 - std::fmod(-position.y, 1.f);
  }

  Wrap(position);

  // Grab the subrect used in the sprite from animation Update() call
  TextureOffset(dummy.getTextureRect());
}