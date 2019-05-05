precision lowp float;
precision lowp int;

// Pixel blur inside of active texture coordinates

varying vec2 vTexCoord;
varying vec4 vColor;
uniform sampler2D texture;
uniform float pixel_threshold;

uniform float x;
uniform float y;
uniform float w;
uniform float h;

void main()
{
    vec2 origin = vec2(x, y);
    vec2 size   = vec2(w, h);

    // Get the texture coordinate
    vec2 texCoord = vTexCoord.xy;

    // Make its lower-left be at (0,0) and it's upper right be at (1,1)
    texCoord = (texCoord - origin) / size;

    // Apply the factor
    float factor = 1.0 / (pixel_threshold + 0.001);
    texCoord = floor(texCoord * factor + 0.5) / factor;

    // Apply the wrapping
    texCoord = fract(texCoord);

    // Convert back to texture atlas coordinates
    texCoord = (texCoord * size) + origin;

    gl_FragColor = texture2D(texture, texCoord) * vColor;
}
