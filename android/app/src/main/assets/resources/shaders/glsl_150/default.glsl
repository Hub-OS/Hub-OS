// #version 150

// vec2 uv;
uniform sampler2D texture; // ../../navis/megaman/navi_megaman_shoot.png
vec4 fragColor;

void main()
{
    vec2 uv = gl_FragCoord.xy/iResolution.xy;
    fragColor = texture2D(texture, uv.st);
    gl_FragColor = fragColor;
}
