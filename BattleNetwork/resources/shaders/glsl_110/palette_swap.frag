#version 120

uniform sampler2D texture;
uniform sampler2D palette;

void main()
{
    vec4 pixel = texture2D(texture, vec2(gl_TexCoord[0].x, gl_TexCoord[0].y));
    vec4 color = vec4(0.299*pixel.r,0.587*pixel.g,0.114*pixel.b,pixel.a);
    float lum = color.r+color.g+color.b;

    gl_FragColor = texture2D(palette, vec2(pixel.r,0));
}