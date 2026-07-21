#pragma once

/* Fixed 2D VBO of 6 points. To be expanded upon later. */
class VBO
{
public:

    enum Type : uint8_t
    {
        Static,
        Dynamic,
        Stream
    };

    enum IdxKind : uint8_t
    {
        ArrayBuffer,
        IndexBuffer
    };

private:

    static uint32_t last_bound_;
    static uint32_t last_bound_index_;

    std::unique_ptr<uint8_t[]> vbo_data_;

    /* GLuint */
    uint32_t internal_vbo_;
    uint32_t element_count_;
    uint32_t element_size_;

    IdxKind m_kind_;
    Type m_type_;

    bool is_valid_;

public:
    VBO(Type t, uint32_t elements, uint32_t size = sizeof(float), IdxKind kind = ArrayBuffer);
    ~VBO();
    void invalidate();
    void validate();
    void bind(bool force = false) const;
    uint32_t get_element_count() const;

    void upload_to_gpu();

    /* Size must be valid with parameters given to VBO. */
    void assign(const void *data);
};