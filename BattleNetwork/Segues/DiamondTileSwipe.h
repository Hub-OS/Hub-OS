#pragma once
#include <Swoosh/EmbedGLSL.h>
#include <Swoosh/Segue.h>
#include <Swoosh/Ease.h>

using namespace swoosh;

namespace {
  auto DIAMOND_SHADER = GLSL(
<<<<<<< HEAD
    100,
    precision lowp float;
    precision lowp int;

    varying vec2 vTexCoord;
    varying vec4 vColor;

    uniform sampler2D texture; // Our render texture
=======
    110,
    uniform sampler2D texture;
>>>>>>> b486e21e11627262088deae73097eaa7af56791c
    uniform float time;
    uniform int direction;

    float modulo(float a, float b) {
      return a - (b * floor(a / b));
    }

    void main() {
      // Map range time from (0.0, 1.0) to (0.5,2.25) for the equation to cover the screen
      float range = 0.50*(1.0 - time) + (2.25*time);

      //Scale the uvs to integers to scale directly with the equation.
<<<<<<< HEAD
      vec2 posI = vec2(vTexCoord.x * 40.0, vTexCoord.y * 30.0);
=======
      vec2 posI = vec2(gl_TexCoord[0].x * 40.0, gl_TexCoord[0].y * 30.0);
>>>>>>> b486e21e11627262088deae73097eaa7af56791c
      //modulo the position to clamp it to repeat the pattern.
      vec2 pos = vec2(modulo(posI.x, 2.0), modulo(posI.y, 2.0)) - vec2(1.0, 1.0);
      float size;

      if(direction == 0) {
<<<<<<< HEAD
          size = pow(range - (1.0 - vTexCoord.x), 3.0);
      } else if(direction == 1) {
          size = pow(range - vTexCoord.x, 3.0);
      } else if(direction == 2) {
          size = pow(range - (1.0 - vTexCoord.y), 3.0);
      } else if(direction == 3) {
          size = pow(range - vTexCoord.y, 3.0);
      } else {
          size = pow(range - (1.0 - vTexCoord.x), 3.0);
      }

      size = abs(size);
      vec4 outcol = texture2D(texture, vTexCoord.xy);
=======
          size = pow(range - (1.0 - gl_TexCoord[0].x), 3.0);
      } else if(direction == 1) {
          size = pow(range - gl_TexCoord[0].x, 3.0);
      } else if(direction == 2) {
          size = pow(range - (1.0 - gl_TexCoord[0].y), 3.0);
      } else if(direction == 3) {
          size = pow(range - gl_TexCoord[0].y, 3.0);
      } else {
          size = pow(range - (1.0 - gl_TexCoord[0].x), 3.0);
      }

      size = abs(size);
      vec4 outcol = texture2D(texture, gl_TexCoord[0].xy);
>>>>>>> b486e21e11627262088deae73097eaa7af56791c

      if (abs(pos.x) + abs(pos.y) < size) {
        outcol = vec4(0, 0, 0, 1);
      }

      gl_FragColor = outcol;
    }
  );
};

template<int direction>
class DiamondTileSwipe : public Segue {
private:
  sf::Texture* temp;
  sf::Shader shader;

public:
  virtual void onDraw(sf::RenderTexture& surface) {
    double elapsed = getElapsed().asMilliseconds();
    double duration = getDuration().asMilliseconds();
    double alpha = ease::wideParabola(elapsed, duration, 1.0);

    if (elapsed < duration * 0.5)
      this->drawLastActivity(surface);
    else
      this->drawNextActivity(surface);

    surface.display(); // flip and ready the buffer

    if (temp) delete temp;
<<<<<<< HEAD

    temp = new sf::Texture(surface.getTexture()); // Make a copy of the source texture
    temp->flip(true);
=======
    temp = new sf::Texture(surface.getTexture()); // Make a copy of the source texture
>>>>>>> b486e21e11627262088deae73097eaa7af56791c

    sf::Sprite sprite(*temp);

    shader.setUniform("texture", *temp);
    shader.setUniform("direction", direction);
    shader.setUniform("time", (float)alpha);

    sf::RenderStates states;
    states.shader = &shader;

    surface.draw(sprite, states);
  }

  DiamondTileSwipe(sf::Time duration, Activity* last, Activity* next) : Segue(duration, last, next) {
    /* ... */
    temp = nullptr;
<<<<<<< HEAD
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
                        "}", ::DIAMOND_SHADER);
=======
    shader.loadFromMemory(::DIAMOND_SHADER, sf::Shader::Fragment);
>>>>>>> b486e21e11627262088deae73097eaa7af56791c
  }

  virtual ~DiamondTileSwipe() { if (temp) { delete temp; } }
};
