precision lowp float;
precision lowp int;

varying vec2 vTexCoord;
uniform sampler2D texture;

vec4 fragColor;

void main()
{
    fragColor = texture2D(texture, vTexCoord.st);
    gl_FragColor = vec4(1.0, 0.0, 0.0, 1.0);
}
