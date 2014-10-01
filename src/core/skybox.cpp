#include "core/skybox.h"

skybox::skybox()
    : m_shader("skybox/skybox.vert", "skybox/skybox.frag")
{
    quad = gluNewQuadric();
}

void skybox::display(const camera& cam, glm::vec3 skycolor)
{
    m_shader.bind();

    m_shader.set("view", cam.view(false));
    m_shader.set("proj", cam.proj());
    //glBegin();
    gluSphere(quad, 100, 32, 32);
    //glEnd();

    m_shader.unbind();
}
