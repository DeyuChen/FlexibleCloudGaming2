#version 300 es
in vec3 in_Position;
in vec3 in_Normal;
in vec2 in_TexCoord;
in vec3 in_Color;

out vec3 ex_FragPos;
out vec3 ex_Normal;
out vec3 ex_Color;
out vec2 ex_TexCoord;

uniform mat4 model;
uniform mat4 view;
uniform mat4 projection;
uniform bool hasTex;
uniform bool hasColor;

void main(void){
    ex_FragPos = mat3(model) * in_Position;
    ex_Normal = mat3(view) * (mat3(model) * in_Normal);
    gl_Position = projection * (view * vec4(ex_FragPos, 1.0));

    if (hasTex){
        ex_TexCoord = in_TexCoord;
    } else if (hasColor){
        ex_Color = in_Color;
    }
}
