#pragma once

#include <SFML/System.hpp>
#include <cmath>

inline float SquaredDistance(sf::Vector2f a, sf::Vector2f b) {
  auto delta = a - b;
  return delta.x * delta.x + delta.y * delta.y;
}

inline float Hypotenuse(sf::Vector2f a) {
  return std::sqrt(a.x * a.x + a.y * a.y);
}

inline float Distance(sf::Vector2f a, sf::Vector2f b) {
  auto delta = a - b;
  return Hypotenuse(delta);
}

inline sf::Vector2f Floor(sf::Vector2f a) {
  return {
    std::floor(a.x),
    std::floor(a.y)
  };
}

inline sf::Vector3f Floor(sf::Vector3f a) {
  return {
    std::floor(a.x),
    std::floor(a.y),
    std::floor(a.z)
  };
}

inline sf::Vector3f FloorXY(sf::Vector3f a) {
  return {
    std::floor(a.x),
    std::floor(a.y),
    a.z
  };
}

inline sf::Vector2f Round(sf::Vector2f a) {
  return {
    std::round(a.x),
    std::round(a.y)
  };
}

inline sf::Vector3f Round(sf::Vector3f a) {
  return {
    std::round(a.x),
    std::round(a.y),
    std::round(a.z)
  };
}

inline sf::Vector3f RoundXY(sf::Vector3f a) {
  return {
    std::round(a.x),
    std::round(a.y),
    a.z
  };
}

inline sf::Vector2f ToVector2f(sf::Vector3f a) {
  return { a.x, a.y };
}

// based on: http://www.cse.yorku.ca/~amana/research/grid.pdf
// lua explanation: https://theshoemaker.de/2016/02/ray-casting-in-2d-grids/
inline sf::Vector2f CastRay(sf::Vector2f start, sf::Vector2f ray, std::function<bool(sf::Vector2i tile, sf::Vector2f pos, sf::Vector2f ray)> test) {
  auto tile = sf::Vector2i(start);
  auto step = sf::Vector2i(ray.x < 0 ? -1 : 1, ray.y < 0 ? -1 : 1);

  // tMax* = the t distance to the next edge instersection
  float tMaxX = std::fabs(((float)tile.x - start.x + (float)(ray.x > 0)) / ray.x);
  float tMaxY = std::fabs(((float)tile.y - start.y + (float)(ray.y > 0)) / ray.y);

  // t = ray multiplier
  float t = 0.0f;

  auto currentPos = start;

  if (ray == sf::Vector2f()) {
    return currentPos;
  }

  do {
    bool takingXStep = tMaxX < tMaxY;

    // increase t, reset tMax*, decrease the other tMax* as it is now closer, take a step
    if (takingXStep) {
      t += tMaxX;
      tMaxY -= tMaxX;
      tMaxX = step.x / ray.x;
      tile.x += step.x;
    } else {
      t += tMaxY;
      tMaxX -= tMaxY;
      tMaxY = step.y / ray.y;
      tile.y += step.y;
    }

    currentPos.x = start.x + ray.x * t;
    currentPos.y = start.y + ray.y * t;
  } while (!test(tile, currentPos, ray));

  return currentPos;
}
