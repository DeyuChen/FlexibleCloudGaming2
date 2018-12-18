#version 300 es
precision highp float;

in vec3 ex_Color;
out vec4 fragColor;

void main(void){
    fragColor = vec4(ex_Color, 1.0);
}
