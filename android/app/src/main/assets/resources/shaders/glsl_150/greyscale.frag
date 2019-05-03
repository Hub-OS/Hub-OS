precision lowp float;
precision lowp int;

varying vec2 vTexCoord;
uniform sampler2D texture;

void main()
{
    vec4 pixel = texture2D(texture, vTexCoord.xy);
    pixel.rgb = vec3((pixel.r+pixel.b+pixel.g)/3.0);
    // gl_FragColor = pixel;
}