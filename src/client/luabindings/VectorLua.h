
#ifndef RAINDROP_VECTORLUA_H
#define RAINDROP_VECTORLUA_H

/* wrapper around glm vectors for lua. very basic */
class VectorLua {
    glm::vec2 vec2{};
public:
    VectorLua(float x = 0, float y = 0);
    explicit VectorLua(glm::vec2 vec);
    VectorLua add(VectorLua v);
    VectorLua sub(VectorLua v);
    float dot(VectorLua v);
    VectorLua scale(float v);
    VectorLua normalized();
    void setX(float x);
    void setY(float y);
    float getX() const;
    float getY() const;
};


#endif //RAINDROP_VECTORLUA_H
