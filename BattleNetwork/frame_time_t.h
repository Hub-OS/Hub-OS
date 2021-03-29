#pragma once

#include "template_helpers.h"

/**
* @class frame_time_t
* @brief struct representing a single frame time. Allows for basic math operations.
*
* This struct helps keep the codebase consistent with nomenclature
* When the game refers to "5 frames of animation" clocked at 60 fps,
* that is to say "1/12th of a second of animation". When referencing material
* it's easier to discuss time in terms of frames than it is to discuss fractions of a second.
* 
* Because time-sensistive components may need millisecond precision, this struct allows
* easily operability between either fidelities of time within the same struct.
*/
struct frame_time_t {
  static const unsigned frames_per_second = 60u;

  struct milliseconds {
    long long value{};
  };

  struct seconds {
    double value{};
  };

  milliseconds milli{};

  seconds asSeconds() const {
    return seconds{ this->milli.value / 1000.0 };
  }

  milliseconds asMilli() const {
    return this->milli;
  }

  unsigned count() const {
    // returns the number of full frames in this time span
    return static_cast<unsigned>(asSeconds().value * frames_per_second);
  }

  operator seconds() const {
    return asSeconds();
  }

  operator milliseconds() const {
    return asMilli();
  }

  template<typename T>
  operator T() const;

  /// TODO: should we be comparing frames or the precision?

  frame_time_t& operator-=(const frame_time_t& other) {
    this->milli.value = this->milli.value - other.milli.value;
    return *this;
  }

  frame_time_t& operator+=(const frame_time_t& other) {
    this->milli.value = this->milli.value + other.milli.value;
    return *this;
  }

  frame_time_t operator-(const frame_time_t& other) const {
    return frame_time_t{ this->milli.value - other.milli.value };
  }

  frame_time_t operator+(const frame_time_t& other) const {
    return frame_time_t{ this->milli.value + other.milli.value };
  }

  bool operator <(const frame_time_t& other) const {
    return this->milli.value < other.milli.value;
  }

  bool operator <=(const frame_time_t& other) const {
    return this->milli.value <= other.milli.value;
  }

  bool operator >(const frame_time_t& other) const {
    return this->milli.value > other.milli.value;
  }

  bool operator >=(const frame_time_t& other) const {
    return this->milli.value >= other.milli.value;
  }

  bool operator ==(const frame_time_t& other) const {
    return this->milli.value == other.milli.value;
  }

  bool operator !=(const frame_time_t& other) const {
    return this->milli.value != other.milli.value;
  }
};

template<typename T>
static constexpr frame_time_t from_seconds(T sec) {
  // cast to high precision to type of milliseconds
  return frame_time_t{ static_cast<decltype(frame_time_t::milliseconds::value)>(static_cast<double>(sec) * 1000.0) };
}

 //!< frames utility method transforms frames to engine time
static constexpr frame_time_t frames(unsigned int frames) {
  return frame_time_t{ static_cast<long long>(1000 * (double(frames) / 60.0)) };
};

template<typename T>
const T seconds_cast(const frame_time_t& rhs) {
  return static_cast<T>(rhs.asSeconds().value);
}

template<typename T>
const T milli_cast(const frame_time_t& rhs) {
  return static_cast<T>(rhs.asMilli().value);
}

template<typename T>
const T time_cast(const frame_time_t& rhs) {
  static_assert(!is_template_parsed<T>(), "no cast available for frame_time_cast");
}

template<typename T>
frame_time_t::operator T() const {
  return time_cast<T>(*this);
}