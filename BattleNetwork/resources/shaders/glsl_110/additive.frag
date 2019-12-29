#version 120

uniform sampler2D texture;

void main()
{
    vec4 pixel = texture2D(texture, vec2(gl_TexCoord[0].x, gl_TexCoord[0].y));
    vec4 color = gl_Color * (vec4(1.0f)-pixel);
    color = color + pixel;
    color.a = pixel.a;
    gl_FragColor = color;
}