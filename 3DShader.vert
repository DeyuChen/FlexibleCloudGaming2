#version 300 es
in vec3 in_Position;
in vec3 in_Normal;
in lowp vec3 in_Color;
in vec2 in_TexCoord;

out vec3 ex_FragPos;
out vec3 ex_Normal;
out vec3 ex_Color;
out vec2 ex_TexCoord;

uniform mat4 model;
uniform mat4 view;
uniform mat4 projection;

void main(void){
    ex_FragPos = vec3(model * vec4(in_Position, 1.0));
    ex_Normal = mat3(transpose(inverse(model))) * in_Normal;
    gl_Position = projection * view * vec4(ex_FragPos, 1.0);
    
    ex_Color = in_Color / 255.0;
    ex_TexCoord = in_TexCoord;
}
