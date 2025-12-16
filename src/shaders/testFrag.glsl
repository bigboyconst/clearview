#version 460 compatibility

in vec2 vUV;

out vec4 fragColor;

uniform sampler2D u_tex;

void main() {
	fragColor = texture(u_tex, vUV);
}