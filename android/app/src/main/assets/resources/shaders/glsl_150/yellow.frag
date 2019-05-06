precision lowp float;
precision lowp int;

varying vec4 vColor;
varying vec2 vTexCoord;
uniform sampler2D texture;

void main()
{
    vec4 color = texture2D(texture, vTexCoord.st) * vColor;
    float weightGray = color.r * 0.3 + color.g * 0.59 + color.b * 0.11 + 0.2;
    gl_FragColor.rgb = vec3(weightGray, weightGray, 0.0);
    gl_FragColor.a = color.a;
}