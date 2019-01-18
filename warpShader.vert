#version 300 es
in vec2 in_Position;
in lowp vec3 in_Color;
in float in_Depth;

out vec3 ex_Color;

uniform mat4 mvp;

void main(void){
    if (in_Depth == 1.0){
        // make it invisible
        gl_Position = vec4(in_Position, 1.1, 1.0);
    } else {
        gl_Position = mvp * vec4(in_Position.x, in_Position.y, (in_Depth - 0.5) * 2.0, 1.0);
        // leave depth of 1.0 for special purpose
        if (gl_Position.z == 1.0){
            gl_Position.z = 0.9999999999;
        }
    }

    ex_Color = in_Color / 255.0;
}
