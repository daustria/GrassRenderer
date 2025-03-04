#ifndef BLADE_H
#define BLADE_H
#include <GLFW/glfw3.h>
#include <cstdlib>
#include <glad/glad.h>
#include <glm/ext/scalar_constants.hpp>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>

#define PI glm::pi<float>()

#define RANDOM_OFFSET (float)std::rand() / RAND_MAX

struct Blade {
    // Position on the geometry. w: orientation
    glm::vec4 v0;
    // Bezier curve guide above v0 with respect to up vector, w: height
    glm::vec4 v1;
    // Physical model guide. w: width
    glm::vec4 v2;
    // The 'up' vector, w: stiffness, affects force computations on the blade.
    glm::vec4 up;

    Blade(const glm::vec3& position = glm::vec3(0.0f, 0.0f, 0.0f),
        const glm::vec3& up_vector = glm::vec3(0.0f, 1.0f, 0.0f),
        const float angle = PI / 2,
        const float h = 0.6f,
        const float w = 0.1f)
    {
        // For now we'll have simple values for these.
        float stiffness = 0.6f;

        glm::vec3 height_tmp = h * glm::normalize(up_vector);

        v0 = glm::vec4(position, angle);
        v1 = glm::vec4(position + height_tmp, h);
        v2 = glm::vec4(position + height_tmp, w);
        up = glm::vec4(height_tmp, stiffness);
    }
};

#endif // BLADE_H