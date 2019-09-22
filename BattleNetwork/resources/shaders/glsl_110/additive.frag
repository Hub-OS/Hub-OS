#version 120

uniform sampler2D texture;

void main()
{
    vec4 pixel = texture2D(texture, vec2(gl_TexCoord[0].x, gl_TexCoord[0].y));
    vec4 color = mix(gl_Color.rgb, vec3(1.0), pixel.a);
    gl_FragColor = color;
}