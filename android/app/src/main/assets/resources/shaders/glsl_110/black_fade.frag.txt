#version 120

uniform sampler2D texture;
uniform float opacity;

void main()
{
    vec4 pixel = texture2D(texture, gl_TexCoord[0].xy);
    vec4 color = gl_Color * pixel;
    color.rgb = color.rgb * (1.0-opacity);
    gl_FragColor = color;
}
