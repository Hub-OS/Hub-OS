precision lowp float;
precision lowp int;

varying vec2 vTexCoord;
uniform sampler2D texture;
uniform sampler2D map;
uniform float progress;

void main()
{
    vec4 pixel = texture2D(texture, vTexCoord.xy);
    vec4 transition = texture2D(map,  vTexCoord.xy);
    vec4 color = pixel;

    if(progress >= transition.b) { 
      color *= vec4(0,0,0,1);
    }

    // gl_FragColor = color;
}
