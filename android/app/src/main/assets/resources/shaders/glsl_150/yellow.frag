precision lowp float;
precision lowp int;

varying vec2 vTexCoord;
uniform sampler2D texture;

void main()
{
    vec4 pixel = texture2D(texture, vTexCoord.xy);
    vec4 color = pixel;
    color.rgb = vec3(255,255,0);
    // gl_FragColor = color;
}