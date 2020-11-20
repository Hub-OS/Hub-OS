#include "bnCamera.h"
#include <iostream>
#include <cmath>

Camera::Camera(const sf::View& view) : focus(view)
{
  progress = 1.f;
  dest = origin = focus.getCenter();
  dur = sf::milliseconds((sf::Int32)progress);
  shakeProgress = 1.f;
  shakeDur = dur;
  init = focus = view;
  isShaking = false;
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

void Camera::Update(float elapsed) {
  // Compare as milliseconds
  progress += elapsed*1000;
  shakeProgress += elapsed*1000;

  // If progress is over, update position to the dest
  if (sf::Time(sf::milliseconds((sf::Int32)progress)) >= dur) {
    PlaceCamera(dest);
  }
  else {  
    // Otherwise calculate the delta
    sf::Vector2f delta = (dest - origin) * (progress / dur.asMilliseconds()) + origin;
    PlaceCamera(delta);
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
  float spriteX = sprite.getPosition().x - offset_x;
  float spriteY = sprite.getPosition().y - offset_y;

  return (spriteX+spriteW >= 0 && spriteX-spriteW <= camW && spriteY+spriteH >= 0 && spriteY-spriteH <= camH);
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

const sf::View Camera::GetView() const
{
  return focus;
}
