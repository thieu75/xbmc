/*
 *      Copyright (C) 2005-2008 Team XBMC
 *      http://www.xbmc.org
 *
 *  This Program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2, or (at your option)
 *  any later version.
 *
 *  This Program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with XBMC; see the file COPYING.  If not, write to
 *  the Free Software Foundation, 675 Mass Ave, Cambridge, MA 02139, USA.
 *  http://www.gnu.org/copyleft/gpl.html
 *
 */

#include "GUIExternalAppControl.h"
#include "windowing/WindowingFactory.h"
#include "utils/log.h"
#include "utils/MathUtils.h"
#include "Key.h"
#include "utils/CharsetConverter.h"
#include <GL/glew.h>
#include <X11/Xlib.h>
#include <X11/Xutil.h>
#include <GL/gl.h>
#include <GL/glx.h>
#include <X11/extensions/Xcomposite.h>

CGUIExternalAppControl::CGUIExternalAppControl()
 : m_pixmap_gl(None)
 , m_pixmap(None)
 , m_texture(0)
{
  m_glXBindTexImageEXT    = (PFNGLXBINDTEXIMAGEEXTPROC)glXGetProcAddress((GLubyte *) "glXBindTexImageEXT");
  m_glXReleaseTexImageEXT = (PFNGLXRELEASETEXIMAGEEXTPROC)glXGetProcAddress((GLubyte *) "glXReleaseTexImageEXT");

  if(m_glXBindTexImageEXT    == NULL
  || m_glXReleaseTexImageEXT == NULL)
    CLog::Log(LOGERROR, "CGUIExternalAppControl::CGUIExternalAppControl - failed to get glXBindTexImageEXT");

  memset(&m_attrib, 0, sizeof(m_attrib));
  glGenTextures(1, &m_texture);

}

CGUIExternalAppControl::~CGUIExternalAppControl()
{

}

void CGUIExternalAppControl::Dispose()
{

  if(m_pixmap_gl)
  {
    glXDestroyPixmap(g_Windowing.GetDisplay(), m_pixmap_gl);
    m_pixmap_gl = None;
  }

  if(m_pixmap)
  {
    XFreePixmap(g_Windowing.GetDisplay(), m_pixmap);
    m_pixmap = None;
  }

  if(m_texture)
  {
    glDeleteTextures(1, &m_texture);
    m_texture = 0;
  }
}


bool CGUIExternalAppControl::SetWindow(Window window)
{
  Dispose();

  if(m_glXBindTexImageEXT    == NULL
  || m_glXReleaseTexImageEXT == NULL)
    return false;

  m_display = g_Windowing.GetDisplay();
  m_screen  = DefaultScreen(m_display);


  XVisualInfo* visinfo;

  XGetWindowAttributes (m_display, window, &m_attrib);

  VisualID visualid = XVisualIDFromVisual (m_attrib.visual);

  int nfbconfigs, i;
  GLXFBConfig* fbconfigs = glXGetFBConfigs (m_display, m_screen, &nfbconfigs);
  for (i = 0; i < nfbconfigs; i++)
  {
    int value;
    visinfo = glXGetVisualFromFBConfig (m_display, fbconfigs[i]);
    if (!visinfo || visinfo->visualid != visualid)
      continue;

    glXGetFBConfigAttrib (m_display, fbconfigs[i], GLX_DRAWABLE_TYPE, &value);
    if (!(value & GLX_PIXMAP_BIT))
      continue;

    glXGetFBConfigAttrib (m_display, fbconfigs[i],
        GLX_BIND_TO_TEXTURE_TARGETS_EXT,
        &value);
    if (!(value & GLX_TEXTURE_2D_BIT_EXT))
      continue;

    glXGetFBConfigAttrib (m_display, fbconfigs[i],
        GLX_BIND_TO_TEXTURE_RGBA_EXT,
        &value);
    if (value == 0)
    {
      glXGetFBConfigAttrib (m_display, fbconfigs[i],
          GLX_BIND_TO_TEXTURE_RGB_EXT,
          &value);
      if (value == 0)
        continue;
    }

    glXGetFBConfigAttrib (m_display, fbconfigs[i],
        GLX_Y_INVERTED_EXT,
        &value);
    if (value)
    {
      m_top    = 0.0f;
      m_bottom = 1.0f;
    }
    else
    {
      m_top    = 1.0f;
      m_bottom = 0.0f;
    }

    break;
  }

  if (i == nfbconfigs)
    return false;

  m_config = fbconfigs[i];
  m_window = window;

  m_vertex[0].u = 0.0f;
  m_vertex[0].v = m_top;
  m_vertex[0].z = 0.0f;
  m_vertex[1].u = 1.0f;
  m_vertex[1].v = m_top;
  m_vertex[1].z = 0.0f;
  m_vertex[2].u = 1.0f;
  m_vertex[2].v = m_bottom;
  m_vertex[2].z = 0.0f;
  m_vertex[3].u = 0.0f;
  m_vertex[3].v = m_bottom;
  m_vertex[3].z = 0.0f;


  XCompositeRedirectWindow(m_display, window, CompositeRedirectAutomatic);
  XCompositeRedirectSubwindows(m_display, window, CompositeRedirectAutomatic);

  return true;
}

