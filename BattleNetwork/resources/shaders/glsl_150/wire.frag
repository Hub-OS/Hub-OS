precision lowp float;
precision lowp int;

varying vec4 vColor;
varying vec2 vTexCoord;
uniform sampler2D texture;

uniform int index;
uniform int numOfWires;
uniform float progress;
uniform vec4 inColor;

void main()
{
    vec4 pixel = texture2D(texture, vec2(vTexCoord.x, vTexCoord.y));
    vec4 color = vColor * pixel;

    float numOfWiresf = float(numOfWires);
    float indexf = float(index);

    float wireBIndex = floor(255.0/numOfWiresf)/255.0;
    wireBIndex = wireBIndex*indexf;

    float progressClamp = min(1.0, progress);

    vec4 transp = vec4(1.0, 1.0,1.0, 0.0);

    if(color != vec4(0.0,0.0,0.0,1.0) ){
        if(wireBIndex >= color.b-0.1 && wireBIndex <= color.b+0.1) {
            if(progressClamp >= color.a-0.1 && progressClamp <= color.a+0.1) {
                color = inColor;
                color.a = 0.8;
            } else {
                color = transp;
            }
        } else {
            color = transp;
        }
    }
    else {
        color = transp;
    }

    gl_FragColor = color;
}
