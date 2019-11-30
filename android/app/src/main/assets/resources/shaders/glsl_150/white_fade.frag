precision lowp float;
precision lowp int;

varying vec2 vTexCoord;
varying vec4 vColor;
uniform sampler2D texture;
uniform float opacity;

void main()
{
    vec4 pixel = texture2D(texture, vTexCoord.xy);
    vec4 color = pixel;
    color = vec4(1.0, 1.0, 1.0, color.a)*opacity + (1.0-opacity)*color;
    gl_FragColor = color;
}