#pragma once



namespace Renderer {

	enum DefaultShaderVars
	{
		A_POSITION,
		A_UV,
		A_COLOR,
		U_MVP,
		U_CENTERED,
		U_COLOR,
		U_INVERT,
		U_HIDDEN,
		U_HIDCENTER,
		U_HIDSIZE,
		U_HIDFLSIZE,
		U_REPCOLOR,
		U_BTRANSP,
		NUM_SHADERVARS
	};

	class Shader {
		int mShaderHandle;
		bool mIsValid;
	protected:
		static int mLastShader;
	public:
		Shader();
		~Shader();

		virtual void Bind();
		void Compile(std::string frag);
		void CompileFull(std::string frag, std::string vert);

		uint32_t GetUniform(std::string uni);

		bool IsValid();

		static void SetUniform(uint32_t Uniform, int i);

		static void SetUniform(uint32_t Uniform, float F);
		static void SetUniform(uint32_t Uniform, glm::vec2 vec);
		static void SetUniform(uint32_t Uniform, glm::vec3 vec);
		static void SetUniform(uint32_t Uniform, float A, float B, float C, float D);
		
		static void SetUniform(uint32_t Uniform, float *Matrix4x4);

		static int EnableAttribArray(uint32_t attrib);
		static int DisableAttribArray(uint32_t attrib);
		
	};

	class DefaultShader : public Shader {
		static int mFragProgram, mVertProgram, mProgram;
		static uint32_t uniforms[NUM_SHADERVARS];
	public:
		static bool Compile();
		static void UpdateProjection(Mat4 proj);
		static void StaticBind();
		static uint32_t GetUniform(uint32_t uni);

		static void SetColor(float r, float g, float b, float a);

		static int GetVertexShader();
	};
}
