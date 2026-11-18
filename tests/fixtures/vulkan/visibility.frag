#version 450
layout(push_constant) uniform Draw { float depth; uint discard_all; } draw;
layout(location=0) out vec4 color;
void main() {
    if (draw.discard_all != 0) discard;
    color = vec4(1,0,0,1);
}
