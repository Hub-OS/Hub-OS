precision highp float;
precision highp int;

uniform mat4 viewMatrix;
uniform mat4 projMatrix;
uniform mat4 textMatrix;

attribute vec2 position;
attribute vec4 color;
attribute vec2 texCoord;

varying vec4 vColor;
varying vec2 vTexCoord;
varying vec2 vRawTexCoord;
 
void main()
{
    gl_Position = projMatrix * viewMatrix * vec4(position, 0.0, 1.0);
    vColor = color;
    vTexCoord = (textMatrix * vec4(texCoord.xy, 0.0, 1.0)).xy;
}