#include <Rocket/Core.h>
#include <Rocket/Core/SystemInterface.h>
#include <Rocket/Core/FileInterface.h>
#include <Rocket/Core/RenderInterface.h>
#include <Rocket/Controls.h>
#include <Rocket/Core/Lua/Interpreter.h>
#include <Rocket/Controls/Lua/Controls.h>
#include <Rocket/Debugger.h>

#include <GL/GLEW.h>
#include <GLFW/glfw3.h>
#include "GameGlobal.h"
#include "GameState.h"
#include "VBO.h"
#include "Rendering.h"
#include "ImageLoader.h"
#include "GameWindow.h"
#include "RaindropRocketInterface.h"

namespace Engine { namespace RocketInterface {

	float SystemInterface::GetElapsedTime()
	{
		return glfwGetTime();
	}

	void RenderInterface::EnableScissorRegion(bool enable)
	{
		if (enable) glEnable(GL_SCISSOR_TEST);
		else glDisable(GL_SCISSOR_TEST);
	}

	void RenderInterface::SetScissorRegion(int x, int y, int width, int height)
	{
		glScissor(x, y, width, height);
	}

	struct pointf
	{
		float x, y;
	};

	struct colf
	{
		float r, g, b, a;
	};

	void RenderInterface::RenderGeometry(Rocket::Core::Vertex* vertices, int num_vertices, int* indices, int num_indices, Rocket::Core::TextureHandle texture, const Rocket::Core::Vector2f& translation)
	{
		Rocket::Core::CompiledGeometryHandle Handle = CompileGeometry(vertices, num_vertices, indices, num_indices, texture);
		RenderCompiledGeometry(Handle, translation);
		ReleaseCompiledGeometry(Handle);
	}

	// Called by Rocket when it wants to compile geometry it believes will be static for the forseeable future.
	Rocket::Core::CompiledGeometryHandle RenderInterface::CompileGeometry(Rocket::Core::Vertex* vertices, int num_vertices, int* indices, int num_indices, Rocket::Core::TextureHandle texture)
	{
		std::vector <pointf> posv, uvv;
		std::vector <colf> colv;

		// reorder vertices
		for (int i = 0; i < num_vertices; i++)
		{
			pointf p = { vertices[i].position.x, vertices[i].position.y };
			pointf u = { vertices[i].tex_coord.x, vertices[i].tex_coord.y };
			colf   c = { vertices[i].colour.red / 255.0, vertices[i].colour.green / 255.0, 
				vertices[i].colour.blue / 255.0, vertices[i].colour.alpha / 255.0 }; // normalize to float
			posv.push_back(p);
			uvv.push_back(u);
			colv.push_back(c);
		}

		// create VBOs
		InternalGeometryHandle *Handle = new InternalGeometryHandle;
		Handle->vert = new VBO(VBO::Static, num_vertices * 2); // x/y
		Handle->uv = new VBO(VBO::Static, num_vertices * 2); // u/v
		Handle->color = new VBO(VBO::Static, num_vertices * 4); // r/g/b/a
		Handle->indices = new VBO(VBO::Static, num_indices, sizeof(int), VBO::IndexBuffer);

		// assign them data
		Handle->vert->AssignData(posv.data()); // address of first member should be enough
		Handle->uv->AssignData(uvv.data());
		Handle->color->AssignData(colv.data());
		Handle->indices->AssignData(indices);

		// assign texture
		Handle->tex = (Image*) texture;

		// give it some useful data
		Handle->num_vertices = num_vertices;
		Handle->num_indices = num_indices;

		// we're done
		return (Rocket::Core::CompiledGeometryHandle) Handle;
	}

	// Called by Rocket when it wants to render application-compiled geometry.
	void RenderInterface::RenderCompiledGeometry(Rocket::Core::CompiledGeometryHandle geometry, const Rocket::Core::Vector2f& translation)
	{
		InternalGeometryHandle *Handle = (InternalGeometryHandle*) geometry;
		Mat4 tMatrix = glm::translate(Mat4(), Vec3(translation.x, translation.y, 0));
		
		glDisable(GL_DEPTH_TEST);
		SetShaderParameters(false, false, false);
		WindowFrame.SetUniform(U_COLOR, 1, 1, 1, 1);
		WindowFrame.SetUniform(U_MVP, &(tMatrix[0][0])); 
		
		// bind texture
		Handle->tex->Bind();

		// bind VBOs
		Handle->vert->Bind();
		glVertexAttribPointer(WindowFrame.EnableAttribArray(A_POSITION), 2, GL_FLOAT, GL_FALSE, sizeof(float) * 2, 0);

		Handle->uv->Bind();
		glVertexAttribPointer(WindowFrame.EnableAttribArray(A_UV), 2, GL_FLOAT, GL_FALSE, sizeof(float) * 2, 0);

		Handle->indices->Bind();
		glDrawElements(GL_TRIANGLES, Handle->num_indices, GL_UNSIGNED_INT, 0);

		// missing: colour VBO

		WindowFrame.DisableAttribArray(A_POSITION);
		WindowFrame.DisableAttribArray(A_UV);
		glEnable(GL_DEPTH_TEST);
	}

