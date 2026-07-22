#pragma once

#include <string>

#include <glm.h>
#include <rmath.h>

namespace renderer {

	enum ShaderAttribute : int
	{
		A_POSITION,
		A_UV,
		A_COLOR
	};

	class Shader {
		int mShaderHandle;
		bool mIsValid{};
	protected:
		static int m_last_shader_;
		int uniform_location(const std::string& name) const;
	public:
		struct DrawState {
			const Mat4 &projection;
			const Mat4 *model;
			bool centered;
			ColorRGBA color;
		};

		class Default;
		class Note;
		class BGA;
		class SDF;

		Shader();
		virtual ~Shader();

		virtual void bind();
		void compile(const std::string& frag);
		void CompileFull(std::string frag, std::string vert);

		int get_uniform(const std::string& uni) const;
		virtual void apply_draw_state(const DrawState &state) const = 0;

		bool is_valid() const;

		static void set_uniform(int uniform, int i);

		static void set_uniform(int Uniform, float F);
		static void set_uniform(int uniform, glm::vec2 vec);
		static void set_uniform(int uniform, glm::vec3 vec);
		static void set_uniform(int uniform, float A, float B, float C, float D);
		
		static void set_uniform(int uniform, const float *matrix4_x4);

		static int enable_attrib_array(int attrib);
		static int disable_attrib_array(int attrib);
		
	};

	class Shader::Default : public Shader {
	public:
		enum class Uniform { Projection, ModelView, Centered, Color, Count };
	private:
		static int mFragProgram, mVertProgram, mProgram;
		static int uniforms[static_cast<int>(Uniform::Count)];
	public:
		static bool compile();
		static void update_projection(Mat4 proj);
		static void static_bind();
		static int uniform(Uniform uniform);

		static void set_color(float r, float g, float b, float a);

		static int get_vertex_shader();
	};

	class Shader::Note : public Shader {
		struct Locations {
			int projection;
			int model_view;
			int centered;
			int color;
			int hidden_mode;
			int hidden_center;
			int hidden_size;
			int flashlight_size;
		} locations_{};

		void cache_locations();
	public:
		Note();
		void apply_draw_state(const DrawState &state) const override;
		void set_hidden_effect(int mode, float center, float transition_size, float flashlight_size);
	};

	class Shader::BGA : public Shader {
		struct Locations {
			int projection;
			int model_view;
			int centered;
			int color;
		} locations_{};

		void cache_locations();
	public:
		BGA();
		void apply_draw_state(const DrawState &state) const override;
	};

	class Shader::SDF : public Shader {
		struct Locations {
			int projection;
			int model_view;
			int centered;
			int color;
		} locations_{};

		void cache_locations();
	public:
		SDF();
		void apply_draw_state(const DrawState &state) const override;
	};
}