void CGUIExternalAppControl::Resize()
{
  int width  = MathUtils::round_int(m_width);
  int height = MathUtils::round_int(m_height);
  XResizeWindow(m_display, m_window, width, height);
}

void CGUIExternalAppControl::Process(unsigned int currentTime, CDirtyRegionList &dirtyregions)
{
  if(m_window == None) {
    return;
  }

  /* check for invalidated pixmap */
  XWindowAttributes attrib;
  XGetWindowAttributes (m_display, m_window, &attrib);
  if(attrib.width  != m_attrib.width
  || attrib.height != m_attrib.height)
    Dispose();

  m_attrib = attrib;

  if (m_pixmap == None)
  {
    m_pixmap = XCompositeNameWindowPixmap (m_display, m_window);
    if (m_pixmap == None)
      return;
  }

  if(m_pixmap_gl == None)
  {
    int pixmapAttribs[] = { GLX_TEXTURE_TARGET_EXT, GLX_TEXTURE_2D_EXT,
                            GLX_TEXTURE_FORMAT_EXT, GLX_TEXTURE_FORMAT_RGBA_EXT,
                            None };
    m_pixmap_gl = glXCreatePixmap (m_display, m_config, m_pixmap, pixmapAttribs);
    if(m_pixmap_gl == None)
      return;
  }

  XGetWindowAttributes (m_display, m_window, &m_attrib);
  float scale_x = m_width  / m_attrib.width;
  float scale_y = m_height / m_attrib.height;

  float w, h, x, y;
  if(scale_x < scale_y)
  {
    w  = m_width;
    h = m_attrib.height * scale_x;
  }
  else
  {
    h = m_height;
    w = m_attrib.width  * scale_y;
  }

  x = m_posX + m_width * 0.5  - w * 0.5;
  y = m_posY + m_height * 0.5 - h * 0.5;

  m_vertex[0].x = x;
  m_vertex[0].y = y;

  m_vertex[1].x = x + w;
  m_vertex[1].y = y;

  m_vertex[2].x = x + w;
  m_vertex[2].y = y + h;

  m_vertex[3].x = x;
  m_vertex[3].y = y + h;

  for(unsigned i; i < 4; ++i)
    g_graphicsContext.ScaleFinalCoords(m_vertex[i].x, m_vertex[i].y, m_vertex[i].z);

  MarkDirtyRegion();
  CGUIControl::Process(currentTime, dirtyregions);
}

