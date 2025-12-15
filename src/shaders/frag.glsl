#version 460 compatibility

out vec4 fragColor;

in vec2 vTexCoord;

uniform sampler2D u_tex;
uniform vec2 u_cursorPos;
uniform vec2 u_windowSize;
uniform float u_flShadow;
uniform float u_flRadius;
uniform float u_cameraScale;

void main() {
	vec4 cursor = vec4(u_cursorPos.x, u_windowSize.y - u_cursorPos.y, 0.0, 1.0);

	color = mix(
		texture(u_tex, vTexCoord), 
		vec4(0.0, 0.0, 0.0, 0.0),
		length(cursor - gl_FragCoord) < (u_flRadius * u_cameraScale) ? 0.0 : u_flShadow
	);
}