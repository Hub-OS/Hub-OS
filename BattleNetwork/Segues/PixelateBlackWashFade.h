#pragma once
#include <Swoosh/Segue.h>
#include <Swoosh/Ease.h>
#include <Swoosh/EmbedGLSL.h>

using namespace swoosh;

namespace {
  auto PIXELATE_SHADER = GLSL
  (
    100,
    precision lowp float;
    precision lowp int;

    varying vec2 vTexCoord;
    varying vec4 vColor;

    uniform sampler2D texture; // Our render texture
    uniform float pixel_threshold;

    void main()
    {
      float factor = 1.0 / (pixel_threshold + 0.001);
      vec2 pos = floor(vTexCoord.xy * factor + 0.5) / factor;
      gl_FragColor = texture2D(texture, pos) * vColor;
    }
  );
}

class PixelateBlackWashFade : public Segue {
private:
  sf::Shader shader;
  float factor;
  bool loaded;
  bool nextScreen;

  sf::Texture* temp;
public:
  virtual void onDraw(sf::RenderTexture& surface) {
    double elapsed = getElapsed().asMilliseconds();
    double duration = getDuration().asMilliseconds();
    double alpha = ease::wideParabola(elapsed, duration, 1.0);

    if (elapsed <= duration * 0.5)
      this->drawLastActivity(surface);
    else {
        if(loaded && !nextScreen) {
            loaded = false;
            nextScreen = true;
        }

        this->drawNextActivity(surface);
    }

    surface.display();

#ifdef __ANDROID__
    if(!loaded) {
        temp = new sf::Texture(surface.getTexture()); // Make a copy of the source texture
        temp->flip(true);
        loaded = true;
    }
#else
      temp = new sf::Texture(surface.getTexture()); // Make a copy of the source texture
#endif

    sf::Sprite sprite(*temp);

    shader.setUniform("texture", *temp);
    shader.setUniform("pixel_threshold", (float)alpha/15.0f);
    sf::RenderStates states;
    states.shader = &shader;

    surface.clear(sf::Color::Transparent);
    surface.draw(sprite, states);

    // 10% of segue is a pixelate before darkening
    double delay = (duration / 10.0);
    if (elapsed > delay) {
      double alpha = ease::wideParabola(elapsed - delay, duration - delay, 1.0);

      sf::RectangleShape whiteout;
      whiteout.setSize(sf::Vector2f((float)surface.getTexture().getSize().x, (float)surface.getTexture().getSize().y));
      whiteout.setFillColor(sf::Color(0, 0, 0, (sf::Uint8)(alpha * (double)255)));
      surface.draw(whiteout);
    }
  }

  PixelateBlackWashFade(sf::Time duration, Activity* last, Activity* next) : Segue(duration, last, next) {
      loaded = nextScreen = false;

    shader.loadFromMemory("uniform mat4 viewMatrix;\n"
                         "uniform mat4 projMatrix;\n"
                         "uniform mat4 textMatrix;\n"
                         " \n"
                         "attribute vec2 position;\n"
                         "attribute vec4 color;\n"
                         "attribute vec2 texCoord;\n"
                         "\n"
                         "varying vec4 vColor;\n"
                         "varying vec2 vTexCoord;\n"
                         " \n"
                         "void main()\n"
                         "{\n"
                         "    gl_Position = projMatrix * viewMatrix * vec4(position, 0.0, 1.0);\n"
                         "    vColor = color;\n"
                         "    vTexCoord = (textMatrix * vec4(texCoord.xy, 0.0, 1.0)).xy;\n"
                         "}",::PIXELATE_SHADER);
  }

  virtual ~PixelateBlackWashFade() {
#ifdef __ANDROID__
      if(temp) {
          delete temp;
      }
#endif
  }
};