void CGUIExternalAppControl::Render()
{
  glBindTexture (GL_TEXTURE_2D, m_texture);

  m_glXBindTexImageEXT (m_display, m_pixmap_gl, GLX_FRONT_LEFT_EXT, NULL);

  glTexParameteri (GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
  glTexParameteri (GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
  glTexEnvf(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_COMBINE);
  glTexEnvf(GL_TEXTURE_ENV, GL_COMBINE_RGB , GL_MODULATE);
  glTexEnvf(GL_TEXTURE_ENV, GL_SOURCE0_RGB , GL_TEXTURE0);
  glTexEnvf(GL_TEXTURE_ENV, GL_OPERAND0_RGB, GL_SRC_COLOR);
  glTexEnvf(GL_TEXTURE_ENV, GL_SOURCE1_RGB , GL_PRIMARY_COLOR);
  glTexEnvf(GL_TEXTURE_ENV, GL_OPERAND1_RGB, GL_SRC_COLOR);

  glBlendFunc(GL_SRC_ALPHA,GL_ONE_MINUS_SRC_ALPHA);
  glEnable(GL_BLEND);

  glBegin (GL_QUADS);
  glColor4ub(0xff, 0xff, 0xff, 0xff);

  for(unsigned i = 0; i < 4; ++i)
  {
    glTexCoord2f (m_vertex[i].u, m_vertex[i].v);
    glVertex3f   (m_vertex[i].x, m_vertex[i].y, m_vertex[i].z);
  }

  glEnd ();

  m_glXReleaseTexImageEXT (m_display, m_pixmap_gl, GLX_FRONT_LEFT_EXT);
}

void CGUIExternalAppControl::FillXKeyEvent(XKeyEvent& event)
{
  event.display     = m_display;
  event.window      = m_window;
  event.root        = XDefaultRootWindow(m_display);
  event.subwindow   = None;
  event.time        = 0;
  event.x           = 1;
  event.y           = 1;
  event.x_root      = 1;
  event.y_root      = 1;
  event.same_screen = True;
}

void CGUIExternalAppControl::SendKeyPress(KeySym sym)
{
  XKeyEvent event = {0};
  FillXKeyEvent(event);
  event.keycode     = XKeysymToKeycode(m_display, sym);
  event.state       = 0;
  event.type        = KeyPress;
  XSendEvent(m_display, m_window, True, KeyPressMask, (XEvent *)&event);
  event.time       += 100;
  event.state      |= KeyPressMask;
  event.type        = KeyRelease;
  XSendEvent(m_display, m_window, True, KeyReleaseMask, (XEvent *)&event);
}

bool CGUIExternalAppControl::OnAction(const CAction &action)
{
  if (action.GetID() == ACTION_PLAYER_PLAY)
  {

    OnMouseEvent(CPoint(m_width * 0.5, m_height * 0.5), CMouseEvent(ACTION_MOUSE_LEFT_CLICK, 0, 0, 0));

  }
  else if(action.GetID() >= KEY_ASCII && action.GetID() < KEY_MOUSE)
  {
    CStdStringW wide;
    CStdString  utf8;
    wide.Format(L"%c", action.GetUnicode());
    g_charsetConverter.wToUTF8(wide, utf8);
    SendKeyPress(XStringToKeysym(utf8.c_str()));
  }
  else
  {
    struct SActionToSym {
      unsigned int action;
      KeySym       sym;
    } actions[] = {
        {ACTION_MOVE_UP   , XK_Up},
        {ACTION_MOVE_DOWN , XK_Down},
        {ACTION_MOVE_LEFT , XK_Left},
        {ACTION_MOVE_RIGHT, XK_Right},
        {ACTION_NONE      , 0},
        {ACTION_PAGE_UP   , XK_Page_Up},
        {ACTION_PAGE_DOWN , XK_Page_Down},
    };

    for(SActionToSym* it = actions; it->action != ACTION_NONE; ++it)
    {
      if(it->action == action.GetID())
      {
        SendKeyPress(it->sym);
        return true;
      }
    }

  }
  return CGUIControl::OnAction(action);
}

EVENT_RESULT CGUIExternalAppControl::OnMouseEvent(const CPoint &point, const CMouseEvent &event)
{
  if (event.m_id == ACTION_MOUSE_LEFT_CLICK)
  {
    XCrossingEvent crossev = {0};
    crossev.display=m_display;
    crossev.window=m_window;
    crossev.root=XDefaultRootWindow(m_display);;
    crossev.subwindow=None;
    crossev.time=0;
    crossev.x=MathUtils::round_int(point.x / m_width  * m_attrib.width);
    crossev.y=MathUtils::round_int(point.y / m_height * m_attrib.height);
    crossev.x_root      = m_attrib.x + crossev.x;
    crossev.y_root      = m_attrib.y + crossev.y;
    crossev.mode=NotifyNormal;
    crossev.detail=NotifyNonlinear;
    crossev.same_screen=True;
    crossev.focus=True;
    crossev.state=0;

    crossev.type=EnterNotify;
    XSendEvent(m_display,m_window,True,EnterWindowMask|LeaveWindowMask,(XEvent*)&crossev);
    XSync(m_display,false);


    XButtonEvent event = {0};

    event.display     = m_display;
    event.window      = m_window;
    event.root        = XDefaultRootWindow(m_display);
    event.subwindow   = None;
    event.time        = 0;
    event.x           = MathUtils::round_int(point.x / m_width  * m_attrib.width);
    event.y           = MathUtils::round_int(point.y / m_height * m_attrib.height);
    event.x_root      = m_attrib.x + event.x;
    event.y_root      = m_attrib.y + event.y;
    event.same_screen = True;
    event.send_event  = True;
    event.state       = 0;
    event.button      = Button1;

    event.type        = ButtonPress;
    XSendEvent(m_display, m_window, True, ButtonPressMask, (XEvent *)&event);
    XSync(m_display, false);

    event.time       += 1000;
    event.state       = Button1Mask;
    event.type        = ButtonRelease;
    XSendEvent(m_display, m_window, True, ButtonReleaseMask, (XEvent *)&event);
    XSync(m_display, false);


    crossev.time+= 2000;
    crossev.type =LeaveNotify;
    XSendEvent(m_display,m_window,True,EnterWindowMask|LeaveWindowMask,(XEvent*)&crossev);
    XSync(m_display,false);

    return EVENT_RESULT_HANDLED;
  }
  return EVENT_RESULT_UNHANDLED;
}

bool CGUIExternalAppControl::OnMessage(CGUIMessage& message)
{
  return CGUIControl::OnMessage(message);
}
