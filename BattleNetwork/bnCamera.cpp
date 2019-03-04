#include "bnCamera.h"
#include <iostream>

Camera::Camera(const sf::View& view) : focus(view)
{
  progress = 1.f;
  dest = origin = focus.getCenter();
  dur = sf::milliseconds((sf::Int32)progress);
  shakeProgress = 1.f;
  shakeDur = dur;
  init = view;
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
  progress += elapsed*1000;
  shakeProgress += elapsed*1000;

  if (sf::Time(sf::milliseconds((sf::Int32)progress)) >= dur) {
    PlaceCamera(dest);
  }
  else {
    sf::Vector2f delta = (dest - origin) * (progress / dur.asMilliseconds()) + origin;
    PlaceCamera(delta);
  }

  if (isShaking) {
    if (sf::Time(sf::milliseconds((sf::Int32)shakeProgress)) < shakeDur) {
      // Drop off to zero by end of shake
      double currStress = stress * (1 - (shakeProgress / shakeDur.asMilliseconds()));

      double factor = (-currStress) + (double)((double)(currStress - (-currStress)) * (rand() / (RAND_MAX + 1.0)));
      double factor2 = (-currStress) + (double)((double)(currStress - (-currStress)) * (rand() / (RAND_MAX + 1.0)));
      sf::Vector2f offset = sf::Vector2f((float)factor, (float)factor2);

      focus.setCenter(init.getCenter() + offset);
    }
    else {
      focus = init;
      dest = focus.getCenter();
      isShaking = false;
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

  float spriteW = sprite.getLocalBounds().width  * sprite.getScale().x;
  float spriteH = sprite.getLocalBounds().height * sprite.getScale().y;
  float spriteX = sprite.getPosition().x;
  float spriteY = sprite.getPosition().y;

  return (spriteX+spriteW >= 0 && spriteX-spriteW <= camW && spriteY+spriteH >= 0 && spriteY-spriteH <= camH);
}

void Camera::ShakeCamera(double stress, sf::Time duration)
{
  std::cout << "camera shake request" << std::endl;

  if (!isShaking) {
    init = focus;
    this->stress = stress;
    shakeDur = duration;
    shakeProgress = 0;
    isShaking = true;
  }
  else {
    std::cout << "camera already shaking" << std::endl;
  }
}

const sf::View Camera::GetView() const
{
  return focus;
}
