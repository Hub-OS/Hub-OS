#version 120

uniform sampler2D texture;

void main()
{
    vec4 pixel = texture2D(texture, vec2(gl_TexCoord[0].x, gl_TexCoord[0].y));
    vec4 color = gl_Color * pixel;
    vec3 luminances = vec3(0.2126, 0.7152, 0.0722);

    float luminance = dot(luminances, color.rgb);
    float threshold = 0.5;

    luminance = max(0.0, luminance - threshold);

    color.rgb *= sign(luminance);

    if(color.a > 0.0) {
        if(color.rgb == vec3(0,0,0)) {  color.rgb = vec3(1,1,0); }
        else { color.rgb = vec3(1,1,1); }
    }

    gl_FragColor = color;
}