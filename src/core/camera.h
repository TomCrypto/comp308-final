#ifndef CAMERA_H
#define CAMERA_H

#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <glm/glm.hpp>

class camera
{
public:
    camera(const glm::ivec2& dims,
           const glm::vec3& position,
           const glm::vec3& direction,
           float fov);
    camera() {};

    void resize(const glm::ivec2& dims);

    // move relative to current direction
    // e.g. (0, 0, 1) moves forward
    void move(const glm::vec3& direction);

    // change in euler angles
    void turn(const glm::vec2& delta);

    glm::mat4 view() const;
    glm::mat4 proj() const;

private:
    glm::ivec2 m_dims;
    glm::vec3 m_pos;
    glm::vec3 m_dir;
    float m_fov;
};

#endif
