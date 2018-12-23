#version 300 es
in vec2 in_Position;
in lowp vec3 in_Color_1;
in lowp vec3 in_Color_2;

out vec3 ex_Color;

void main(void){
    gl_Position = vec4(in_Position.x, in_Position.y, 0.0, 1.0);
    ex_Color = max(min((in_Color_1 / 255.0 - in_Color_2 / 255.0) / 2.0 + 0.5, 1.0), 0.0);
}
