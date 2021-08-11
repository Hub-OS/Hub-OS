#version 120

uniform sampler2D texture;

void main()
{
    vec4 pixel = texture2D(texture, vec2(gl_TexCoord[0].x, gl_TexCoord[0].y));
    gl_FragColor = vec4(0,0,0,pixel.a);
} 