#include "LinuxWindow.h"
#include <X11/Xutil.h>
#include <GL/glx.h>

struct LinuxWindow::GLContextData {
    XVisualInfo* visualInfo;
    GLXContext context;
};

VisualID LinuxWindow::setupXVisualID()
{
    m_glContextData = new GLContextData;

    GLint attributes[] = {
        GLX_RGBA,
        GLX_DEPTH_SIZE, 24,
        GLX_DOUBLEBUFFER,
        None
    };
    m_glContextData->visualInfo = glXChooseVisual(m_display, 0, attributes);

    if (!m_glContextData->visualInfo)
        fatalError("glXChooseVisual() failed.");

    return m_glContextData->visualInfo->visualid;
}

void LinuxWindow::createGLContext()
{
    m_glContextData->context = glXCreateContext(m_display, m_glContextData->visualInfo, 0, GL_TRUE);
    if (!m_glContextData->context)
        fatalError("glXCreateContext() failed.");
}

void LinuxWindow::destroyGLContext()
{
    glXMakeCurrent(m_display, None, 0);
    glXDestroyContext(m_display, m_glContextData->context);
    XFree(m_glContextData->visualInfo);
    delete m_glContextData;
}

void LinuxWindow::makeCurrent()
{
    glXMakeCurrent(m_display, m_window, m_glContextData->context);
}

void LinuxWindow::swapBuffers()
{
    glXSwapBuffers(m_display, m_window);
}
