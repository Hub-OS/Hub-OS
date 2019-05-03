precision lowp float;
precision lowp int;

varying vec2 vTexCoord;

uniform sampler2D texture;
uniform float pixel_threshold;

void main()
{
    float factor = 1.0 / (pixel_threshold + 0.001);
    vec2 pos = floor(vTexCoord.xy * factor + 0.5) / factor;
    // gl_FragColor = texture2D(texture, pos);
}