#include "pch.h"
#include "Logging.h"
#include "Rendering.h"
#include "Shader.h"
#include "GameWindow.h"

void CHECKERR() {
		auto err = glGetError();
		if (err) {
			Log::LogPrintf("OpenGL Shader Error: %d\n", err);
		}
	}

const char* vertShader = "#version 120\n"
"attribute vec3 position;\n"
"attribute vec2 vertexUV;\n"
"attribute vec4 colorvert;\n"
"uniform mat4 projection;\n"
"uniform mat4 mvp;\n"
"uniform bool centered;\n"
"varying vec2 texcoord;\n"
"varying vec3 Pos_world;\n"
"varying vec4 colorfrag;\n"
"void main() \n"
"{\n"
"	vec3 k_pos = position;\n"
"	if (centered){\n"
"		k_pos = k_pos + vec3(-0.5, -0.5, 0);\n"
"	}\n"
"	gl_Position = projection * mvp * vec4(k_pos.xyz, 1);\n"
"	Pos_world = (projection * mvp * vec4(k_pos.xyz, 1)).xyz;\n"
"	texcoord = vertexUV;\n"
"   colorfrag = colorvert;\n"
"}";

const char* fragShader = "#version 120\n"
"varying vec2 texcoord;\n"
"varying vec3 Pos_world;\n"
"varying vec4 colorfrag;\n"
"uniform vec4 color;\n"
"uniform sampler2D tex;\n"
"uniform bool inverted;\n"
"uniform bool replaceColor;\n"
"uniform float hdcenter;\n"
"uniform float flsize;\n"
"uniform float hdsize;\n"
"uniform bool BlackToTransparent;\n" // If true, transform r0 g0 b0 aX to a0.
"uniform int HiddenLightning;\n"
"\n"
"void main(void)\n"
"{\n"
"    vec4 tCol;\n"
"	 vec4 tex2D;\n"
"	 if (!replaceColor){\n"
"		tex2D = texture2D(tex, texcoord);\n"
"	 }else{"
"		tex2D = vec4(1.0, 1.0, 1.0, smoothstep(0.40, 0.5, texture2D(tex, texcoord).a));"
"	 }"
"	 if (inverted) {\n"
"		tCol = vec4(1.0, 1.0, 1.0, tex2D.a*2) - tex2D * color;\n"
"	 }else{\n"
"		tCol = tex2D * color;\n"
"	 }\n"
"	 if (BlackToTransparent) {\n"
"			if (tCol.r == 0 && tCol.g == 0 && tCol.b == 0) tCol.a = 0;"
"	 }"
"	if (HiddenLightning > 0) {\n"
"       float ld = 1;\n"
"		if (HiddenLightning == 2) "
"           ld = smoothstep(hdcenter - hdsize, hdcenter, Pos_world.y);\n"
"		else if (HiddenLightning == 1) "
"           ld = smoothstep(hdcenter, hdcenter - hdsize, Pos_world.y);\n"
"		else { ld = smoothstep(hdcenter - flsize - hdsize / 2, hdcenter - flsize, Pos_world.y) * "
"                   smoothstep(hdcenter + flsize + hdsize / 2, hdcenter + flsize, Pos_world.y);"
" } \n"
"		gl_FragColor = vec4(tCol.rgb, tCol.a * ld);\n"
"	} else if (HiddenLightning == 0) {\n"
"		gl_FragColor = tCol;\n"
"	}\n"
"    gl_FragColor *= colorfrag;\n"
"}\n";



namespace Renderer {
	int Shader::mLastShader = -1;
	int DefaultShader::mVertProgram, DefaultShader::mFragProgram, DefaultShader::mProgram;
	uint32_t DefaultShader::uniforms[NUM_SHADERVARS];

	bool DefaultShader::Compile()
	{
		CHECKERR();
		mVertProgram = glCreateShader(GL_VERTEX_SHADER);
		glShaderSource(mVertProgram, 1, &vertShader, NULL);
		glCompileShader(mVertProgram);

		mFragProgram = glCreateShader(GL_FRAGMENT_SHADER);
		glShaderSource(mFragProgram, 1, &fragShader, NULL);
		glCompileShader(mFragProgram);

		GLint status;
		char buffer[512];

		glGetShaderiv(mVertProgram, GL_COMPILE_STATUS, &status);

		glGetShaderInfoLog(mVertProgram, 512, NULL, buffer);

		if (status != GL_TRUE)
		{
			Log::LogPrintf("Default Vertex Shader Error: %s\n", buffer);
			return false;
		}
		else
			Log::LogPrintf("Default Vertex Shader Compiled Succesfully\n", buffer);

		glGetShaderiv(mFragProgram, GL_COMPILE_STATUS, &status);

		glGetShaderInfoLog(mFragProgram, 512, NULL, buffer);

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
			glGetProgramInfoLog(mProgram, 512, NULL, buffer);
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
		uniforms[U_MVP] = glGetUniformLocation(mProgram, "mvp");
		uniforms[U_CENTERED] = glGetUniformLocation(mProgram, "centered");
		uniforms[U_COLOR] = glGetUniformLocation(mProgram, "color");
		uniforms[U_INVERT] = glGetUniformLocation(mProgram, "inverted");
		uniforms[U_HIDDEN] = glGetUniformLocation(mProgram, "HiddenLightning");
		uniforms[U_HIDCENTER] = glGetUniformLocation(mProgram, "hdcenter");
		uniforms[U_HIDSIZE] = glGetUniformLocation(mProgram, "hdsize");
		uniforms[U_HIDFLSIZE] = glGetUniformLocation(mProgram, "flsize");
		uniforms[U_REPCOLOR] = glGetUniformLocation(mProgram, "replaceColor");
		uniforms[U_BTRANSP] = glGetUniformLocation(mProgram, "BlackToTransparent");


		Bind();

		return true;
	}

