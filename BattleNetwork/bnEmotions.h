#pragma once
#include <cstdint>

/*! Types of emotions a Navi can show in battle */
enum class Emotion : uint8_t {
    normal = 0,
    full_synchro,
    angry,
    evil,
    anxious,
    tired,
    exhausted,
    pinch,
    focus,
    happy,

    COUNT,
};
