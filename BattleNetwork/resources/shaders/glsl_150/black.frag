precision lowp float;
precision lowp int;

varying vec4 vColor;
varying vec2 vTexCoord;
uniform sampler2D texture;

void main()
{
    vec4 pixel = texture2D(texture, vec2(vTexCoord.x, vTexCoord.y));
    gl_FragColor = vec4(0,0,0,pixel.a);
} 