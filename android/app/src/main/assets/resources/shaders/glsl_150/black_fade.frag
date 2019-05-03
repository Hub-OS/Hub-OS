precision lowp float;
precision lowp int;

varying vec2 vTexCoord;
uniform sampler2D texture;
uniform float opacity;

void main()
{
    //vec4 pixel = texture2D(texture, vTexCoord.xy);
    //pixel.rgb = pixel.rgb * (1.0-opacity);
    // gl_FragColor = pixel;
}
