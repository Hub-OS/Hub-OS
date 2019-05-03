precision lowp float;
precision lowp int;

uniform mat4 viewMatrix;
uniform mat4 projMatrix;
 
attribute vec2 position;
attribute vec4 color;
attribute vec2 texCoord;

varying vec4 vColor;
varying vec2 vTexCoord;
 
void main()
{
    gl_Position = projMatrix * viewMatrix * vec4(position, 0.0, 1.0);
    vColor = color;
    vTexCoord = texCoord;
}