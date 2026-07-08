#pragma once



namespace renderer {

	enum DefaultShaderVars
	{
		A_POSITION,
		A_UV,
		A_COLOR,
		U_MODELVIEW,
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
		bool mIsValid{};
	protected:
		static int m_last_shader_;
	public:
		Shader();
		~Shader();

		virtual void bind();
		void compile(const std::string& frag);
		void CompileFull(std::string frag, std::string vert);

		uint32_t get_uniform(const std::string& uni) const;

		bool is_valid() const;

		static void set_uniform(uint32_t uniform, int i);

		static void set_uniform(uint32_t Uniform, float F);
		static void set_uniform(uint32_t uniform, glm::vec2 vec);
		static void set_uniform(uint32_t uniform, glm::vec3 vec);
		static void set_uniform(uint32_t uniform, float A, float B, float C, float D);
		
		static void set_uniform(uint32_t uniform, const float *matrix4_x4);

		static int enable_attrib_array(uint32_t attrib);
		static int disable_attrib_array(uint32_t attrib);
		
	};

	class DefaultShader : public Shader {
		static int mFragProgram, mVertProgram, mProgram;
		static uint32_t uniforms[NUM_SHADERVARS];
	public:
		static bool compile();
		static void update_projection(Mat4 proj);
		static void static_bind();
		static uint32_t get_uniform(uint32_t uni);

		static void set_color(float r, float g, float b, float a);

		static int get_vertex_shader();
	};
}
