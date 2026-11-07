#version 330 core

layout(location=0) in vec2 pos;
layout(location=1) in float side;

uniform mat4 mvp;

out float fragSide;

void main() {
        fragSide = side;
        gl_Position = mvp * vec4(pos.xy, 0.0, 1.0);
}
