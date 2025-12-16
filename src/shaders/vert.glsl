#version 460 compatibility

in vec2 aPos;
in vec2 aTexCoord;

out vec2 vTexCoord;

uniform vec2 u_cameraPos;
uniform float u_cameraScale;
uniform vec2 u_windowSize;
uniform vec2 u_screenshotSize;
uniform vec2 u_cursorPos;

vec3 toWorld(vec3 v) {
	vec2 ratio = vec2(
		u_windowSize.x / u_screenshotSize.x / u_cameraScale,
		u_windowSize.y / u_screenshotSize.y / u_cameraScale
	);

	return vec3(
		(v.x / u_screenshotSize.x * 2.0 - 1.0) / ratio.x,
		(v.y / u_screenshotSize.y * 2.0 - 1.0) / ratio.y,
		v.z
	);
}

void main() {
	vec3 p = vec3(aPos, 0.0) - vec3(u_cameraPos * vec2(1.0, -1.0), 0.0);
	gl_Position = vec4(toWorld(p), 1.0);
	vTexCoord = vec2(aTexCoord.x, 1.0f - aTexCoord.y);
}