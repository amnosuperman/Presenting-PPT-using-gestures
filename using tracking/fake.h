#include <X11/Xlib.h>
#include <X11/keysym.h>

#ifndef FAKE_H
#define FAKE_H

XKeyEvent createKeyEvent(Display *display, Window &win,
                           Window &winRoot, bool press,
                           int keycode, int modifiers);
                           
void fake(Display* display,int x);

#endif
