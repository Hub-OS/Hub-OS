#version 120
#define PI 3.1415926535897932384626433832795

uniform sampler2D texture;
uniform float factor;
uniform vec4 firstColor;
uniform vec4 secondColor;

void main()
{
    vec2 pos = gl_TexCoord[0].xy;
    vec4 pixel = texture2D(texture, vec2(gl_TexCoord[0].x, gl_TexCoord[0].y));

    if(pixel.a == 0)
      discard;

    // TODO: get rid of this comparison
    if(pixel.r == pixel.g && pixel.g == pixel.b) {
      /* oscillate gradient over time */ 
      /* 0 => 1 => 0 => repeat */
      float neg_half_pi = -0.5 * PI;
      float blend = (sin(neg_half_pi + radians((factor+pixel.r)*180.0)) + 1.0) / 2.0;

      pixel = mix(firstColor, secondColor, blend);
    }

    gl_FragColor = pixel * gl_Color;
}
