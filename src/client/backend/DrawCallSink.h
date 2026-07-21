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
class DrawCallSink
{
    static constexpr size_t BucketSize = 96;
    static constexpr uint32_t LayerCount = 16;

    enum class Type : uint8_t { Quad, String, Line };
    struct Handle { size_t bucket; size_t slot; };

    // 96 fits neatly on a 32kb cache
    struct Bucket {
        std::array<Type, BucketSize> types{};
        std::array<bool, BucketSize> drawn{};
        std::array<renderer::QuadDrawParams, BucketSize> quads{};
        std::array<Texture2D*, BucketSize> quad_textures{};
        std::array<Mat4, BucketSize> quad_models{};
        std::array<bool, BucketSize> quad_has_models{};
        std::array<bool, BucketSize> quad_scissors{};
        std::array<AABB, BucketSize> quad_scissor_regions{};
        std::array<bool, BucketSize> quad_scissor_windows{};
        std::array<Font*, BucketSize> fonts{};
        std::array<std::string, BucketSize> strings{};
        std::array<Vec2, BucketSize> string_positions{};
        std::array<Mat4, BucketSize> string_transforms{};
        std::array<Vec2, BucketSize> string_scales{};
        std::array<ColorRGBA, BucketSize> string_colors{};
        std::array<bool, BucketSize> string_scissors{};
        std::array<AABB, BucketSize> string_scissor_regions{};
        std::array<bool, BucketSize> string_scissor_windows{};
        std::array<Vec2, BucketSize> line_starts{};
        std::array<Vec2, BucketSize> line_ends{};
        std::array<ColorRGBA, BucketSize> line_colors{};
        size_t next_slot = 0;
    };

    std::vector<std::unique_ptr<Bucket>> buckets_;
    std::vector<Handle> free_calls_;
    std::array<std::vector<Handle>, LayerCount> layers_;
    std::unique_ptr<VBO> line_vbo_;
    bool clip_enabled_ = false;
    bool clip_window_coordinates_ = false;
    AABB clip_region_{};

    Handle allocate();
    void release(Handle handle);
    void queue(uint32_t z, Handle handle);
    void draw(Handle handle);

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
