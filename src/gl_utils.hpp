#pragma once

#include <iostream>
#include <fstream>
#include <sstream>
#include <string>

// functions stolen from https://github.com/bigboyconst/glRenderer
GLuint compileShader(GLenum type, const char* src) {
	GLuint shader = glCreateShader(type);
	glShaderSource(shader, 1, &src, nullptr);
	glCompileShader(shader);

	GLint ok;
	glGetShaderiv(shader, GL_COMPILE_STATUS, &ok);
	if (!ok) {
		GLint len;
		glGetShaderiv(shader, GL_INFO_LOG_LENGTH, &len);
		std::string log(len, '\0');
		glGetShaderInfoLog(shader, len, nullptr, log.data());
		fprintf(stderr, "(in %s)\n", type == GL_FRAGMENT_SHADER ? "fragment shader" : "vertex shader");
		fprintf(stderr, "Shader compilation error:\n %s\n", log.c_str());
		exit(EXIT_FAILURE);
	}
	return shader;
}

GLuint getCompiledShader(GLenum type, const char* path) {
	std::ifstream file(path);
	if (!file.is_open()) {
		throw std::runtime_error("Couldn't find shader.");
	}

	std::stringstream buf;
	buf << file.rdbuf();
	file.close();

	std::string src = buf.str();

	return compileShader(type, src.c_str());
}

GLuint loadShader(const char* vertPath, const char* fragPath) {
	GLuint v = getCompiledShader(GL_VERTEX_SHADER, vertPath);
	GLuint f = getCompiledShader(GL_FRAGMENT_SHADER, fragPath);
	
	GLuint prog = glCreateProgram();
	glAttachShader(prog, v);
	glAttachShader(prog, f);
	glLinkProgram(prog);

	GLint ok;
	glGetProgramiv(prog, GL_LINK_STATUS, &ok);
	if (!ok) {
		GLint len;
		glGetProgramiv(prog, GL_INFO_LOG_LENGTH, &len);
		std::string log(len, '\0');
		glGetProgramInfoLog(prog, len, nullptr, log.data());
		fprintf(stderr, "Program linking error:\n %s\n", log.c_str());
		exit(EXIT_FAILURE);
	}

	glDeleteShader(v);
	glDeleteShader(f);
	return prog;
}

GLuint createTexture(int width, int height) {
	GLuint tex;
	glGenTextures(1, &tex);
	glBindTexture(GL_TEXTURE_2D, tex);

	glTexImage2D(
		GL_TEXTURE_2D,
		0,
		GL_RGBA8,
		width,
		height,
		0,
		GL_RGBA,
		GL_UNSIGNED_BYTE,
		nullptr
	);

	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

	return tex;
}