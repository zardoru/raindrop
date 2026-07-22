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

const unsigned char noteFragShader[] = {
#embed "./noteFrag.glsl"
};

const unsigned char bgaFragShader[] = {
#embed "./bgaFrag.glsl"
};

const unsigned char sdfFragShader[] = {
#embed "./sdfFrag.glsl"
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
	int Shader::Default::mVertProgram, Shader::Default::mFragProgram, Shader::Default::mProgram;
	int Shader::Default::uniforms[static_cast<int>(Shader::Default::Uniform::Count)];

	bool Shader::Default::compile()
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
		// else
			// Log::LogPrintf("Default Vertex Shader Compiled Succesfully\n", buffer);

		glGetShaderiv(mFragProgram, GL_COMPILE_STATUS, &status);

		glGetShaderInfoLog(mFragProgram, 512, nullptr, buffer);

		if (status != GL_TRUE)
		{
			Log::LogPrintf("Fragment Shader Error: %s\n", buffer);
			return false;
		}
		// else Log::LogPrintf("Fragment Shader Compiled succesfully\n", buffer);

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
		uniforms[static_cast<int>(Uniform::Projection)] = glGetUniformLocation(mProgram, "projection");
		uniforms[static_cast<int>(Uniform::ModelView)] = glGetUniformLocation(mProgram, "mvp");
		uniforms[static_cast<int>(Uniform::Centered)] = glGetUniformLocation(mProgram, "centered");
		uniforms[static_cast<int>(Uniform::Color)] = glGetUniformLocation(mProgram, "color");


		static_bind();

		return true;
	}

	void Shader::Default::set_color(const float r, const float g, const float b, const float a)
	{
		set_uniform(uniform(Uniform::Color), l2gamma(r), l2gamma(g), l2gamma(b), a);
	}

	void Shader::Default::update_projection(Mat4 proj)
	{
		glUniformMatrix4fv(uniform(Uniform::Projection), 1, GL_FALSE, &proj[0][0]);
	}

	void Shader::Default::static_bind() {
		CHECKERR();
		if (m_last_shader_ != mProgram) {
			m_last_shader_ = mProgram;
			glUseProgram(mProgram);
			CHECKERR();
		}
	}

	int Shader::Default::get_vertex_shader() {
		return mVertProgram;
	}

	
	void Shader::compile(const std::string& frag) {
		mIsValid = true;
		CHECKERR();
		// Log::LogPrintf("Compiling fragment shader.\n");

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
		// else
		// 	Log::LogPrintf("Fragment Shader Compiled Succesfully\n");

		mShaderHandle = glCreateProgram();
		CHECKERR();
		glAttachShader(mShaderHandle, Shader::Default::get_vertex_shader());
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
		// else
		// 	Log::LogPrintf("Shader linking succesful.\n");

        assert(glGetError() == 0);

		glDeleteShader(fragsh);

        assert(glGetError() == 0);
	}

	Shader::Note::Note() {
		compile(std::string(reinterpret_cast<const char *>(noteFragShader), sizeof(noteFragShader)));
		if (is_valid()) cache_locations();
	}

	Shader::BGA::BGA() {
		compile(std::string(reinterpret_cast<const char *>(bgaFragShader), sizeof(bgaFragShader)));
		if (is_valid()) cache_locations();
	}

	Shader::SDF::SDF() {
		compile(std::string(reinterpret_cast<const char *>(sdfFragShader), sizeof(sdfFragShader)));
		if (is_valid()) cache_locations();
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

	void Shader::set_uniform(const int uniform, const int i)
	{
		glUniform1i(uniform, i);
	}

	void Shader::set_uniform(const int uniform, const float A, const float B, const float C, const float D)
	{
		glUniform4f(uniform, A, B, C, D);
	}

	void Shader::set_uniform(const int uniform, const glm::vec2 Pos)
	{
		glUniform2f(uniform, Pos.x, Pos.y);
	}

	void Shader::set_uniform(const int uniform, const glm::vec3 Pos)
	{
		glUniform3f(uniform, Pos.x, Pos.y, Pos.z);
	}

	void Shader::set_uniform(const int Uniform, const float F)
	{
		glUniform1f(Uniform, F);
	}

	void Shader::set_uniform(const int uniform, const float *matrix4_x4)
	{
		glUniformMatrix4fv(uniform, 1, GL_FALSE, matrix4_x4);
	}

	int Shader::enable_attrib_array(const int Attrib)
	{
		glEnableVertexAttribArray(Attrib);
		return Attrib;
	}

	int Shader::disable_attrib_array(const int Attrib)
	{
		glDisableVertexAttribArray(Attrib);
		return Attrib;
	}

	int Shader::get_uniform(const std::string& uni) const {
		return uniform_location(uni);
	}

	int Shader::uniform_location(const std::string& name) const {
		return glGetUniformLocation(mShaderHandle, name.c_str());
	}

	bool Shader::is_valid() const
	{
		return mIsValid;
	}

	int Shader::Default::uniform(const Uniform uniform) {
		return uniforms[static_cast<int>(uniform)];
	}

	void Shader::Note::cache_locations() {
		locations_.projection = uniform_location("projection");
		locations_.model_view = uniform_location("mvp");
		locations_.centered = uniform_location("centered");
		locations_.color = uniform_location("color");
		locations_.hidden_mode = uniform_location("HiddenLightning");
		locations_.hidden_center = uniform_location("hdcenter");
		locations_.hidden_size = uniform_location("hdsize");
		locations_.flashlight_size = uniform_location("flsize");
	}

	void Shader::Note::apply_draw_state(const DrawState &state) const {
		set_uniform(locations_.projection, &state.projection[0][0]);
		if (state.model) set_uniform(locations_.model_view, &(*state.model)[0][0]);
		set_uniform(locations_.centered, state.centered);
		set_uniform(locations_.color, l2gamma(state.color.red), l2gamma(state.color.green), l2gamma(state.color.blue), state.color.alpha);
	}

	void Shader::Note::set_hidden_effect(const int mode, const float center, const float transition_size,
	                                     const float flashlight_size) {
		bind();
		set_uniform(locations_.hidden_mode, mode);
		set_uniform(locations_.hidden_center, center);
		set_uniform(locations_.hidden_size, transition_size);
		set_uniform(locations_.flashlight_size, flashlight_size);
	}

	void Shader::BGA::cache_locations() {
		locations_.projection = uniform_location("projection");
		locations_.model_view = uniform_location("mvp");
		locations_.centered = uniform_location("centered");
		locations_.color = uniform_location("color");
	}

	void Shader::BGA::apply_draw_state(const DrawState &state) const {
		set_uniform(locations_.projection, &state.projection[0][0]);
		if (state.model) set_uniform(locations_.model_view, &(*state.model)[0][0]);
		set_uniform(locations_.centered, state.centered);
		set_uniform(locations_.color, l2gamma(state.color.red), l2gamma(state.color.green), l2gamma(state.color.blue), state.color.alpha);
	}

	void Shader::SDF::cache_locations() {
		locations_.projection = uniform_location("projection");
		locations_.model_view = uniform_location("mvp");
		locations_.centered = uniform_location("centered");
		locations_.color = uniform_location("color");
	}

	void Shader::SDF::apply_draw_state(const DrawState &state) const {
		set_uniform(locations_.projection, &state.projection[0][0]);
		if (state.model) set_uniform(locations_.model_view, &(*state.model)[0][0]);
		set_uniform(locations_.centered, state.centered);
		set_uniform(locations_.color, l2gamma(state.color.red), l2gamma(state.color.green), l2gamma(state.color.blue), state.color.alpha);
	}

	Shader::Shader() {
		mShaderHandle = -1;
		GameWindow::get_instance().add_shader(this);
	}

	Shader::~Shader() {
		glDeleteProgram(mShaderHandle);
		GameWindow::get_instance().remove_shader(this);
	}
}
