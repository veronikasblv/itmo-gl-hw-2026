#version 330 core

uniform vec4 color;
uniform vec3 background;

in float fragSide;

out vec4 out_col;

void main() {
        float d = abs(fragSide);
        float glow = pow(1.0 - d, 2.0);
        vec3 animated_color = mix(background, color.rgb, glow);

        out_col = vec4(animated_color, 1.0);
}
