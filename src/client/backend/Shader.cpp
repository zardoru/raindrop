#include <string>
#include <cctype>
#include <glm.h>
#include <GL/glew.h>
#include <cassert>
#include <rmath.h>

#include "Transformation.h"
#include "Rendering.h"
#include "Shader.h"
#include "GameWindow.h"

#include "Logging.h"

void CHECKERR() {
		auto err = glGetError();
		if (err) {
			Log::LogPrintf("OpenGL Shader Error: %d\n", err);
		}
	}

const unsigned char vertShader[] = {
#embed "./defaultVert.glsl"
};

const unsigned char fragShader[] = {
#embed "./defaultFrag.glsl"
};

namespace {
	void replace_all(std::string& source, const std::string& from, const std::string& to) {
		size_t pos = 0;
		while ((pos = source.find(from, pos)) != std::string::npos) {
			source.replace(pos, from.size(), to);
			pos += to.size();
		}
	}

	std::string normalize_shader_source(std::string source) {
		if (source.size() >= 3 &&
			static_cast<unsigned char>(source[0]) == 0xEF &&
			static_cast<unsigned char>(source[1]) == 0xBB &&
			static_cast<unsigned char>(source[2]) == 0xBF) {
			source.erase(0, 3);
		}

		const auto first = source.find_first_not_of(" \t\r\n");
		if (first != std::string::npos)
			source.erase(0, first);

		replace_all(source, "texture2D", "texture");

		return source;
	}
}

namespace renderer {
	int Shader::m_last_shader_ = -1;
	int DefaultShader::mVertProgram, DefaultShader::mFragProgram, DefaultShader::mProgram;
	uint32_t DefaultShader::uniforms[NUM_SHADERVARS];

	bool DefaultShader::compile()
	{
		CHECKERR();
		mVertProgram = glCreateShader(GL_VERTEX_SHADER);
		auto normalizedVert = normalize_shader_source(std::string(reinterpret_cast<const char *>(vertShader), sizeof(vertShader)));
		const auto vertSrc = normalizedVert.c_str();
		const auto vertLength = static_cast<GLint>(normalizedVert.size());
		glShaderSource(mVertProgram, 1, &vertSrc, &vertLength);
		glCompileShader(mVertProgram);

		mFragProgram = glCreateShader(GL_FRAGMENT_SHADER);
		auto normalizedFrag = normalize_shader_source(std::string(reinterpret_cast<const char *>(fragShader), sizeof(fragShader)));
		const auto fragSrc = normalizedFrag.c_str();
		const auto fragLength = static_cast<GLint>(normalizedFrag.size());
		glShaderSource(mFragProgram, 1, &fragSrc, &fragLength);
		glCompileShader(mFragProgram);

		GLint status;
		char buffer[512];

		glGetShaderiv(mVertProgram, GL_COMPILE_STATUS, &status);

		glGetShaderInfoLog(mVertProgram, 512, nullptr, buffer);

		if (status != GL_TRUE)
		{
			Log::LogPrintf("Default Vertex Shader Error: %s\n", buffer);
			return false;
		}
		else
			Log::LogPrintf("Default Vertex Shader Compiled Succesfully\n", buffer);

		glGetShaderiv(mFragProgram, GL_COMPILE_STATUS, &status);

		glGetShaderInfoLog(mFragProgram, 512, nullptr, buffer);

		if (status != GL_TRUE)
		{
			Log::LogPrintf("Fragment Shader Error: %s\n", buffer);
			return false;
		}
		else Log::LogPrintf("Fragment Shader Compiled succesfully\n", buffer);

		mProgram = glCreateProgram();
		glAttachShader(mProgram, mVertProgram);
		glAttachShader(mProgram, mFragProgram);

		// glBindFragDataLocation( mProgram, 0, "outColor" );

		glLinkProgram(mProgram);

		glGetProgramiv(mProgram, GL_LINK_STATUS, &status);
		if (!status) {
			glGetProgramInfoLog(mProgram, 512, nullptr, buffer);
			Log::LogPrintf("Shader linking failed: %d - %s\n", status, buffer);
			return false;
		}

		// Use our recently compiled program.

		// Clean up..
		//glDeleteShader(mVertProgram);
		glDeleteShader(mFragProgram);

		// Set up the uniform constants we'll be using in the program.
		uniforms[A_POSITION] = glGetAttribLocation(mProgram, "position");
		uniforms[A_UV] = glGetAttribLocation(mProgram, "vertexUV");
		uniforms[A_COLOR] = glGetAttribLocation(mProgram, "colorvert");
		uniforms[U_MODELVIEW] = glGetUniformLocation(mProgram, "mvp");
		uniforms[U_CENTERED] = glGetUniformLocation(mProgram, "centered");
		uniforms[U_COLOR] = glGetUniformLocation(mProgram, "color");
		uniforms[U_INVERT] = glGetUniformLocation(mProgram, "inverted");
		uniforms[U_HIDDEN] = glGetUniformLocation(mProgram, "HiddenLightning");
		uniforms[U_HIDCENTER] = glGetUniformLocation(mProgram, "hdcenter");
		uniforms[U_HIDSIZE] = glGetUniformLocation(mProgram, "hdsize");
		uniforms[U_HIDFLSIZE] = glGetUniformLocation(mProgram, "flsize");
		uniforms[U_REPCOLOR] = glGetUniformLocation(mProgram, "replaceColor");
		uniforms[U_BTRANSP] = glGetUniformLocation(mProgram, "BlackToTransparent");


		static_bind();

		return true;
	}

