#include "bnCamera.h"
#include <iostream>
#include <cmath>
#include <Swoosh/Ease.h>

Camera::Camera(const sf::View& view) : focus(view), rect(view.getSize())
{
  progress = 1.f;
  dest = origin = focus.getCenter();
  dur = sf::milliseconds((sf::Int32)progress);
  shakeProgress = 1.f;
  shakeDur = dur;
  init = focus = view;
  isShaking = false;
  startColor = sf::Color(0,0,0,0);
  fadeColor = sf::Color(0,0,0,0);
}

void Camera::operator=(const Camera& rhs) {
  progress = rhs.progress;
  dest = rhs.dest;
  dur = rhs.dur;
  focus = rhs.focus;
  shakeProgress = rhs.shakeProgress;
  shakeDur = rhs.shakeDur;
  init = rhs.init;
  isShaking = rhs.isShaking;
}

Camera::~Camera()
{
}

void Camera::Update(double elapsed) {
  // Compare as milliseconds
  double asMilli = elapsed * 1000;
  progress += asMilli;
  shakeProgress += asMilli;
  fadeProgress += asMilli;

  double x = swoosh::ease::linear(fadeProgress, (double)fadeDur.asMilliseconds(), 1.0);
  
  sf::Color fcolor(fadeColor);
  
  fcolor.r = ((1 - x) * startColor.r) + (x * fadeColor.r);
  fcolor.g = ((1 - x) * startColor.g) + (x * fadeColor.g);
  fcolor.b = ((1 - x) * startColor.b) + (x * fadeColor.b);
  fcolor.a = ((1 - x) * startColor.a) + (x * fadeColor.a);

  rect.setFillColor(fcolor);
  rect.setOutlineColor(fcolor);

  // If progress is over, update position to the dest
  if (sf::Time(sf::milliseconds((sf::Int32)progress)) >= dur) {
    PlaceCamera(dest);

    // reset wane params
    waneFactor = 1.0f;
    waning = false;
  }
  else {  
    // Otherwise calculate the delta
    if (waning) {
      double x = swoosh::ease::wane(progress, (double)dur.asMilliseconds(), waneFactor);
      sf::Vector2f delta = (dest - origin) * (float)x + origin;
      focus.setCenter(delta);
      init = focus;
    }
    else {
      sf::Vector2f delta = (dest - origin) * static_cast<float>(progress / dur.asMilliseconds()) + origin;
      focus.setCenter(delta);
      init = focus;
    }
  }

  if (isShaking) {
    if (sf::Time(sf::milliseconds((sf::Int32)shakeProgress)) <= shakeDur) {
      // Drop off to zero by end of shake
      double currStress = stress *(1 - (shakeProgress / shakeDur.asMilliseconds()));

      int randomAngle = int(shakeProgress) * (rand() % 360);
      randomAngle += (150 + (rand() % 60));

      auto offset = sf::Vector2f(std::sin((float)randomAngle) * float(currStress), std::cos((float)randomAngle) * float(currStress));

      focus.setCenter(init.getCenter() + offset);
    }
    else {
      focus = init;
      dest = focus.getCenter();
      isShaking = false;
      stress = 0;
    }
  }
}

void Camera::MoveCamera(sf::Vector2f destination, sf::Time duration) {
  origin = focus.getCenter();
  progress = 0;
  dest = destination;
  dur = duration;

}

void Camera::WaneCamera(sf::Vector2f destination, sf::Time duration, float factor)
{
  waneFactor = factor;
  waning = true;
  origin = focus.getCenter();
  progress = 0;
  dest = destination;
  dur = duration;
}

void Camera::PlaceCamera(sf::Vector2f pos) {
  focus.setCenter(pos);
  init = focus;
  dest = focus.getCenter();
}

void Camera::OffsetCamera(sf::Vector2f offset)
{
  origin = focus.getCenter();
  focus.setCenter(origin + offset);
  dest = focus.getCenter();
}

bool Camera::IsInView(sf::Sprite& sprite) {
  float camW = focus.getSize().x;
  float camH = focus.getSize().y;
  float offset_x = focus.getCenter().x - (camW * 0.5f);
  float offset_y = focus.getCenter().y - (camH * 0.5f);

  float spriteW = sprite.getLocalBounds().width  * sprite.getScale().x;
  float spriteH = sprite.getLocalBounds().height * sprite.getScale().y;
  float spriteX = sprite.getPosition().x + offset_x;
  float spriteY = sprite.getPosition().y + offset_y;

  return (spriteX >= 0 && spriteX+spriteW <= camW && spriteY >= 0 && spriteY+spriteH <= camH);
}

void Camera::ShakeCamera(double stress, sf::Time duration)
{
  if(isShaking) return;
  
  init = focus;
  Camera::stress = stress;
  shakeDur = duration;
  shakeProgress = 0;
  isShaking = true;
}

const sf::View& Camera::GetView() const
{
  return focus;
}

void Camera::FadeCamera(const sf::Color& color, sf::Time duration)
{
  startColor = rect.getFillColor();
  fadeProgress = 0;
  fadeColor = color;
  fadeDur = duration;
}

const sf::RectangleShape& Camera::GetLens()
{
  return rect;
}

sf::Vector2f Camera::GetCenterOffset(const sf::Vector2f& scale, const swoosh::Activity& activity)
{
    sf::Vector2f cameraCenter = focus.getCenter();
    cameraCenter.x = std::floor(cameraCenter.x) * scale.x;
    cameraCenter.y = std::floor(cameraCenter.y) * scale.y;
    auto offset = cameraCenter - activity.getView().getCenter();

    return offset;
}
