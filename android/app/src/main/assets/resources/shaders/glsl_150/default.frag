precision lowp float;
precision lowp int;

varying vec4 vColor;
varying vec2 vTexCoord;
uniform sampler2D texture;

void main()
{
    gl_FragColor = texture2D(texture, vTexCoord.st);
    //gl_FragColor.a = 1.0;
}
