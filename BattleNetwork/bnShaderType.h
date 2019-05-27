#pragma once
/**
* \brief List of all shaders used in game
* 
* To request the size of the list used ShaderType::SHADER_TYPE_SIZE
* 
* @warning These have to be in order.
* @see ShaderResourceManager
*/
<<<<<<< HEAD

#include <SFML/Config.hpp>

#ifdef SFML_SYSTEM_ANDROID
    enum class ShaderType : int {
        DEFAULT = 0,
        BLACK_FADE,
        CUSTOM_BAR,
        GREYSCALE,
        OUTLINE,
        PIXEL_BLUR,
        TEXEL_PIXEL_BLUR,
        TEXEL_TEXTURE_WRAP,
        WHITE,
        WHITE_FADE,
        YELLOW,
        DISTORTION,
        SPOT_DISTORTION,
        SPOT_REFLECTION,
        TRANSITION,
        CHIP_REVEAL,
        BADGE_WIRE,
        SHADER_TYPE_SIZE
    };
#else
    enum class ShaderType : int {
        BLACK_FADE,
        CUSTOM_BAR,
        GREYSCALE,
        OUTLINE,
        PIXEL_BLUR,
        TEXEL_PIXEL_BLUR,
        TEXEL_TEXTURE_WRAP,
        WHITE,
        WHITE_FADE,
        YELLOW,
        DISTORTION,
        SPOT_DISTORTION,
        SPOT_REFLECTION,
        TRANSITION,
        CHIP_REVEAL,
        BADGE_WIRE,
        SHADER_TYPE_SIZE
    };
#endif
=======
enum ShaderType {
  BLACK_FADE,
  CUSTOM_BAR,
  GREYSCALE,
  OUTLINE,
  PIXEL_BLUR,
  TEXEL_PIXEL_BLUR,
  TEXEL_TEXTURE_WRAP,
  WHITE,
  WHITE_FADE, 
  YELLOW,
  DISTORTION,
  SPOT_DISTORTION,
  SPOT_REFLECTION,
  TRANSITION,
  CHIP_REVEAL,
  BADGE_WIRE,
  SHADER_TYPE_SIZE
};
>>>>>>> b486e21e11627262088deae73097eaa7af56791c
