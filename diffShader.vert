#version 300 es
layout(location = 0) in vec2 in_Position;
layout(location = 1) in vec2 in_TexCoord;

out vec2 ex_TexCoord;

void main(void){
    gl_Position = vec4(in_Position.x, in_Position.y, 0.0, 1.0);
    ex_TexCoord = in_TexCoord;
}
