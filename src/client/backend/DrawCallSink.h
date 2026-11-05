#pragma once

#include <array>
#include <cstdint>
#include <memory>
#include <string>
#include <vector>

#include "Font.h"
#include "Rendering.h"
#include "VBO.h"

class Texture2D;

// Owns the transient 2D draw queue. Producers emit immutable draw parameters;
// this class alone owns their lifetime, ordering, and renderer interaction.
//
// Lifetime: the queue is a per-frame arena. flush() draws everything queued and
// empties it; begin_frame() discards anything still pending. Nothing survives
// either call, so raw pointers handed in (textures, fonts, shaders) only need to
// outlive the next flush(). Mutable per-draw data (model matrix, UVs) is copied
// at submit time.
class DrawCallSink
{
    static constexpr uint32_t LayerCount = 32;
    static constexpr size_t QuadUVCount = 8;

    using UVs = std::array<float, QuadUVCount>;

    struct Scissor {
        AABB region{};
        bool enabled = false;
        bool window_coordinates = false;
    };

    struct QuadCommand {
        renderer::QuadDrawParams params; // model/texture_coordinates are filled in at draw time
        Mat4 model{};
        UVs uvs{};
        Texture2D *texture = nullptr;
        Scissor scissor;
        bool has_model = false;
        bool has_uvs = false;
    };

    struct StringCommand {
        Font *font = nullptr;
        std::string text;
        Vec2 position{};
        Mat4 transform{};
        Vec2 scale{};
        ColorRGBA color{};
        Scissor scissor;
    };

    struct LineCommand {
        Vec2 start{};
        Vec2 end{};
        ColorRGBA color{};
    };

    // Layer entries pack the command type into the top bits and its index into the rest.
    enum class Type : uint32_t { Quad = 0, String = 1, Line = 2 };
    static constexpr uint32_t TypeShift = 30;
    static constexpr uint32_t IndexMask = (1u << TypeShift) - 1;

    std::vector<QuadCommand> quads_;
    std::vector<StringCommand> strings_;
    std::vector<LineCommand> lines_;
    std::array<std::vector<uint32_t>, LayerCount> layers_;

    std::unique_ptr<VBO> line_vbo_;
    std::unique_ptr<VBO> uv_vbo_;
    UVs uploaded_uvs_{};
    bool uv_vbo_loaded_ = false;

    Scissor clip_;

    Scissor resolve_scissor(bool scissor, const AABB &region) const;
    void queue(uint32_t z, Type type, size_t index);
    void reset();
    void draw(const QuadCommand &cmd);
    void draw(const StringCommand &cmd);
    void draw(const LineCommand &cmd);
    VBO *upload_uvs(const UVs &uvs);

public:
    ~DrawCallSink();
    void begin_frame();
    void flush();
    void set_clip(bool enabled, const AABB &region = {}, bool window_coordinates = false);

    void submit_quad(uint32_t z, const renderer::QuadDrawParams &params, Texture2D *texture,
                     bool scissor, const AABB &scissor_region);
    void submit_string(uint32_t z, Font *font, std::string text,
                       const Vec2 &position, const Mat4 &transform, const Vec2 &scale,
                       const ColorRGBA &color,
                       bool scissor, const AABB &scissor_region);
    void submit_line(uint32_t z, const Vec2 &start, const Vec2 &end, const ColorRGBA &color);
};