	void DefaultShader::set_color(const float r, const float g, const float b, const float a)
	{
		SetUniform(GetUniform(U_COLOR), l2gamma(r), l2gamma(g), l2gamma(b), a);
	}

	void DefaultShader::update_projection(Mat4 proj)
	{
		GLuint MatrixID = glGetUniformLocation(mProgram, "projection");
		glUniformMatrix4fv(MatrixID, 1, GL_FALSE, &proj[0][0]);
	}

	void DefaultShader::static_bind() {
		CHECKERR();
		if (m_last_shader_ != mProgram) {
			m_last_shader_ = mProgram;
			glUseProgram(mProgram);
			CHECKERR();
		}
	}

	int DefaultShader::get_vertex_shader() {
		return mVertProgram;
	}

	
	void Shader::compile(const std::string& frag) {
		mIsValid = true;
		CHECKERR();
		Log::LogPrintf("Compiling fragment shader.\n");

		auto normalized_frag = normalize_shader_source(frag);
		auto fragsh = glCreateShader(GL_FRAGMENT_SHADER);
		auto src = normalized_frag.c_str();
		const auto length = static_cast<GLint>(normalized_frag.size());
		glShaderSource(fragsh, 1, &src, &length); CHECKERR();
		glCompileShader(fragsh);

		GLint status;
		char buffer[512];

		glGetShaderiv(fragsh, GL_COMPILE_STATUS, &status);

		glGetShaderInfoLog(fragsh, 512, nullptr, buffer);

		if (status != GL_TRUE)
		{
			Log::LogPrintf("Fragment Shader Error: %s\n", buffer);
			mIsValid = false;
		}
		else
			Log::LogPrintf("Fragment Shader Compiled Succesfully\n");

		mShaderHandle = glCreateProgram();
		CHECKERR();
		glAttachShader(mShaderHandle, DefaultShader::get_vertex_shader());
		CHECKERR();
		glAttachShader(mShaderHandle, fragsh);
		CHECKERR();
		glLinkProgram(mShaderHandle);
		CHECKERR();

		glGetProgramiv(mShaderHandle, GL_LINK_STATUS, &status);
		if (!status) {
			glGetProgramInfoLog(mShaderHandle, 512, nullptr, buffer);
			Log::LogPrintf("Shader linking failed: %d - %s\n", status, buffer);
			mIsValid = false;
		}
		else
			Log::LogPrintf("Shader linking succesful.\n");

        assert(glGetError() == 0);

		glDeleteShader(fragsh);

        assert(glGetError() == 0);
	}

	void Shader::bind() {
		CHECKERR();
		assert(glIsProgram(mShaderHandle));
		if (m_last_shader_ != mShaderHandle) {
			m_last_shader_ = mShaderHandle;
			glUseProgram(mShaderHandle);
			CHECKERR();
		}
	}

	void Shader::SetUniform(const uint32_t uniform, const int i)
	{
		glUniform1i(uniform, i);
	}

	void Shader::SetUniform(const uint32_t uniform, const float A, const float B, const float C, const float D)
	{
		glUniform4f(uniform, A, B, C, D);
	}

	void Shader::SetUniform(const uint32_t uniform, const glm::vec2 Pos)
	{
		glUniform2f(uniform, Pos.x, Pos.y);
	}

	void Shader::SetUniform(const uint32_t uniform, const glm::vec3 Pos)
	{
		glUniform3f(uniform, Pos.x, Pos.y, Pos.z);
	}

	void Shader::SetUniform(const uint32_t Uniform, const float F)
	{
		glUniform1f(Uniform, F);
	}

	void Shader::set_uniform(const uint32_t uniform, const float *matrix4_x4)
	{
		glUniformMatrix4fv(uniform, 1, GL_FALSE, matrix4_x4);
	}

	int Shader::enable_attrib_array(const uint32_t Attrib)
	{
		glEnableVertexAttribArray(Attrib);
		return Attrib;
	}

	int Shader::disable_attrib_array(const uint32_t Attrib)
	{
		glDisableVertexAttribArray(Attrib);
		return Attrib;
	}

	uint32_t Shader::get_uniform(const std::string& uni) const {
		return glGetUniformLocation(mShaderHandle, uni.c_str());
	}

	bool Shader::is_valid() const
	{
		return mIsValid;
	}

	uint32_t DefaultShader::GetUniform(const uint32_t uni) {
		assert(uni < NUM_SHADERVARS);
		return uniforms[uni];
	}

	Shader::Shader() {
		mShaderHandle = -1;
		window.add_shader(this);
	}

	Shader::~Shader() {
		glDeleteProgram(mShaderHandle);
		window.remove_shader(this);
	}
}
