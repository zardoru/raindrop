#ifndef RROCKETI_H_
#define RROCKETI_H_

class VBO;

namespace Engine {

namespace RocketInterface {

	/*
		To Do: multiple-inherit from raindrop's own system and fileinterface 
		instead of implementing it all here.
	*/ 
	class FileSystemInterface : Rocket::Core::FileInterface { 
	public:
		// Opens a file.
		Rocket::Core::FileHandle Open(const Rocket::Core::String& path);

		// Closes a previously opened file.
		void Close(Rocket::Core::FileHandle file);

		// Reads data from a previously opened file.
		size_t Read(void* buffer, size_t size, Rocket::Core::FileHandle file);

		// Seeks to a point in a previously opened file.
		bool Seek(Rocket::Core::FileHandle file, long offset, int origin);

		// Returns the current position of the file pointer.
		size_t Tell(Rocket::Core::FileHandle file);
	};

	class SystemInterface : Rocket::Core::SystemInterface {
	public:
		float GetElapsedTime();
	};

	class RenderInterface : Rocket::Core::RenderInterface {

		struct InternalGeometryHandle {
			VBO *vert;
			VBO *uv;
			VBO *color;
			Image* tex;
			int *indices;
			int num_indices, num_vertices;
		};

	public:

		// Called by Rocket when it wants to compile geometry it believes will be static for the forseeable future.
		Rocket::Core::CompiledGeometryHandle CompileGeometry(Rocket::Core::Vertex* vertices, int num_vertices, int* indices, int num_indices, Rocket::Core::TextureHandle texture);

		// Called by Rocket when it wants to render application-compiled geometry.
		void RenderCompiledGeometry(Rocket::Core::CompiledGeometryHandle geometry, const Rocket::Core::Vector2f& translation);

		// Called by Rocket when it wants to release application-compiled geometry.
		void ReleaseCompiledGeometry(Rocket::Core::CompiledGeometryHandle geometry);

		// Called by Rocket when it wants to render geometry that the application does not wish to optimise.
		void RenderGeometry(Rocket::Core::Vertex* vertices, int num_vertices, int* indices, int num_indices, Rocket::Core::TextureHandle texture, const Rocket::Core::Vector2f& translation);

		// Called by Rocket when it wants to enable or disable scissoring to clip content.
		void EnableScissorRegion(bool enable);

		// Called by Rocket when it wants to change the scissor region.
		void SetScissorRegion(int x, int y, int width, int height);

		// Called by Rocket when a texture is required by the library.
		bool LoadTexture(Rocket::Core::TextureHandle& texture_handle,
			Rocket::Core::Vector2i& texture_dimensions,
			const Rocket::Core::String& source);

		// Called by Rocket when a texture is required to be built from an internally-generated sequence of pixels.
		bool GenerateTexture(Rocket::Core::TextureHandle& texture_handle,
			const Rocket::Core::byte* source,
			const Rocket::Core::Vector2i& source_dimensions);

		// Called by Rocket when a loaded texture is no longer required.
		void ReleaseTexture(Rocket::Core::TextureHandle texture_handle);
	};
}

}

#endif