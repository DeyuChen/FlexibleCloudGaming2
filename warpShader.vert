#version 300 es
in vec2 in_Position;
in lowp vec3 in_Color;
in float in_Depth;

out vec3 ex_Color;

uniform mat4 mvp;

void main(void){
    gl_Position = mvp * vec4(in_Position.x, in_Position.y, (in_Depth - 0.5) * 2.0, 1.0);
    // keep depth in feasible range, leave 1.0 for hole filling checking
    gl_Position.z = min(max(gl_Position.z, 0.0), 0.9999999999);

    ex_Color = in_Color / 255.0;
}
