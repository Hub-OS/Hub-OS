precision lowp float;
precision lowp int;

varying vec4 vColor;
varying vec2 vTexCoord;

uniform sampler2D texture;
uniform float resolutionW;
uniform float resolutionH;

void main(void)
{
    float dx = 1.0 / resolutionW;
    float dy = 1.0 / resolutionH;

    vec2 pos = vTexCoord.xy;
    vec4 incolor = texture2D(texture, pos).rgba;

    // this just checks for alpha
    vec4 top = texture2D(texture, vec2(pos.x, pos.y - dy));
    vec4 left = texture2D(texture, vec2(pos.x - dx, pos.y));
    vec4 right = texture2D(texture, vec2(pos.x + dx, pos.y));
    vec4 down = texture2D(texture, vec2(pos.x, pos.y + dy));

    vec4 n = max(down, max(top, max(left, right)));
    float m = n.a*(1.0 - incolor.a);

    gl_FragColor = m * (n * 0.90);
}