	// Called by Rocket when it wants to release application-compiled geometry.
	void RenderInterface::ReleaseCompiledGeometry(Rocket::Core::CompiledGeometryHandle geometry)
	{
		InternalGeometryHandle *Handle = (InternalGeometryHandle*)geometry;
		delete Handle->vert;
		delete Handle->uv;
		delete Handle->color;
		delete Handle->indices;
		delete Handle; // texture is handled by rocket
	}

	bool RenderInterface::LoadTexture(Rocket::Core::TextureHandle& texture_handle,
		Rocket::Core::Vector2i& texture_dimensions,
		const Rocket::Core::String& source)
	{
		ImageData Data = ImageLoader::GetDataForImage(GameState::GetInstance().GetSkinFile(source.CString()));
		Image *Ret = new Image();

		texture_handle = (Rocket::Core::TextureHandle) Ret;

		texture_dimensions.x = Data.Width;
		texture_dimensions.y = Data.Height;

		return GenerateTexture(texture_handle, Data.Data, Rocket::Core::Vector2i(Data.Width, Data.Height));
	}

	bool RenderInterface::GenerateTexture(Rocket::Core::TextureHandle& texture_handle,
		const Rocket::Core::byte* source,
		const Rocket::Core::Vector2i& source_dimensions)
	{
		Image* Ret = (Image*)texture_handle;
		ImageData Data;
		Data.Data = (unsigned char*)source;
		Data.Width = source_dimensions.x;
		Data.Height = source_dimensions.y;
		Ret->SetTextureData(&Data);
		return Data.Data != NULL;
	}

	void RenderInterface::ReleaseTexture(Rocket::Core::TextureHandle texture_handle)
	{
		delete (Image*)texture_handle;
	}
	
	Rocket::Core::FileHandle FileSystemInterface::Open(const Rocket::Core::String& path)
	{
		Rocket::Core::String npath = Rocket::Core::String((GameState::GetInstance().GetSkinPrefix() + path.CString()).c_str());
#ifdef WIN32
		return (Rocket::Core::FileHandle) _wfopen(Utility::Widen(npath.CString()).c_str(), L"r");
#else
		return (Rocket::Core::FileHandle) fopen(npath.CString(), "r");
#endif
	}
	
	void FileSystemInterface::Close(Rocket::Core::FileHandle file)
	{
		fclose((FILE*)file);
	}

	
	size_t FileSystemInterface::Read(void* buffer, size_t size, Rocket::Core::FileHandle file)
	{
		return fread(buffer, 1, size, (FILE*)file);
	}

	
	bool FileSystemInterface::Seek(Rocket::Core::FileHandle file, long offset, int origin)
	{
		return fseek((FILE*)file, offset, origin) != 0;
	}

	
	size_t FileSystemInterface::Tell(Rocket::Core::FileHandle file)
	{
		return ftell((FILE*)file);
	}
} 

	void SetupRocket()
	{
		Rocket::Core::SetFileInterface(new Engine::RocketInterface::FileSystemInterface);
		Rocket::Core::SetRenderInterface(new Engine::RocketInterface::RenderInterface);
		Rocket::Core::SetSystemInterface(new Engine::RocketInterface::SystemInterface);

		Rocket::Core::Initialise();
		Rocket::Controls::Initialise();
		Rocket::Core::Lua::Interpreter::Initialise();
		// Rocket::Controls::Lua::RegisterTypes(Rocket::Core::Lua::Interpreter::GetLuaState());

		Rocket::Core::FontDatabase::LoadFontFace("font.ttf");

		GameState::GetInstance().InitializeLua(Rocket::Core::Lua::Interpreter::GetLuaState());
	}

	void SetupRocketLua(void* State)
	{
		Rocket::Core::Lua::Interpreter::RegisterCoreTypes((lua_State*)State);
		Rocket::Controls::Lua::RegisterTypes((lua_State*)State);
	}
}