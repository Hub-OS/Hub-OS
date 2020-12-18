#pragma once
#include <SFML/System.hpp>
#include "frame_time_t.h"

template<>
inline const sf::Time time_cast(const frame_time_t& rhs) {
  return sf::seconds(static_cast<float>(rhs.asSeconds().value));
}