#pragma once
/**
* @warning These have to be in order.
* @see ShaderResourceManager
*/

#ifdef SFML_SYSTEM_ANDROID
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
      ES2_DEFAULT_FRAG,
      ES2_DEFAULT_VERT,
      SHADER_TYPE_SIZE
    };
#else
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
      SHADER_TYPE_SIZE
    };
#endif