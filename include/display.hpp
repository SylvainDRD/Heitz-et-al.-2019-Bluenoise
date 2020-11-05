#pragma once

#include <utils.hpp>


class Display {
public:
    /// \brief Constructor for the display. DIsplay the integration result for each sequence of the mask.
    /// \param displayTexture The OpenGL 3D texture ID of the integration results.
    Display(GLuint displayTexture);

    /// \brief Free the GL ressources before the destruction of the object.
    /// \note This is required because otherwise the context will be destroyed before the ressources are freed.
    void freeGLRessources();

    /// \brief Draw the integration results of the 2D gaussian for the first 2 dimensions.
    void draw() const;

private:
    GLuint m_program;

    GLuint m_screenquadVAO;

    void generateScreenquad();
};
