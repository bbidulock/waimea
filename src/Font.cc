/**
 * @file   Font.cc
 * @author David Reveman <c99drn@cs.umu.se>
 * @date   08-Oct-2002 11:05:01
 *
 * @brief Implementation of WaFont class  
 *
 * Waimea's font implementation. Includes both Xft and X core fonts.
 *
 * Copyright (C) David Reveman. All rights reserved.
 *
 */

#ifdef    HAVE_CONFIG_H
#  include "../config.h"
#endif // HAVE_CONFIG_H

#include "Font.hh"
#include "Waimea.hh"

#include <iostream>

using std::cerr;
using std::cout;
using std::endl;

/**
 * @fn    WaFont(void)
 * @brief Constructor for WaFont class
 *
 * Clears all class members.
 */
WaFont::WaFont(void) {
    xft = font_ok = false;
    shodow_off_x = shodow_off_y = 0;
    font = NULL;
    xfont = NULL;
    diff = 0;
    gc = s_gc = NULL;

#ifdef XFT        
    xftfont = NULL;
    color = s_color = NULL;
#endif // XFT
        
}

/**
 * @fn    Open(Display *dpy, int screen_number, WaFont *default_font)
 * @brief Opens font
 *
 * Opens the for dpy and screen_number. default_font is used if open fails.
 *
 * @param dpy Display connection
 * @param screen_number Screen to open font for
 * @param default_font Font used if open fails
 *
 * @return Returns height of the opened font
 */
int WaFont::Open(Display *dpy, int screen_number, WaFont *default_font) {    
    
#ifdef XFT    
    if (xft) {
        if (! (xftfont = XftFontOpenName(dpy, screen_number, font))) {
            WARNING << "failed loading font pattern `" << font << "'" <<
                endl;
            if (! default_font) return -1;
            xftfont = default_font->xftfont;
            xft = default_font->xft;
            xfont = default_font->xfont;
            diff = default_font->diff;
        } else {
            font_ok = true;
            diff = xftfont->ascent - xftfont->descent;
        }
        delete [] font;
        if (xft) return xftfont->height;
        else return xfont->ascent + xfont->descent;
    }
#endif // XFT
        
    if (! (xfont = XLoadQueryFont(dpy, font))) {
        WARNING << "failed loading font `" << font << "'" << endl;
        if (! default_font) return -1;
        
#ifdef XFT        
        xftfont = default_font->xftfont;
#endif // XFT
        
        xft = default_font->xft;
        xfont = default_font->xfont;
        diff = default_font->diff;
    } else {
        font_ok = true;
        diff = xfont->ascent - xfont->descent;
    }
    delete [] font;
    
#ifdef XFT
    if (xft) return xftfont->height;
    else 
#endif // XFT
        
    return xfont->ascent + xfont->descent;
}

/**
 * @fn    AllocColor(Display *dpy, Window id, WaColor *wac, WaColor *swac)
 * @brief Allocate colors
 *
 * Creates colors used for font rendering.
 *
 * @param dpy Display connection
 * @param id Window id used for GC creation
 * @param wac Font color
 * @param wac Font shadow color
 */
void WaFont::AllocColor(Display *dpy, Window id, WaColor *wac, WaColor *swac) {
    XGCValues gcv;
        
#ifdef XFT
    if (xft) {
        color = wac->getXftColor();
        if (swac) s_color = swac->getXftColor();
        return;
    }
#endif // XFT
        
    gcv.foreground = wac->getPixel();
    gcv.font = xfont->fid;
    gc = XCreateGC(dpy, id, GCForeground | GCFont, &gcv);
    if (swac) {
        gcv.foreground = swac->getPixel();
        gcv.font = xfont->fid;
        s_gc = XCreateGC(dpy, id, GCForeground | GCFont, &gcv);
    }
}

/**
 * @fn    Draw(Display *dpy, Window id, XftDraw *xftdraw, int x, int y,
 *             char *s, int len)
 * @brief Draw text
 *
 * Draws text string on drawable at a given position.
 *
 * @param dpy Display connection
 * @param id Drawable used for X core font rendering
 * @param xftdraw XftDrawable used for Xft font rendering
 * @param x X position to draw text at
 * @param y Y position to draw text at
 * @param s Text string to draw
 * @param len Length of text string s
 */

#ifdef XFT
void WaFont::Draw(Display *dpy, Drawable id, XftDraw *xftdraw, int x, int y,
                  char *s, int len) {    
    if (xft) {
        if (shodow_off_x || shodow_off_y)
            XftDrawString8(xftdraw, s_color, xftfont,
                           x + shodow_off_x, y + shodow_off_y,
                           (unsigned char *) s, len);
        XftDrawString8(xftdraw, color, xftfont, x, y,
                       (unsigned char *) s, len);
        return;
    }
#else // !XFT
void WaFont::Draw(Display *dpy, Window id, int x, int y,
                  char *s, int len) {
#endif // XFT
    
    if (shodow_off_x || shodow_off_y)
        XDrawString(dpy, (Drawable) id, s_gc,
                    x + shodow_off_x, y + shodow_off_y, s, len);
    XDrawString(dpy, (Drawable) id, gc, x, y, s, len);
}

/**
 * @fn    Width(Display *dpy, char *s, int len)
 * @brief Returns text width
 *
 * Calculates text width for a given string.
 *
 * @param dpy Display connection
 * @param s Text string to calculate width for
 * @param len Length of text string s
 *
 * @return Calculated width
 */ 
int WaFont::Width(Display *dpy, char *s, int len) {
        
#ifdef XFT
    if (xft) {
        XGlyphInfo extents;
        XftTextExtents8(dpy, xftfont, (unsigned char *) s, len, &extents);
        return extents.width;
    }
#endif // XFT
    
    return XTextWidth(xfont, s, len);
}