	void DefaultShader::SetColor(float r, float g, float b, float a)
	{
		SetUniform(GetUniform(U_COLOR), l2gamma(r), l2gamma(g), l2gamma(b), a);
	}

	void DefaultShader::UpdateProjection(Mat4 proj)
	{
		GLuint MatrixID = glGetUniformLocation(mProgram, "projection");
		glUniformMatrix4fv(MatrixID, 1, GL_FALSE, &proj[0][0]);
	}

	void DefaultShader::Bind() {
		CHECKERR();
		if (mLastShader != mProgram) {
			mLastShader = mProgram;
			glUseProgram(mProgram);
			CHECKERR();
		}
	}

	int DefaultShader::GetVertexShader() {
		return mVertProgram;
	}

	
	void Shader::Compile(std::string frag) {
		mIsValid = true;
		CHECKERR();
		Log::LogPrintf("Compiling fragment shader.\n");

		auto fragsh = glCreateShader(GL_FRAGMENT_SHADER);
		auto src = frag.c_str();
		glShaderSource(fragsh, 1, &src, NULL);
		glCompileShader(fragsh);

		GLint status;
		char buffer[512];

		glGetShaderiv(fragsh, GL_COMPILE_STATUS, &status);

		glGetShaderInfoLog(fragsh, 512, NULL, buffer);

		if (status != GL_TRUE)
		{
			Log::LogPrintf("Fragment Shader Error: %s\n", buffer);
			mIsValid = false;
		}
		else
			Log::LogPrintf("Fragment Shader Compiled Succesfully\n");

		mShaderHandle = glCreateProgram();
		CHECKERR();
		glAttachShader(mShaderHandle, DefaultShader::GetVertexShader());
		CHECKERR();
		glAttachShader(mShaderHandle, fragsh);
		CHECKERR();
		glLinkProgram(mShaderHandle);
		CHECKERR();

		glGetProgramiv(mShaderHandle, GL_LINK_STATUS, &status);
		if (!status) {
			glGetProgramInfoLog(mShaderHandle, 512, NULL, buffer);
			Log::LogPrintf("Shader linking failed: %d - %s\n", status, buffer);
			mIsValid = false;
		}
		else
			Log::LogPrintf("Shader linking succesful.\n");

		glDeleteShader(fragsh);
	}

	void Shader::Bind() {
		CHECKERR();
		assert(glIsProgram(mShaderHandle));
		if (mLastShader != mShaderHandle) {
			mLastShader = mShaderHandle;
			glUseProgram(mShaderHandle);
			CHECKERR();
		}
	}

	void Shader::SetUniform(uint32_t Uniform, int i)
	{
		glUniform1i(Uniform, i);
	}

	void Shader::SetUniform(uint32_t Uniform, float A, float B, float C, float D)
	{
		glUniform4f(Uniform, A, B, C, D);
	}

	void Shader::SetUniform(uint32_t Uniform, glm::vec2 Pos)
	{
		glUniform2f(Uniform, Pos.x, Pos.y);
	}

	void Shader::SetUniform(uint32_t Uniform, glm::vec3 Pos)
	{
		glUniform3f(Uniform, Pos.x, Pos.y, Pos.z);
	}

	void Shader::SetUniform(uint32_t Uniform, float F)
	{
		glUniform1f(Uniform, F);
	}

	void Shader::SetUniform(uint32_t Uniform, float *Matrix4x4)
	{
		glUniformMatrix4fv(Uniform, 1, GL_FALSE, Matrix4x4);
	}

	int Shader::EnableAttribArray(uint32_t Attrib)
	{
		glEnableVertexAttribArray(Attrib);
		return Attrib;
	}

	int Shader::DisableAttribArray(uint32_t Attrib)
	{
		glDisableVertexAttribArray(Attrib);
		return Attrib;
	}

	uint32_t Shader::GetUniform(std::string uni) {
		return glGetUniformLocation(mShaderHandle, uni.c_str());
	}

	bool Shader::IsValid()
	{
		return mIsValid;
	}

	uint32_t DefaultShader::GetUniform(uint32_t uni) {
		assert(uni < NUM_SHADERVARS);
		return uniforms[uni];
	}

	Shader::Shader() {
		mShaderHandle = -1;
		WindowFrame.AddShader(this);
	}

	Shader::~Shader() {
		glDeleteProgram(mShaderHandle);
		WindowFrame.RemoveShader(this);
	}
}