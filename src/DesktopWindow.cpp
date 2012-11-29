#include "DesktopWindow.h"

DesktopWindow::DesktopWindow(DesktopWindowClient* client, int width, int height)
    : m_client(client), m_size(WKSizeMake(width, height))
{
}

DesktopWindow::~DesktopWindow()
{
}
