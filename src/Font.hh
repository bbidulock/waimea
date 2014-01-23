/**
 * @file   Font.hh
 * @author David Reveman <david@waimea.org>
 * @date   08-Oct-2002 11:05:01
 *
 * @brief Definition of WaFont class
 *
 * Function declarations and variable definitions for WaFont.
 *
 * Copyright (C) David Reveman. All rights reserved.
 *
 */

#ifndef __Font_hh
#define __Font_hh

extern "C" {
#include <X11/Xlib.h>

#ifdef    XFT
#  include <X11/Xft/Xft.h>
#endif // XFT
}

class WaColor;

class WaFont {
public:
    WaFont(void);
    
    int Open(Display *, int, WaFont *);
    void AllocColor(Display *, Drawable id, WaColor *, WaColor * = NULL);
    
    void Draw(Display *, Window,
              
#ifdef XFT
              XftDraw *,
#endif // XFT
              
              int, int, char *, int);

    int Width(Display *, char *, int);

    bool xft;
    const char *font;
    GC gc, s_gc;
    XFontStruct *xfont;
    bool font_ok;
    int shodow_off_x, shodow_off_y;
    int diff;

#ifdef XFT
    XftFont *xftfont;
    XftColor *color, *s_color;
#endif // XFT

};

#endif // __Font_hh
