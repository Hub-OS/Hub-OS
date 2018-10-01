#pragma once
#include <SFML/Graphics.hpp>

class Camera
{
sf::Vector2f dest;
sf::Vector2f origin;
sf::Time dur;
sf::Time shakeDur;
double stress;
sf::View focus;
sf::View init;
float progress;
float shakeProgress;
bool isShaking;

public:
  Camera(const sf::View& view);
  void operator=(const Camera& rhs); // copy ctor
  ~Camera();

  void Update(float elapsed);
  void MoveCamera(sf::Vector2f destination, sf::Time duration);
  void PlaceCamera(sf::Vector2f pos);
  void OffsetCamera(sf::Vector2f offset);
  bool IsInView(sf::Sprite & sprite);
  void ShakeCamera(double stress, sf::Time duration);
  const sf::View GetView() const;
};

