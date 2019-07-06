#version 120

uniform sampler2D texture;

void main()
{
    vec4 pixel = texture2D(texture, vec2(gl_TexCoord[0].x, gl_TexCoord[0].y));
    vec4 color = gl_Color * pixel;
    color.rgb = vec3(255);
    gl_FragColor = color;
}