#version 120

uniform sampler2D texture;

void main()
{
    vec4 pixel = texture2D(texture, vec2(gl_TexCoord[0].x, gl_TexCoord[0].y));
    vec4 color = gl_Color * vec4((pixel.r+pixel.g+pixel.b)/3.0f);
    color.a = pixel.a;
    gl_FragColor = color;
}