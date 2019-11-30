#version 120

// Wrap a subrect in a texture to preserve the image

uniform sampler2D texture;

uniform float x;
uniform float y;
uniform float w;
uniform float h;
uniform float offsetx;
uniform float offsety;

void main()
{
    vec2 origin = vec2(x, y);
    vec2 size   = vec2(w, h);

    // Get the texture coordinate
    vec2 texCoord = gl_TexCoord[0].xy;

    // Make its lower-left be at (0,0) and it's upper right be at (1,1)
    texCoord = (texCoord - origin) / size;

    // Apply the offest
    texCoord = texCoord + vec2(offsetx,offsety);

    // Apply the wrapping
    texCoord = fract(texCoord);

    // Convert back to texture atlas coordinates
    texCoord = (texCoord * size) + origin;

    gl_FragColor = texture2D(texture, texCoord) * gl_Color;
}