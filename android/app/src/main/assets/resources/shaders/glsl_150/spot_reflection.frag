precision lowp float;
precision lowp int;

varying vec2 vTexCoord;
varying vec4 vColor;

uniform sampler2D currentTexture; // Our render texture
uniform sampler2D sceneTexture; // Our reflection

uniform float x;
uniform float y;
uniform float w;
uniform float h;
uniform vec2 textureSizeIn;
uniform float shine; // 1.0 = completely reflective. 0 = no reflection

void main()
{
    gl_FragColor = texture2D(currentTexture, vTexCoord.st);

    vec2 size = textureSizeIn;
    vec2 screenCoord = vec2(vTexCoord.s, (vTexCoord.t)) * size;

    if(screenCoord.x >= x && screenCoord.x <= x + w && screenCoord.y >= y && screenCoord.y <= y + h) {
      // vec2 distortionMapCoordinate = vTexCoord.st; NOTE: Bump mapping?

      vec2 reflectionMapCoordinate = vTexCoord.st;
      reflectionMapCoordinate.y = 1.0 - reflectionMapCoordinate.y; // flip
      reflectionMapCoordinate.y += 0.11; // offset reflection

      vec4 color1 = texture2D(currentTexture, vTexCoord.st);

      if(color1.r < 224.0/255.0 && color1.g >= 200.0/255.0 && color1.b >= 184.0/255.0) {
        vec4 color2 = texture2D(sceneTexture, reflectionMapCoordinate);

        // gl_FragColor = ( 1.0 - shine) * color1 + shine * color2;
      }
    }
}
