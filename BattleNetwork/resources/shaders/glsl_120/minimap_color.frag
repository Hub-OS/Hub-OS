#version 120

uniform sampler2D texture;
uniform vec2 center;
uniform vec2 tileSize;
uniform vec4 finalColor;
uniform int mask;

void main() {

    vec2 pos = gl_TexCoord[0].xy; // pos is the uv coord (subrect)
    vec4 incolor = texture2D(texture, pos).rgba;
    vec4 outcolor;

    pos.x = center.x - pos.x;
    pos.y = center.y - pos.y;

    if (mask == 0) {
    outcolor = vec4(finalColor.r*0.60, finalColor.g*1.20, finalColor.b*0.60, ceil(incolor.a));
    }
    else {
    outcolor = finalColor * ceil(incolor.a);
    }

    if (mask > 0 && abs(2.0 * pos.x / tileSize.x) + abs(2.0 * pos.y / tileSize.y) > 1.0)
    discard;

    gl_FragColor = outcolor;
}