#include "DrawCallSink.h"

#include <algorithm>
#include <filesystem>

#include "Texture2D.h"
#include "VBO.h"
#include "Shader.h"

#include <GL/glew.h>

DrawCallSink::~DrawCallSink() = default;

DrawCallSink::Handle DrawCallSink::allocate() {
    if (!free_calls_.empty()) {
        auto handle = free_calls_.back();
        free_calls_.pop_back();
        return handle;
    }
    if (buckets_.empty() || buckets_.back()->next_slot == BucketSize)
        buckets_.push_back(std::make_unique<Bucket>());
    auto &bucket = *buckets_.back();
    return {buckets_.size() - 1, bucket.next_slot++};
}

void DrawCallSink::release(const Handle handle) {
    auto &bucket = *buckets_[handle.bucket];
    const auto slot = handle.slot;
    bucket.drawn[slot] = false;
    bucket.quads[slot] = {};
    bucket.quad_textures[slot] = nullptr;
    bucket.quad_has_models[slot] = false;
    bucket.quad_scissors[slot] = false;
    bucket.fonts[slot] = nullptr;
    bucket.strings[slot].clear();
    bucket.string_scissors[slot] = false;
    free_calls_.push_back(handle);
}

void DrawCallSink::begin_frame() {
    for (auto &layer : layers_) {
        for (auto it = layer.begin(); it != layer.end();) {
            if (buckets_[it->bucket]->drawn[it->slot]) {
                release(*it);
                it = layer.erase(it);
            } else ++it;
        }
    }
}

void DrawCallSink::set_clip(const bool enabled, const AABB &region, const bool window_coordinates) {
    clip_enabled_ = enabled;
    clip_window_coordinates_ = window_coordinates;
    clip_region_ = region;
}

void DrawCallSink::queue(const uint32_t z, const Handle handle) {
    layers_[std::min(z, LayerCount - 1)].push_back(handle);
}

void DrawCallSink::submit_quad(const uint32_t z, const renderer::QuadDrawParams &params,
                               Texture2D *texture, const bool scissor, const AABB &region) {
    const auto handle = allocate();
    auto &bucket = *buckets_[handle.bucket];
    const auto slot = handle.slot;
    bucket.types[slot] = Type::Quad;
    bucket.quads[slot] = params;
    bucket.quad_textures[slot] = texture;
    bucket.quad_has_models[slot] = params.model != nullptr;
    if (params.model) bucket.quad_models[slot] = *params.model;
    bucket.quad_scissors[slot] = clip_enabled_ || scissor;
    bucket.quad_scissor_regions[slot] = clip_enabled_ ? clip_region_ : region;
    bucket.quad_scissor_windows[slot] = clip_enabled_ && clip_window_coordinates_;
    queue(z, handle);
}

void DrawCallSink::submit_string(const uint32_t z, Font *font, std::string text,
                                 const Vec2 &position, const Mat4 &transform, const Vec2 &scale,
                                 const ColorRGBA &color,
                                 const bool scissor, const AABB &region) {
    if (!font) return;
    const auto handle = allocate();
    auto &bucket = *buckets_[handle.bucket];
    const auto slot = handle.slot;
    bucket.types[slot] = Type::String;
    bucket.fonts[slot] = font;
    bucket.strings[slot] = std::move(text);
    bucket.string_positions[slot] = position;
    bucket.string_transforms[slot] = transform;
    bucket.string_scales[slot] = scale;
    bucket.string_colors[slot] = color;
    bucket.string_scissors[slot] = clip_enabled_ || scissor;
    bucket.string_scissor_regions[slot] = clip_enabled_ ? clip_region_ : region;
    bucket.string_scissor_windows[slot] = clip_enabled_ && clip_window_coordinates_;
    queue(z, handle);
}

void DrawCallSink::submit_line(const uint32_t z, const Vec2 &start, const Vec2 &end, const ColorRGBA &color) {
    const auto handle = allocate();
    auto &bucket = *buckets_[handle.bucket];
    bucket.types[handle.slot] = Type::Line;
    bucket.line_starts[handle.slot] = start;
    bucket.line_ends[handle.slot] = end;
    bucket.line_colors[handle.slot] = color;
    queue(z, handle);
}

void DrawCallSink::draw(const Handle handle) {
    auto &bucket = *buckets_[handle.bucket];
    const auto slot = handle.slot;
    if (bucket.drawn[slot]) return;
    if (bucket.types[slot] == Type::Quad) {
        if (bucket.quad_textures[slot]) bucket.quad_textures[slot]->bind();
        renderer::set_scissor(bucket.quad_scissors[slot]);
        if (bucket.quad_scissors[slot]) {
            const auto &r = bucket.quad_scissor_regions[slot];
            if (bucket.quad_scissor_windows[slot]) renderer::set_scissor_region_wnd(r.X1, r.Y1, r.width(), r.height());
            else renderer::set_scissor_region(r.X1, r.Y1, r.width(), r.height());
        }
        if (bucket.quad_has_models[slot]) bucket.quads[slot].model = &bucket.quad_models[slot];
        renderer::draw_quad(bucket.quads[slot]);
    } else if (bucket.types[slot] == Type::String) {
        renderer::set_scissor(bucket.string_scissors[slot]);
        if (bucket.string_scissors[slot]) {
            const auto &r = bucket.string_scissor_regions[slot];
            if (bucket.string_scissor_windows[slot]) renderer::set_scissor_region_wnd(r.X1, r.Y1, r.width(), r.height());
            else renderer::set_scissor_region(r.X1, r.Y1, r.width(), r.height());
        }
        auto *font = bucket.fonts[slot];
        font->set_color(bucket.string_colors[slot].red, bucket.string_colors[slot].green, bucket.string_colors[slot].blue);
        font->set_alpha(bucket.string_colors[slot].alpha);
        font->render(bucket.strings[slot], bucket.string_positions[slot], bucket.string_transforms[slot], bucket.string_scales[slot]);
    } else {
        if (!line_vbo_) line_vbo_ = std::make_unique<VBO>(VBO::Stream, 4);
        const float points[] = {bucket.line_starts[slot].x, bucket.line_starts[slot].y,
                                bucket.line_ends[slot].x, bucket.line_ends[slot].y};
        line_vbo_->assign(points);
        constexpr auto identity = glm::identity<Mat4>();
        renderer::set_default_shader_parameters(true, false, false, false);
		renderer::Shader::Default::set_color(bucket.line_colors[slot].red, bucket.line_colors[slot].green,
                                           bucket.line_colors[slot].blue, bucket.line_colors[slot].alpha);
		renderer::Shader::set_uniform(renderer::Shader::Default::get_uniform(renderer::U_MODELVIEW), &identity[0][0]);
        line_vbo_->bind();
		glVertexAttribPointer(renderer::Shader::enable_attrib_array(renderer::Shader::Default::get_uniform(renderer::A_POSITION)),
                              2, GL_FLOAT, GL_FALSE, 0, nullptr);
        glDrawArrays(GL_LINES, 0, 2);
		renderer::Shader::disable_attrib_array(renderer::Shader::Default::get_uniform(renderer::A_POSITION));
        Texture2D::force_rebind();
    }
    bucket.drawn[slot] = true;
}

void DrawCallSink::flush() {
    for (auto &layer : layers_) {
        const auto count = layer.size();
        for (size_t i = 0; i < count; ++i) draw(layer[i]);
    }
}
