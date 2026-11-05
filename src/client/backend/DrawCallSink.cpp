#include "DrawCallSink.h"

#include <algorithm>
#include <cassert>
#include <cstring>
#include <filesystem>

#include "Texture2D.h"
#include "VBO.h"
#include "Shader.h"

#include <GL/glew.h>

DrawCallSink::~DrawCallSink() = default;

void DrawCallSink::reset() {
    // clear() keeps each vector's capacity, so steady-state frames don't allocate
    // (beyond the strings handed in by value).
    quads_.clear();
    strings_.clear();
    lines_.clear();
    for (auto &layer : layers_) layer.clear();
}

void DrawCallSink::begin_frame() {
    // Anything still queued here was submitted after the last flush. Drop it rather than
    // drawing it a frame late with pointers that may no longer be valid.
    reset();
}

void DrawCallSink::set_clip(const bool enabled, const AABB &region, const bool window_coordinates) {
    clip_.enabled = enabled;
    clip_.region = region;
    clip_.window_coordinates = window_coordinates;
}

DrawCallSink::Scissor DrawCallSink::resolve_scissor(const bool scissor, const AABB &region) const {
    if (clip_.enabled) return clip_;
    return {region, scissor, false};
}

void DrawCallSink::queue(const uint32_t z, const Type type, const size_t index) {
    assert(index <= IndexMask);
    layers_[std::min(z, LayerCount - 1)].push_back(
        static_cast<uint32_t>(type) << TypeShift | static_cast<uint32_t>(index));
}

void DrawCallSink::submit_quad(const uint32_t z, const renderer::QuadDrawParams &params,
                               Texture2D *texture, const bool scissor, const AABB &region) {
    auto &cmd = quads_.emplace_back();
    cmd.params = params;
    cmd.params.model = nullptr;
    cmd.texture = texture;
    cmd.has_model = params.model != nullptr;
    if (params.model) cmd.model = *params.model;

    // UV buffers are mutable and shared (a sprite rewrites its own when its crop changes),
    // so snapshot the contents now instead of keeping the pointer.
    const auto *uv = params.texture_coordinates;
    cmd.has_uvs = uv && uv->get_element_count() == QuadUVCount && uv->get_element_size() == sizeof(float);
    if (cmd.has_uvs) {
        std::memcpy(cmd.uvs.data(), uv->get_data(), sizeof(UVs));
        cmd.params.texture_coordinates = nullptr;
    }

    cmd.scissor = resolve_scissor(scissor, region);
    queue(z, Type::Quad, quads_.size() - 1);
}

void DrawCallSink::submit_string(const uint32_t z, Font *font, std::string text,
                                 const Vec2 &position, const Mat4 &transform, const Vec2 &scale,
                                 const ColorRGBA &color,
                                 const bool scissor, const AABB &region) {
    if (!font) return;
    strings_.push_back({font, std::move(text), position, transform, scale, color,
                        resolve_scissor(scissor, region)});
    queue(z, Type::String, strings_.size() - 1);
}

void DrawCallSink::submit_line(const uint32_t z, const Vec2 &start, const Vec2 &end, const ColorRGBA &color) {
    lines_.push_back({start, end, color});
    queue(z, Type::Line, lines_.size() - 1);
}

VBO *DrawCallSink::upload_uvs(const UVs &uvs) {
    if (!uv_vbo_) uv_vbo_ = std::make_unique<VBO>(VBO::Stream, QuadUVCount);
    // Re-creates the GL buffer from the CPU mirror if the context invalidated it.
    uv_vbo_->validate();
    // Consecutive quads usually share a crop; skip redundant uploads.
    if (!uv_vbo_loaded_ || uploaded_uvs_ != uvs) {
        uv_vbo_->assign(uvs.data());
        uploaded_uvs_ = uvs;
        uv_vbo_loaded_ = true;
    }
    return uv_vbo_.get();
}

static void apply_scissor(const bool enabled, const AABB &r, const bool window_coordinates) {
    renderer::set_scissor(enabled);
    if (!enabled) return;
    if (window_coordinates) renderer::set_scissor_region_wnd(r.X1, r.Y1, r.width(), r.height());
    else renderer::set_scissor_region(r.X1, r.Y1, r.width(), r.height());
}

void DrawCallSink::draw(const QuadCommand &cmd) {
    if (cmd.texture) cmd.texture->bind();
    apply_scissor(cmd.scissor.enabled, cmd.scissor.region, cmd.scissor.window_coordinates);
    auto params = cmd.params;
    if (cmd.has_model) params.model = &cmd.model;
    if (cmd.has_uvs) params.texture_coordinates = upload_uvs(cmd.uvs);
    renderer::draw_quad(params);
}

void DrawCallSink::draw(const StringCommand &cmd) {
    apply_scissor(cmd.scissor.enabled, cmd.scissor.region, cmd.scissor.window_coordinates);
    cmd.font->set_color(cmd.color.red, cmd.color.green, cmd.color.blue);
    cmd.font->set_alpha(cmd.color.alpha);
    cmd.font->render(cmd.text, cmd.position, cmd.transform, cmd.scale);
}

void DrawCallSink::draw(const LineCommand &cmd) {
    // Lines carry no scissor/blend state of their own; don't inherit the previous call's.
    renderer::set_scissor(false);
    renderer::set_blending_mode(BLEND_ALPHA);
    if (!line_vbo_) line_vbo_ = std::make_unique<VBO>(VBO::Stream, 4);
    const float points[] = {cmd.start.x, cmd.start.y, cmd.end.x, cmd.end.y};
    line_vbo_->assign(points);
    constexpr auto identity = glm::identity<Mat4>();
    renderer::set_default_shader_parameters(false);
    renderer::Shader::Default::set_color(cmd.color.red, cmd.color.green, cmd.color.blue, cmd.color.alpha);
    renderer::Shader::set_uniform(renderer::Shader::Default::uniform(renderer::Shader::Default::Uniform::ModelView), &identity[0][0]);
    line_vbo_->bind();
    glVertexAttribPointer(renderer::Shader::enable_attrib_array(renderer::A_POSITION),
                          2, GL_FLOAT, GL_FALSE, 0, nullptr);
    glDrawArrays(GL_LINES, 0, 2);
    renderer::Shader::disable_attrib_array(renderer::A_POSITION);
    Texture2D::force_rebind();
}

void DrawCallSink::flush() {
    if (quads_.empty() && strings_.empty() && lines_.empty()) return;
    for (const auto &layer : layers_) {
        for (const auto entry : layer) {
            const auto index = entry & IndexMask;
            switch (static_cast<Type>(entry >> TypeShift)) {
                case Type::Quad: draw(quads_[index]); break;
                case Type::String: draw(strings_[index]); break;
                case Type::Line: draw(lines_[index]); break;
            }
        }
    }
    // Don't leak per-call scissor state to whatever renders after us.
    renderer::set_scissor(false);
    reset();
}
