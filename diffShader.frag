#version 300 es
precision highp float;

uniform sampler2D Texture0;
uniform sampler2D Texture1;

in vec2 ex_TexCoord;
layout(location = 0) out vec4 fragColor;

void main(void){
    vec4 Color_1 = texture(Texture0, ex_TexCoord);
    vec4 Color_2 = texture(Texture1, ex_TexCoord);
    fragColor = max(min((Color_1 - Color_2) / 2.0 + 0.5, 1.0), 0.0);
}
