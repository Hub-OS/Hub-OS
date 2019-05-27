#pragma once
#include <Swoosh/Segue.h>
#include <Swoosh/Ease.h>
#include <Swoosh/EmbedGLSL.h>

using namespace swoosh;

namespace {
  auto CHECKERBOARD_FRAG_SHADER = GLSL
  (
<<<<<<< HEAD
    100,
    precision highp float;
    precision highp int;
    precision highp sampler2D;

    varying vec2 vTexCoord;
    varying vec4 vColor;

=======
    110,
>>>>>>> b486e21e11627262088deae73097eaa7af56791c
    uniform sampler2D texture;
    uniform sampler2D texture2;
    uniform float progress;
    uniform float smoothness; // = 0.5
    uniform int cols;
    uniform int rows;

    float rand(vec2 co) {
      return fract(sin(dot(co.xy, vec2(12.9898, 78.233))) * 43758.5453);
    }

    void main() {
<<<<<<< HEAD
      vec2 p = vTexCoord.xy;
=======
      vec2 p = gl_TexCoord[0].xy;
>>>>>>> b486e21e11627262088deae73097eaa7af56791c
      vec2 size = vec2(cols, rows);
      float r = rand(floor(vec2(size) * p));
      float m = smoothstep(0.0, -smoothness, r - (progress * (1.0 + smoothness)));
      gl_FragColor = mix(texture2D(texture, p.xy), texture2D(texture2, p.xy), m);
    }

   );
}

template<int cols, int rows>
class CheckerboardCustom : public Segue {
private:
<<<<<<< HEAD
  sf::Texture* temp, * temp2;
  sf::Shader shader;
  bool loaded;
=======
  sf::Texture* temp;
  sf::Shader shader;
>>>>>>> b486e21e11627262088deae73097eaa7af56791c

public:
  virtual void onDraw(sf::RenderTexture& surface) {
    double elapsed = getElapsed().asMilliseconds();
    double duration = getDuration().asMilliseconds();
    double alpha = ease::linear(elapsed, duration, 1.0);

    this->drawLastActivity(surface);

    surface.display(); // flip and ready the buffer

<<<<<<< HEAD
#ifdef __ANDROID__
    if (!loaded) {
        temp = new sf::Texture(surface.getTexture()); // Make a copy of the source texture
        temp->flip(true);
    }
#else
    if(temp) delete temp;
    temp = new sf::Texture(surface.getTexture()); // Make a copy of the source texture
#endif
=======
    if (temp) delete temp;
    temp = new sf::Texture(surface.getTexture()); // Make a copy of the source texture
>>>>>>> b486e21e11627262088deae73097eaa7af56791c

    sf::Sprite sprite(*temp);

    surface.clear(sf::Color::Transparent);
    this->drawNextActivity(surface);

    surface.display(); // flip and ready the buffer

<<<<<<< HEAD
#ifdef __ANDROID__
if(!loaded) {
    temp2 = new sf::Texture(surface.getTexture()); // Make a copy of the source texture
    temp2->flip(true);
    loaded = true;
}
#else
    if(temp2) delete temp2;
    temp2 = new sf::Texture(surface.getTexture()); // Make a copy of the source texture
#endif
=======
    sf::Texture* temp2 = new sf::Texture(surface.getTexture()); // Make a copy of the source texture
>>>>>>> b486e21e11627262088deae73097eaa7af56791c

    shader.setUniform("progress", (float)alpha);
    shader.setUniform("texture2", *temp2);
    shader.setUniform("texture", *temp);

    sf::RenderStates states;
    states.shader = &shader;

    surface.draw(sprite, states);
<<<<<<< HEAD
=======

    delete temp2;
>>>>>>> b486e21e11627262088deae73097eaa7af56791c
  }

  CheckerboardCustom(sf::Time duration, Activity* last, Activity* next) : Segue(duration, last, next) {
    /* ... */
    temp = nullptr;
<<<<<<< HEAD
    loaded = false;

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
                          "}", ::CHECKERBOARD_FRAG_SHADER );
=======

    shader.loadFromMemory(::CHECKERBOARD_FRAG_SHADER, sf::Shader::Fragment);
>>>>>>> b486e21e11627262088deae73097eaa7af56791c
    shader.setUniform("cols", cols);
    shader.setUniform("rows", rows);
    shader.setUniform("smoothness", 0.09f);
  }

<<<<<<< HEAD
  virtual ~CheckerboardCustom() {
      if(temp) delete temp;
      if(temp2) delete temp2;
  }
=======
  virtual ~CheckerboardCustom() { ; }
>>>>>>> b486e21e11627262088deae73097eaa7af56791c
};

using Checkerboard = CheckerboardCustom<10, 10>;
