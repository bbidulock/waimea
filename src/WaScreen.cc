/**
 * @file   WaScreen.cc
 * @author David Reveman <c99drn@cs.umu.se>
 * @date   25-Jul-2001 23:26:22
 *
 * @brief Implementation of WaScreen and ScreenEdge classes
 *
 * A WaScreen object handles one X server screen. A ScreenEdge is a
 * transperant window placed at the edge of the screen, good to use for
 * virtual screen scrolling.
 *
 * Copyright (C) David Reveman. All rights reserved.
 *
 */

#ifdef    HAVE_CONFIG_H
#  include "../config.h"
#endif // HAVE_CONFIG_H

#include <X11/cursorfont.h>

#include "WaScreen.hh"

#ifdef    HAVE_STDIO_H
#  include <stdio.h>
#endif // HAVE_STDIO_H

#ifdef    STDC_HEADERS
#  include <stdlib.h>
#endif // STDC_HEADERS

#ifdef    HAVE_IOSTREAM
#  include <iostream>
#endif // HAVE_IOSTREAM

using std::cerr;
using std::cout;
using std::endl;

/**
 * @fn    WaScreen(Display *d, int scrn_number, Waimea *wa)
 * @brief Constructor for WaScreen class
 *
 * Sets root window input mask. Creates new image control object and reads
 * style file. Then we create fonts, colors and renders common images.
 * Last thing we do in this functions is to create WaWindows for all windows
 * that should be managed.
 *
 * @param d The display
 * @param scrn_number Screen to manage
 * @param wa Waimea object
 */
WaScreen::WaScreen(Display *d, int scrn_number, Waimea *wa) :
    WindowObject(0, RootType) {
    Window ro, pa, *children;
    int eventmask, i;
    unsigned int nchild;
    XWindowAttributes attr;
    XSetWindowAttributes attrib_set;
    
    display = d;
    screen_number = scrn_number;
    id = RootWindow(display, screen_number);
    visual = DefaultVisual(display, screen_number);
    colormap = DefaultColormap(display, screen_number);
    screen_depth = DefaultDepth(display, screen_number);
    width = DisplayWidth(display, screen_number);
    height = DisplayHeight(display, screen_number);
    waimea = wa;
    net = waimea->net;
    rh = wa->rh;
    focus = true;

    default_font.xft = false;
    default_font.font = "fixed";

#ifdef PIXMAP
    imlib_context_set_display(display);
    imlib_context_set_colormap(colormap);
    imlib_context_set_visual(visual);
    imlib_context_set_mask(0);
    imlib_context_set_drawable(id);
    imlib_context_set_anti_alias(1);    
#endif // PIXMAP
    
    eventmask = SubstructureRedirectMask | StructureNotifyMask |
        PropertyChangeMask | ColormapChangeMask | KeyPressMask |
        KeyReleaseMask | ButtonPressMask | ButtonReleaseMask |
        EnterWindowMask | LeaveWindowMask | FocusChangeMask;

    XSetErrorHandler((XErrorHandler) wmrunningerror);
    XSelectInput(display, id, eventmask);
    XSync(display, false);
    XSetErrorHandler((XErrorHandler) xerrorhandler);
    
    waimea->window_table->insert(make_pair(id, this));
    
    attrib_set.override_redirect = true;
    wm_check = XCreateWindow(display, id, 0, 0, 1, 1, 0,
                             CopyFromParent, InputOnly, CopyFromParent,
                             CWOverrideRedirect, &attrib_set);
    net->SetSupported(this);
    net->SetSupportedWMCheck(this, wm_check);
   
    sprintf(displaystring, "DISPLAY=%s", DisplayString(display));
    sprintf(displaystring + strlen(displaystring) - 1, "%d", screen_number);
	
    v_x = v_y = 0;

    ic = new WaImageControl(display, this, rh->image_dither,
                            rh->colors_per_channel, rh->cache_max);
    ic->installRootColormap();

    rh->LoadStyle(this);
    waimea->wascreen = this;
    rh->LoadActions();

    CreateFonts();
    CreateColors();
    RenderCommonImages();
    XDefineCursor(display, id, waimea->session_cursor);
    
    v_xmax = (rh->virtual_x - 1) * width;
    v_ymax = (rh->virtual_y - 1) * height;
    west = new ScreenEdge(this, 0, 0, 2, height, WEdgeType);
    east = new ScreenEdge(this, width - 2, 0, 2, height, EEdgeType);
    north = new ScreenEdge(this, 0, 0, width, 2, NEdgeType);
    south = new ScreenEdge(this, 0, height - 2, width, 2, SEdgeType);
    net->SetDesktopGeometry(this);
    net->GetDesktopViewPort(this);
    net->SetDesktopViewPort(this);

#ifdef SHAPE
    int dummy;
    shape = XShapeQueryExtension(display, &shape_event, &dummy);
#endif // SHAPE

    strut_list = new list<WMstrut *>;
    workarea = new Workarea;
    workarea->x = workarea->y = 0;
    workarea->width = width;
    workarea->height = height;
    net->SetWorkarea(this);

#ifdef XRENDER
    net->GetXRootPMapId(this);
    ic->setXRootPMapId((xrootpmap_id)? true: false);
#endif // XRENDER    

    docks = new list<DockappHandler *>;
    list<DockStyle *>::iterator dit = waimea->rh->dockstyles->begin();
    for (; dit != waimea->rh->dockstyles->end(); ++dit) {
        docks->push_back(new DockappHandler(this, *dit));
    }
    
    WaWindow *newwin;
    XWMHints *wm_hints;
    wm_hints = XAllocWMHints();
    XQueryTree(display, id, &ro, &pa, &children, &nchild);
    for (i = 0; i < (int) nchild; ++i) {
        XGetWindowAttributes(display, children[i], &attr);
        if ((! attr.override_redirect) && (attr.map_state == IsViewable)) {
            if ((wm_hints = XGetWMHints(display, children[i])) &&
                (wm_hints->flags & StateHint) &&
                (wm_hints->initial_state == WithdrawnState)) {
                AddDockapp(children[i]);
            }
            else if ((waimea->window_table->find(children[i]))
                     == waimea->window_table->end()) {
                newwin = new WaWindow(children[i], this);
                map<Window, WindowObject *>::iterator it;
                if ((it = waimea->window_table->find(children[i]))
                    != waimea->window_table->end()) {
                    if (((*it).second)->type == WindowType) {
                        newwin->net->SetState(newwin, NormalState);
                    }
                }
            }
        }
    }
    XFree(wm_hints);
    XFree(children);
    net->GetClientListStacking(this);
    net->SetClientList(this);
    net->SetClientListStacking(this);
    net->GetActiveWindow(this);
}

/**
 * @fn    ~WaScreen(void)
 * @brief Destructor for WaScreen class
 *
 * Deletes all created colors and fonts.
 */
WaScreen::~WaScreen(void) {
    LISTCLEAR(docks);
    
    LISTDEL(strut_list);
    delete west;
    delete east;
    delete north;
    delete south;
    delete ic;
    delete workarea;

    delete [] wstyle.wa_font.font;
    delete [] mstyle.wa_f_font.font;
    delete [] mstyle.wa_t_font.font;
    delete [] mstyle.wa_b_font.font;
    delete [] mstyle.wa_ct_font.font;
    delete [] mstyle.wa_cf_font.font;
    delete [] mstyle.bullet;
    delete [] mstyle.checkbox_true;
    delete [] mstyle.checkbox_false;

    list<ButtonStyle *>::iterator bit = wstyle.buttonstyles->begin();
    for (; bit != wstyle.buttonstyles->end(); ++bit) {
        if ((*bit)->fg) {
            XFreeGC(display, (*bit)->g_focused);
            XFreeGC(display, (*bit)->g_unfocused);
            XFreeGC(display, (*bit)->g_pressed);
        }
    }
    
#ifdef XFT
    if (wstyle.wa_font.xft) XftFontClose(display, wstyle.xftfont);
    if (mstyle.wa_f_font.xft) XftFontClose(display, mstyle.f_xftfont);
    if (mstyle.wa_t_font.xft) XftFontClose(display, mstyle.t_xftfont);
    if (mstyle.wa_b_font.xft) XftFontClose(display, mstyle.b_xftfont);
    if (mstyle.wa_ct_font.xft) XftFontClose(display, mstyle.ct_xftfont);
    if (mstyle.wa_cf_font.xft) XftFontClose(display, mstyle.cf_xftfont);
#endif // XFT
    
    if (! wstyle.wa_font.xft && wstyle.font_ok)
        XFreeFont(display, wstyle.font);
    if (! mstyle.wa_f_font.xft && mstyle.f_font_ok)
        XFreeFont(display, mstyle.f_font);
    if (! mstyle.wa_t_font.xft && mstyle.t_font_ok)
        XFreeFont(display, mstyle.t_font);
    if (! mstyle.wa_b_font.xft && mstyle.b_font_ok)
        XFreeFont(display, mstyle.b_font);
    if (! mstyle.wa_ct_font.xft && mstyle.ct_font_ok)
        XFreeFont(display, mstyle.ct_font);
    if (! mstyle.wa_cf_font.xft && mstyle.cf_font_ok)
        XFreeFont(display, mstyle.cf_font);

    XFreeFont(display, def_font);

    if (! wstyle.wa_font.xft) {
        XFreeGC(display, wstyle.l_text_focus_gc);
        XFreeGC(display, wstyle.l_text_unfocus_gc);
    }
    if (! mstyle.wa_f_font.xft) {
        XFreeGC(display, mstyle.f_text_gc);
        XFreeGC(display, mstyle.fh_text_gc);
    }
    if (! mstyle.wa_t_font.xft) XFreeGC(display, mstyle.t_text_gc);
    if (! mstyle.wa_b_font.xft) {
        XFreeGC(display, mstyle.b_text_gc);
        XFreeGC(display, mstyle.bh_text_gc);
    }
    if (! mstyle.wa_ct_font.xft) {
        XFreeGC(display, mstyle.cth_text_gc);
        XFreeGC(display, mstyle.ct_text_gc);
    }
    if (! mstyle.wa_cf_font.xft) {
        XFreeGC(display, mstyle.cfh_text_gc);
        XFreeGC(display, mstyle.cf_text_gc);
    }

    net->DeleteSupported(this);
    XDestroyWindow(display, wm_check);
    XSelectInput(display, id, NoEventMask);
    XSync(display, false);
    waimea->window_table->erase(id);
}

/**
 * @fn    CreateFonts(void)
 * @brief Open fonts
 *
 * Opens all fonts and sets frame height.
 */
void WaScreen::CreateFonts(void) {
    int w_diff, mf_diff, mt_diff, mb_diff, mct_diff, mcf_diff;
    bool set_mih;
    
    if (! mstyle.item_height) set_mih = true;
    else set_mih = false;

    if (! (def_font = XLoadQueryFont(display, default_font.font))) {
        ERROR << "failed loading default font \"" << default_font.font <<
            "\"" << endl;
        quit(1);
    }
    
#ifdef XFT
    if (wstyle.wa_font.xft) {
        if (! (wstyle.xftfont = XftFontOpenName(display, screen_number,
                                                wstyle.wa_font.font))) {
            WARNING << "failed loading font pattern \"" <<
                wstyle.wa_font.font << "\"" << endl;
            wstyle.wa_font.xft = default_font.xft;
            delete [] wstyle.wa_font.font;
            wstyle.wa_font.font = wastrdup(default_font.font);
        } else {
            w_diff = wstyle.xftfont->ascent - wstyle.xftfont->descent;
            if (! wstyle.title_height)
                wstyle.title_height = wstyle.xftfont->height + 4;
        }
    }
    if (mstyle.wa_f_font.xft) {
        if (! (mstyle.f_xftfont = XftFontOpenName(display, screen_number,
                                                  mstyle.wa_f_font.font))) {
            WARNING << "failed loading font pattern \"" <<
                mstyle.wa_f_font.font << "\"" << endl;
            mstyle.wa_f_font.xft = default_font.xft;
            delete [] mstyle.wa_f_font.font;
            mstyle.wa_f_font.font = wastrdup(default_font.font);
        } else {
            mf_diff = mstyle.f_xftfont->ascent - mstyle.f_xftfont->descent;
            if (set_mih)
                mstyle.item_height = mstyle.f_xftfont->height + 2;
        }
    }
    if (mstyle.wa_t_font.xft) {
        if (! (mstyle.t_xftfont = XftFontOpenName(display, screen_number,
                                                  mstyle.wa_t_font.font))) {
            WARNING << "failed loading font pattern \"" <<
                mstyle.wa_t_font.font << "\"" << endl;
            mstyle.wa_t_font.xft = default_font.xft;
            delete [] mstyle.wa_t_font.font;
            mstyle.wa_t_font.font = wastrdup(default_font.font);
        } else {
            mt_diff = mstyle.t_xftfont->ascent - mstyle.t_xftfont->descent;
            if (! mstyle.title_height)
                mstyle.title_height = mstyle.t_xftfont->height + 2;
        }
    }
    if (mstyle.wa_b_font.xft) {
        if (! (mstyle.b_xftfont = XftFontOpenName(display, screen_number,
                                                  mstyle.wa_b_font.font))) {
            WARNING << "failed loading font pattern \"" <<
                mstyle.wa_b_font.font << "\"" << endl;
            mstyle.wa_b_font.xft = default_font.xft;
            delete [] mstyle.wa_b_font.font;
            mstyle.wa_b_font.font = wastrdup(default_font.font);
        } else {
            mb_diff = mstyle.b_xftfont->ascent - mstyle.b_xftfont->descent;
            if (set_mih && mstyle.item_height <
                (unsigned int) (mstyle.b_xftfont->height + 2))
                mstyle.item_height = mstyle.b_xftfont->height + 2;
        }
    }
    if (mstyle.wa_ct_font.xft) {
        if (! (mstyle.ct_xftfont = XftFontOpenName(display, screen_number,
                                                   mstyle.wa_ct_font.font))) {
            WARNING << "failed loading font pattern \"" <<
                mstyle.wa_ct_font.font << "\"" << endl;
            mstyle.wa_ct_font.xft = default_font.xft;
            delete [] mstyle.wa_ct_font.font;
            mstyle.wa_ct_font.font = wastrdup(default_font.font);
        } else {
            mct_diff = mstyle.ct_xftfont->ascent - mstyle.ct_xftfont->descent;
            if (set_mih && mstyle.item_height <
                (unsigned int) (mstyle.ct_xftfont->height + 2))
                mstyle.item_height = mstyle.ct_xftfont->height + 2;
        }
    }
    if (mstyle.wa_cf_font.xft) {
        if (! (mstyle.cf_xftfont = XftFontOpenName(display, screen_number,
                                                   mstyle.wa_cf_font.font))) {
            WARNING << "failed loading font pattern \"" <<
                mstyle.wa_cf_font.font << "\"" << endl;
            mstyle.wa_cf_font.xft = default_font.xft;
            delete [] mstyle.wa_cf_font.font;
            mstyle.wa_cf_font.font = wastrdup(default_font.font);
        } else {
            mcf_diff = mstyle.cf_xftfont->ascent - mstyle.cf_xftfont->descent;
            if (set_mih && mstyle.item_height <
                (unsigned int) (mstyle.cf_xftfont->height + 2))
                mstyle.item_height = mstyle.cf_xftfont->height + 2;
        }
    }
#endif // XFT
    
    if (! wstyle.wa_font.xft) {
        if (! (wstyle.font = XLoadQueryFont(display, wstyle.wa_font.font))) {
            WARNING << "failed loading font \"" << wstyle.wa_font.font <<
                "\"" << endl;
            wstyle.font = def_font;
            wstyle.font_ok = false;
        } else wstyle.font_ok = true;
        w_diff = wstyle.font->ascent - wstyle.font->descent;
        if (! wstyle.title_height)
            wstyle.title_height = wstyle.font->ascent +
                wstyle.font->descent + 4;
    }
    if (! mstyle.wa_f_font.xft) {
        if (! (mstyle.f_font = XLoadQueryFont(display,
                                              mstyle.wa_f_font.font))) {
            WARNING << "failed loading font \"" << mstyle.wa_f_font.font <<
                "\"" << endl;
            mstyle.f_font = def_font;
            mstyle.f_font_ok = false;
        } else mstyle.f_font_ok = true;
        mf_diff = mstyle.f_font->ascent - mstyle.f_font->descent;
        if (set_mih)
            mstyle.item_height = mstyle.f_font->ascent +
                mstyle.f_font->descent + 4;
    }
    if (! mstyle.wa_t_font.xft) {
        if (! (mstyle.t_font = XLoadQueryFont(display,
                                              mstyle.wa_t_font.font))) {
            WARNING << "failed loading font \"" << mstyle.wa_t_font.font <<
                "\"" << endl;
            mstyle.t_font = def_font;
            mstyle.t_font_ok = false;
        } else mstyle.t_font_ok = true;
        mt_diff = mstyle.t_font->ascent - mstyle.t_font->descent;
        if (! mstyle.title_height)
            mstyle.title_height = mstyle.t_font->ascent +
                mstyle.t_font->descent + 4;
    }
    if (! mstyle.wa_b_font.xft) {
        if (! (mstyle.b_font = XLoadQueryFont(display,
                                              mstyle.wa_b_font.font))) {
            WARNING << "failed loading font \"" << mstyle.wa_b_font.font <<
                "\"" << endl;
            mstyle.b_font = def_font;
            mstyle.b_font_ok = false;
        } else mstyle.b_font_ok = true;
        mb_diff = mstyle.b_font->ascent - mstyle.b_font->descent;
        if (set_mih && mstyle.item_height <
            (unsigned int) (mstyle.b_font->ascent +
                            mstyle.b_font->descent + 4))
            mstyle.item_height = mstyle.b_font->ascent +
                mstyle.b_font->descent + 4;
    }
    if (! mstyle.wa_ct_font.xft) {
        if (! (mstyle.ct_font = XLoadQueryFont(display,
                                               mstyle.wa_ct_font.font))) {
            WARNING << "failed loading font \"" << mstyle.wa_ct_font.font <<
                "\"" << endl;
            mstyle.ct_font = def_font;
            mstyle.ct_font_ok = false;
        } else mstyle.ct_font_ok = true;
        mct_diff = mstyle.ct_font->ascent - mstyle.ct_font->descent;
        if (set_mih && mstyle.item_height <
            (unsigned int) (mstyle.ct_font->ascent +
                            mstyle.ct_font->descent + 4))
            mstyle.item_height = mstyle.ct_font->ascent +
                mstyle.ct_font->descent + 4;
    }
    if (! mstyle.wa_cf_font.xft) {
        if (! (mstyle.cf_font = XLoadQueryFont(display,
                                               mstyle.wa_cf_font.font))) {
            WARNING << "failed loading font \"" << mstyle.wa_cf_font.font <<
                "\"" << endl;
            mstyle.cf_font = def_font;
            mstyle.cf_font_ok = false;
        } else mstyle.cf_font_ok = true;
        mcf_diff = mstyle.cf_font->ascent - mstyle.cf_font->descent;
        if (set_mih && mstyle.item_height <
            (unsigned int) (mstyle.cf_font->ascent +
                            mstyle.cf_font->descent + 4))
            mstyle.item_height = mstyle.cf_font->ascent +
                mstyle.cf_font->descent + 4;
    }
    
    if (wstyle.title_height < 10) mstyle.title_height = 10;
    if (mstyle.title_height < 4) mstyle.title_height = 4;
    if (mstyle.item_height < 4) mstyle.item_height = 4;
    
    wstyle.y_pos = (wstyle.title_height / 2 - 2) + w_diff / 2 + w_diff % 2;
    mstyle.f_y_pos = (mstyle.item_height / 2) + mf_diff / 2 + mf_diff % 2;
    mstyle.t_y_pos = (mstyle.title_height / 2) + mt_diff / 2 + mt_diff % 2;
    mstyle.b_y_pos = (mstyle.item_height / 2) + mb_diff / 2 + mb_diff % 2;
    mstyle.ct_y_pos = (mstyle.item_height / 2) + mct_diff / 2 + mct_diff % 2;
    mstyle.cf_y_pos = (mstyle.item_height / 2) + mcf_diff / 2 + mcf_diff % 2;
}

/**
 * @fn    CreateColors(void)
 * @brief Creates all colors
 *
 * Creates all color GCs.
 */
void WaScreen::CreateColors(void) {
    XGCValues gcv;    

    list<ButtonStyle *>::iterator bit = wstyle.buttonstyles->begin();
    for (; bit != wstyle.buttonstyles->end(); ++bit) {
        if ((*bit)->fg) {
            gcv.foreground = (*bit)->c_focused.getPixel();
            (*bit)->g_focused = XCreateGC(display, id, GCForeground, &gcv);
            gcv.foreground = (*bit)->c_unfocused.getPixel();
            (*bit)->g_unfocused = XCreateGC(display, id, GCForeground, &gcv);
            gcv.foreground = (*bit)->c_pressed.getPixel();
            (*bit)->g_pressed = XCreateGC(display, id, GCForeground, &gcv);
            gcv.foreground = (*bit)->c_focused2.getPixel();
            (*bit)->g_focused2 = XCreateGC(display, id, GCForeground, &gcv);
            gcv.foreground = (*bit)->c_unfocused2.getPixel();
            (*bit)->g_unfocused2 = XCreateGC(display, id, GCForeground, &gcv);
            gcv.foreground = (*bit)->c_pressed2.getPixel();
            (*bit)->g_pressed2 = XCreateGC(display, id, GCForeground, &gcv);
        }
    }
    
#ifdef XFT
    wstyle.xftfcolor = wstyle.l_text_focus.getXftColor();
    wstyle.xftucolor = wstyle.l_text_unfocus.getXftColor();
    mstyle.f_xftcolor = mstyle.f_text.getXftColor();
    mstyle.fh_xftcolor = mstyle.f_hilite_text.getXftColor();
    mstyle.t_xftcolor = mstyle.t_text.getXftColor();
#endif // XFT

    if (! wstyle.wa_font.xft) {
        gcv.foreground = wstyle.l_text_focus.getPixel();
        gcv.font = wstyle.font->fid;
        wstyle.l_text_focus_gc = XCreateGC(display, id, GCForeground | GCFont,
                                           &gcv);
        gcv.foreground = wstyle.l_text_unfocus.getPixel();
        gcv.font = wstyle.font->fid;
        wstyle.l_text_unfocus_gc = XCreateGC(display, id, GCForeground |
                                             GCFont, &gcv);
    }

    if (! mstyle.wa_f_font.xft) {
        gcv.foreground = mstyle.f_text.getPixel();
        gcv.font = mstyle.f_font->fid;
        mstyle.f_text_gc = XCreateGC(display, id, GCForeground | GCFont, &gcv);

        gcv.foreground = mstyle.f_hilite_text.getPixel();
        gcv.font = mstyle.f_font->fid;
        mstyle.fh_text_gc = XCreateGC(display, id, GCForeground | GCFont,
                                      &gcv);
    }

    if (! mstyle.wa_t_font.xft) {
        gcv.foreground = mstyle.t_text.getPixel();
        gcv.font = mstyle.t_font->fid;
        mstyle.t_text_gc = XCreateGC(display, id, GCForeground | GCFont, &gcv);
    }

    if (! mstyle.wa_b_font.xft) {
        gcv.foreground = mstyle.f_text.getPixel();
        gcv.font = mstyle.b_font->fid;
        mstyle.b_text_gc = XCreateGC(display, id, GCForeground | GCFont, &gcv);

        gcv.foreground = mstyle.f_hilite_text.getPixel();
        gcv.font = mstyle.b_font->fid;
        mstyle.bh_text_gc = XCreateGC(display, id, GCForeground | GCFont,
                                      &gcv);
    }

    if (! mstyle.wa_ct_font.xft) {
        gcv.foreground = mstyle.f_text.getPixel();
        gcv.font = mstyle.ct_font->fid;
        mstyle.ct_text_gc = XCreateGC(display, id, GCForeground | GCFont,
                                      &gcv);
    
        gcv.foreground = mstyle.f_hilite_text.getPixel();
        gcv.font = mstyle.ct_font->fid;
        mstyle.cth_text_gc = XCreateGC(display, id, GCForeground | GCFont,
                                       &gcv);
    }

    if (! mstyle.wa_cf_font.xft) {
        gcv.foreground = mstyle.f_text.getPixel();
        gcv.font = mstyle.cf_font->fid;
        mstyle.cf_text_gc = XCreateGC(display, id, GCForeground | GCFont,
                                      &gcv);
        
        gcv.foreground = mstyle.f_hilite_text.getPixel();
        gcv.font = mstyle.cf_font->fid;
        mstyle.cfh_text_gc = XCreateGC(display, id, GCForeground | GCFont,
                                       &gcv);
    }
}

/**
 * @fn    RenderCommonImages(void)
 * @brief Render common images
 *
 * Render images which are common for all windows.
 */
void WaScreen::RenderCommonImages(void) {
    WaTexture *texture;
    list<ButtonStyle *>::iterator bit = wstyle.buttonstyles->begin();
    for (; bit != wstyle.buttonstyles->end(); ++bit) {
        texture = &(*bit)->t_focused;
        if (texture->getTexture() == (WaImage_Flat | WaImage_Solid)) {
            (*bit)->p_focused = None;
            (*bit)->c_focused = texture->getColor()->getPixel();
        } else
            (*bit)->p_focused = ic->renderImage(wstyle.title_height - 4,
                                                wstyle.title_height - 4,
                                                texture);

        texture = &(*bit)->t_unfocused;
        if (texture->getTexture() == (WaImage_Flat | WaImage_Solid)) {
            (*bit)->p_unfocused = None;
            (*bit)->c_unfocused = texture->getColor()->getPixel();
        } else
            (*bit)->p_unfocused = ic->renderImage(wstyle.title_height - 4,
                                                  wstyle.title_height - 4,
                                                  texture);

        texture = &(*bit)->t_pressed;
        if (texture->getTexture() == (WaImage_Flat | WaImage_Solid)) {
            (*bit)->p_pressed = None;
            (*bit)->c_pressed = texture->getColor()->getPixel();
        } else
            (*bit)->p_pressed = ic->renderImage(wstyle.title_height - 4,
                                                wstyle.title_height - 4,
                                                texture);
        
        texture = &(*bit)->t_focused2;
        if (texture->getTexture() == (WaImage_Flat | WaImage_Solid)) {
            (*bit)->p_focused2 = None;
            (*bit)->c_focused2 = texture->getColor()->getPixel();
        } else
            (*bit)->p_focused2 = ic->renderImage(wstyle.title_height - 4,
                                                 wstyle.title_height - 4,
                                                 texture);

        texture = &(*bit)->t_unfocused2;
        if (texture->getTexture() == (WaImage_Flat | WaImage_Solid)) {
            (*bit)->p_unfocused2 = None;
            (*bit)->c_unfocused2 = texture->getColor()->getPixel();
        } else
            (*bit)->p_unfocused2 = ic->renderImage(wstyle.title_height - 4,
                                                   wstyle.title_height - 4,
                                                   texture);

        texture = &(*bit)->t_pressed2;
        if (texture->getTexture() == (WaImage_Flat | WaImage_Solid)) {
            (*bit)->p_pressed2 = None;
            (*bit)->c_pressed2 = texture->getColor()->getPixel();
        } else
            (*bit)->p_pressed2 = ic->renderImage(wstyle.title_height - 4,
                                                 wstyle.title_height - 4,
                                                 texture);
    }
    
    texture = &wstyle.g_focus;
    if (texture->getTexture() == (WaImage_Flat | WaImage_Solid)) {
        fgrip = None;
        fgrip_pixel = texture->getColor()->getPixel();
    } else
        fgrip = ic->renderImage(25, wstyle.handle_width, texture);

    texture = &wstyle.g_unfocus;
    if (texture->getTexture() == (WaImage_Flat | WaImage_Solid)) {
        ugrip = None;
        ugrip_pixel = texture->getColor()->getPixel();
    } else
        ugrip = ic->renderImage(25, wstyle.handle_width, texture);
}

/**
 * @fn    UpdateWorkarea(void)
 * @brief Update workarea
 *
 * Updates workarea, all maximized windows are maximizied to the new
 * workarea.
 */
void WaScreen::UpdateWorkarea(void) {
    int old_x = workarea->x, old_y = workarea->y,
        old_width = workarea->width, old_height = workarea->height;
    
    workarea->x = workarea->y = 0;
    workarea->width = width;
    workarea->height = height;
    list<WMstrut *>::iterator it = strut_list->begin();
    for (; it != strut_list->end(); ++it) {
        if ((*it)->left > workarea->x) workarea->x = (*it)->left;
        if ((*it)->top > workarea->y) workarea->y = (*it)->top;
        if ((width - (*it)->right) < workarea->width)
            workarea->width = width - (*it)->right;
        if ((height - (*it)->bottom) < workarea->height)
            workarea->height = height - (*it)->bottom;
    }
    workarea->width = workarea->width - workarea->x;
    workarea->height = workarea->height - workarea->y;

    int res_x, res_y, res_w, res_h;
    if (old_x != workarea->x || old_y != workarea->y ||
        old_width != workarea->width || old_height != workarea->height) {
        net->SetWorkarea(this);
        
        list<WaWindow *>::iterator wa_it = waimea->wawindow_list->begin();
        for (; wa_it != waimea->wawindow_list->end(); ++wa_it) {
            if ((*wa_it)->flags.max) {
                (*wa_it)->flags.max = false;
                res_x = (*wa_it)->restore_max.x;
                res_y = (*wa_it)->restore_max.y;
                res_w = (*wa_it)->restore_max.width;
                res_h = (*wa_it)->restore_max.height;
                (*wa_it)->_Maximize((*wa_it)->restore_max.misc0,
                                    (*wa_it)->restore_max.misc1);
                (*wa_it)->restore_max.x = res_x;
                (*wa_it)->restore_max.y = res_y;
                (*wa_it)->restore_max.width = res_w;
                (*wa_it)->restore_max.height = res_h;
            }
        }
    }
}

/**
 * @fn    MoveViewportTo(int x, int y)
 * @brief Move viewport to position
 *
 * Moves the virtual viewport to position (x, y). This is done by moving all
 * windows relative to the viewport change.
 *
 * @param x New x viewport
 * @param y New y viewport
 */
void WaScreen::MoveViewportTo(int x, int y) {
    if (x > v_xmax) x = v_xmax;
    else if (x < 0) x = 0;
    if (y > v_ymax) y = v_ymax;
    else if (y < 0) y = 0;
    
    int x_move = - (x - v_x);
    int y_move = - (y - v_y);
    v_x = x;
    v_y = y;

    list<WaWindow *>::iterator it = waimea->wawindow_list->begin();
    for (; it != waimea->wawindow_list->end(); ++it) {
        if (! (*it)->flags.sticky) {
            (*it)->attrib.x = (*it)->attrib.x + x_move;
            (*it)->attrib.y = (*it)->attrib.y + y_move;
            
            if ((((*it)->attrib.x + (*it)->attrib.width) > 0 &&
                 (*it)->attrib.x < width) && 
                (((*it)->attrib.y + (*it)->attrib.height) > 0 &&
                 (*it)->attrib.y < height))
                (*it)->RedrawWindow();
            else {
                (*it)->dontsend = true;
                (*it)->RedrawWindow();
                (*it)->dontsend = false;
                net->SetVirtualPos(*it);
            }   
        }
    }
    list<WaMenu *>::iterator it2 = waimea->wamenu_list->begin();
    for (; it2 != waimea->wamenu_list->end(); ++it2) {
        if ((*it2)->mapped && (! (*it2)->root_menu))
            (*it2)->Move(x_move, y_move);
    }
    net->SetDesktopViewPort(this);
}

/**
 * @fn    MoveViewport(int direction)
 * @brief Move viewport one screen in specified direction
 *
 * Moves viewport one screen in direction specified by direction parameter.
 * Direction can be one of WestDirection, EastDirection, NorthDirection and
 * SouthDirection.
 *
 * @param direction Direction to move viewport
 */
void WaScreen::MoveViewport(int direction) {
    int vd;
    
    switch (direction) {
        case WestDirection:
            if (v_x > 0) {
                if ((v_x - width) < 0) vd = v_x;
                else vd = width;
                XWarpPointer(display, None, None, 0, 0, 0, 0, vd - 6, 0);
                MoveViewportTo(v_x - vd, v_y);
            }
            break;
        case EastDirection:
            if (v_x < v_xmax) {
                if ((v_x + width) > v_xmax) vd = v_xmax - v_x;
                else vd = width;
                XWarpPointer(display, None, None, 0, 0, 0, 0, 6 - vd, 0);
                MoveViewportTo(v_x + vd, v_y);
            }
            break;
        case NorthDirection:
            if (v_y > 0) {
                if ((v_y - height) < 0) vd = v_y;
                else vd = height;
                XWarpPointer(display, None, None, 0, 0, 0, 0, 0, vd - 6);
                MoveViewportTo(v_x, v_y - vd);
            }
            break;
        case SouthDirection:
            if (v_y < v_ymax) {
                if ((v_y + height) > v_ymax) vd = v_ymax - v_y;
                else vd = height;
                XWarpPointer(display, None, None, 0, 0, 0, 0, 0, 6 - vd);
                MoveViewportTo(v_x, v_y + vd);
            }
    }
}

/** 
 * @fn    ViewportFixedMove(XEvent *, WaAction *ac)
 * @brief Fixed viewport move
 *
 * Parses the action parameter as an X offset string. The X and Y
 * offset values defines the fixed pixel position to move the
 * viewport to.
 *
 * @param ac WaAction object
 */ 
void WaScreen::ViewportFixedMove(XEvent *, WaAction *ac) {
    int x, y, mask; 
    unsigned int w = 0, h = 0;
                            
    if (! ac->param) return;
   
    mask = XParseGeometry(ac->param, &x, &y, &w, &h);
    if (mask & XNegative) x = v_xmax + x;
    if (mask & YNegative) y = v_ymax + y;
    MoveViewportTo(x, y); 
}

/**
 * @fn    ViewportRelativeMove(XEvent *, WaAction *ac)
 * @brief Relative viewport move
 *
 * Parses the action parameter as an X offset string. The X and Y
 * offset values defines the number of pixels to move the viewport.
 *
 * @param ac WaAction object
 */
void WaScreen::ViewportRelativeMove(XEvent *, WaAction *ac) {
    int x, y, mask;
    unsigned int w = 0, h = 0;
                  
    if (! ac->param) return;
   
    mask = XParseGeometry(ac->param, &x, &y, &w, &h);
    MoveViewportTo(v_x + x, v_y + y);
}

/**
 * @fn    ViewportMove(XEvent *e, WaAction *)
 * @brief Move viewport after mouse movement
 *
 * Moves viewport after mouse motion events.
 *
 * @param e XEvent causing function call
 */
void WaScreen::ViewportMove(XEvent *e, WaAction *) {
    XEvent event;
    int px, py, i;
    list<XEvent *> *maprequest_list;
    Window w;
    unsigned int ui;
    list<WaWindow *>::iterator it;

    if (waimea->eh->move_resize != EndMoveResizeType) return;
    waimea->eh->move_resize = MoveOpaqueType;
    
    XQueryPointer(display, id, &w, &w, &px, &py, &i, &i, &ui);
    
    maprequest_list = new list<XEvent *>;
    XGrabPointer(display, id, true, ButtonReleaseMask | ButtonPressMask |
                 PointerMotionMask | EnterWindowMask | LeaveWindowMask,
                 GrabModeAsync, GrabModeAsync, None, waimea->move_cursor,
                 CurrentTime);
    XGrabKeyboard(display, id, true, GrabModeAsync, GrabModeAsync,
                  CurrentTime);
    for (;;) {
        waimea->eh->EventLoop(waimea->eh->menu_viewport_move_return_mask,
                              &event);
        switch (event.type) {
            case MotionNotify:
                it = waimea->wawindow_list->begin();
                for (; it != waimea->wawindow_list->end(); ++it) {
                    (*it)->dontsend = true;
                }    
                MoveViewportTo(v_x - (event.xmotion.x_root - px),
                               v_y - (event.xmotion.y_root - py));
                px = event.xmotion.x_root;
                py = event.xmotion.y_root;
                break;
            case LeaveNotify:
            case EnterNotify:
                it = waimea->wawindow_list->begin();
                for (; it != waimea->wawindow_list->end(); ++it) {
                    (*it)->dontsend = true;
                }    
                MoveViewportTo(v_x - (event.xcrossing.x_root - px),
                               v_y - (event.xcrossing.y_root - py));
                px = event.xcrossing.x_root;
                py = event.xcrossing.y_root;
                break;
            case MapRequest:
                maprequest_list->push_front(&event); break;
            case ButtonPress:
            case ButtonRelease:
                event.xbutton.window = id;
            case KeyPress:
            case KeyRelease:
                if (event.type == KeyPress || event.type == KeyRelease)
                    event.xkey.window = id;
                waimea->eh->HandleEvent(&event);
                if (waimea->eh->move_resize != EndMoveResizeType) break;
                while (! maprequest_list->empty()) {
                    XPutBackEvent(display, maprequest_list->front());
                    maprequest_list->pop_front();
                }
                delete maprequest_list;
                it = waimea->wawindow_list->begin();
                for (; it != waimea->wawindow_list->end(); ++it) {
                    (*it)->dontsend = false;
                    if ((((*it)->attrib.x + (*it)->attrib.width) > 0 &&
                         (*it)->attrib.x < width) && 
                        (((*it)->attrib.y + (*it)->attrib.height) > 0 &&
                         (*it)->attrib.y < height))
                        (*it)->SendConfig();
                    net->SetVirtualPos(*it);
                }
                XUngrabKeyboard(display, CurrentTime);
                XUngrabPointer(display, CurrentTime);
                return;
        }
    }
}

/**
 * @fn    EndMoveResize(XEvent *e, WaAction *)
 * @brief Ends move
 *
 * Ends viewport moving process.
 */
void WaScreen::EndMoveResize(XEvent *, WaAction *) {
    waimea->eh->move_resize = EndMoveResizeType;
}

/**
 * @fn    Focus(XEvent *, WaAction *)
 * @brief Set input focus to root image
 *
 * Sets the keyboard input focus to the WaScreens root window.
 */
void WaScreen::Focus(XEvent *, WaAction *) {
    focus = true;
    XSetInputFocus(display, id, RevertToPointerRoot, CurrentTime);
}

/**
 * @fn    MenuMap(XEvent *e, WaAction *ac)
 * @brief Maps a menu
 *
 * Maps a menu at the current mouse position.
 *
 * @param ac WaAction object
 * @param bool True if we should focus first item in menu
 */
void WaScreen::MenuMap(XEvent *, WaAction *ac, bool focus) {
    Window w;
    int i, rx, ry, x, y, diff;
    unsigned int ui;
    WaMenu *menu = waimea->GetMenuNamed(ac->param);

    if (! menu) return;
    if (waimea->eh->move_resize != EndMoveResizeType) return;

    if (XQueryPointer(display, id, &w, &w, &rx, &ry, &i, &i, &ui)) {
        if (menu->tasksw) menu->Build(this);
        menu->rf = this;
        menu->ftype = MenuRFuncMask;
        x = rx - (menu->width / 2);
        y = ry - (menu->item_list->front()->height / 2);
        diff = (y + menu->height + mstyle.border_width * 2) - height;
        if (diff > 0) y -= diff;
        if (y < 0) y = 0;
        if ((x + menu->width + mstyle.border_width * 2) > width)
            x = width - menu->width - mstyle.border_width * 2;
        if (x < 0) x = 0;
        menu->Map(x, y);
        if (focus) menu->FocusFirst();
    }
}

/**
 * @fn    MenuRemap(XEvent *e, WaAction *ac)
 * @brief Maps a menu
 *
 * Remaps a menu at the current mouse position.
 *
 * @param ac WaAction object
 * @param bool True if we should focus first item in menu
 */
void WaScreen::MenuRemap(XEvent *, WaAction *ac, bool focus) {
    Window w;
    int i, rx, ry, x, y, diff;
    unsigned int ui;
    WaMenu *menu = waimea->GetMenuNamed(ac->param);

    if (! menu) return;
    if (menu->dynamic && menu->mapped) {
        menu->Unmap(menu->has_focus);
        if (! (menu = waimea->CreateDynamicMenu(ac->param))) return;
    }
    if (waimea->eh->move_resize != EndMoveResizeType) return;
    
    if (XQueryPointer(display, id, &w, &w, &rx, &ry, &i, &i, &ui)) {
        if (menu->tasksw) waimea->taskswitch->Build(this);
        menu->rf = this;
        menu->ftype = MenuRFuncMask;
        x = rx - (menu->width / 2);
        y = ry - (menu->item_list->front()->height / 2);
        diff = (y + menu->height + mstyle.border_width * 2) - height;
        if (diff > 0) y -= diff;
        if (y < 0) y = 0;
        if ((x + menu->width + mstyle.border_width * 2) > width)
            x = width - menu->width - mstyle.border_width * 2;
        if (x < 0) x = 0;
        menu->ignore = true;
        menu->ReMap(x, y);
        menu->ignore = false;
        if (focus) menu->FocusFirst();
    }
}

/**
 * @fn    MenuUnmap(XEvent *, WaAction *ac, bool focus)
 * @brief Unmaps menu
 *
 * Unmaps a menu and its linked submenus.
 *
 * @param ac WaAction object
 * @param bool True if we should focus root item
 */
void WaScreen::MenuUnmap(XEvent *, WaAction *ac, bool focus) {
    WaMenu *menu = waimea->GetMenuNamed(ac->param);

    if (! menu) return;
    if (waimea->eh->move_resize != EndMoveResizeType) return;
    
    menu->Unmap(focus);
    menu->UnmapSubmenus(focus);
}

/**
 * @fn    Restart(XEvent *, WaAction *)
 * @brief Restarts window manager
 *
 * Restarts window manager by deleting all objects and executing the program
 * file again.
 *
 * @param ac WaAction object
 */
void WaScreen::Restart(XEvent *, WaAction *ac) {
    if (ac->param)
        restart(ac->param);
    else
        restart(NULL);
}

/**
 * @fn    Exit(XEvent *, WaAction *)
 * @brief Shutdowns window manager
 *
 * Shutdowns window manager. Returning successful status.
 */
void WaScreen::Exit(XEvent *, WaAction *) {
    quit(EXIT_SUCCESS);
}

/**
 * @fn    TaskSwitcher(XEvent *, WaAction *)
 * @brief Maps task switcher menu
 *
 * Maps task switcher menu at middle of screen and sets input focus to
 * first window in list.
 */
void WaScreen::TaskSwitcher(XEvent *, WaAction *) {
    if (waimea->eh->move_resize != EndMoveResizeType) return;
    waimea->taskswitch->Build(this);
    waimea->taskswitch->ReMap(width / 2 - waimea->taskswitch->width / 2,
                              height / 2 - waimea->taskswitch->height / 2);
    waimea->taskswitch->FocusFirst();
}

/**
 * @fn    PreviousTask(XEvent *e, WaAction *ac)
 * @brief Switches to previous task
 *
 * Switches to the previous focused window.
 *
 * @param e X event that have occurred
 * @param ac WaAction object
 */
void WaScreen::PreviousTask(XEvent *e, WaAction *ac) {
    if (waimea->eh->move_resize != EndMoveResizeType) return;
    list<WaWindow *>::iterator it = waimea->wawindow_list->begin();
    it++;
    (*it)->Raise(e, ac);
    (*it)->FocusVis(e, ac);
}

/**
 * @fn    NextTask(XEvent *e, WaAction *ac)
 * @brief Switches to next task
 *
 * Switches to the window that haven't had focus for longest time.
 *
 * @param e X event that have occurred
 * @param ac WaAction object
 */
void WaScreen::NextTask(XEvent *e, WaAction *ac) {
    if (waimea->eh->move_resize != EndMoveResizeType) return;
    waimea->wawindow_list->back()->Raise(e, ac);
    waimea->wawindow_list->back()->FocusVis(e, ac);
}

/**
 * @fn    PointerFixedWarp(XEvent *e, WaAction *ac)
 * @brief Warps pointer to fixed position
 *
 * Parses the action parameter as an X offset string. The X and Y
 * offset values defines the fixed pixel position to warp the 
 * pointer to.
 *
 * @param ac WaAction object
 */
void WaScreen::PointerFixedWarp(XEvent *, WaAction *ac) {
    int x, y, mask, i, o_x, o_y;
    unsigned int ui, w, h;
    Window dw;
    
    mask = XParseGeometry(ac->param, &x, &y, &w, &h);
    if (mask & XNegative) x = width + x;
    if (mask & YNegative) y = height + y;
    XQueryPointer(display, id, &dw, &dw, &o_x, &o_y, &i, &i, &ui);
    x = x - o_x;
    y = y - o_y;
    XWarpPointer(display, None, None, 0, 0, 0, 0, x, y);
}

/** 
 * @fn    PointerRelativeWarp(XEvent *e, WaAction *ac)
 * @brief Relative pointer warp
 *
 * Parses the action parameter as an X offset string. The X and Y
 * offset values defines the number of pixels to warp the pointer.
 * 
 * @param ac WaAction object
 */
void WaScreen::PointerRelativeWarp(XEvent *, WaAction *ac) {
    int x, y, mask;
    unsigned int w, h;
                    
    mask = XParseGeometry(ac->param, &x, &y, &w, &h);
    XWarpPointer(display, None, None, 0, 0, 0, 0, x, y);
} 

/**
 * @fn    EvAct(XEvent *e, EventDetail *ed, list<WaAction *> *acts)
 * @brief Calls WaScreen function
 *
 * Tries to match an occurred X event with the actions in an action list.
 * If we have a match then we execute that action.
 *
 * @param e X event that have occurred
 * @param ed Event details
 * @param acts List with actions to match event with
 */
void WaScreen::EvAct(XEvent *e, EventDetail *ed, list<WaAction *> *acts) {
    if (waimea->eh->move_resize != EndMoveResizeType)
        ed->mod |= MoveResizeMask;
    list<WaAction *>::iterator it = acts->begin();
    for (; it != acts->end(); ++it) {
        if (eventmatch(*it, ed)) {
            if ((*it)->delay.tv_sec || (*it)->delay.tv_usec) {
                Interrupt *i = new Interrupt(*it, e, id);
                waimea->timer->AddInterrupt(i);
            }
            else {
                if ((*it)->exec)
                    waexec((*it)->exec, displaystring);
                else
                    ((*this).*((*it)->rootfunc))(e, *it);
            }
        }
    }
}

/**
 * @fn    AddDockapp(Window window)
 * @brief Add dockapp to dockapp system
 *
 * Inserts the dockapp into the right dockapp holder.
 *
 * @param window Window ID of dockapp window
 */
void WaScreen::AddDockapp(Window window) {
    XClassHint *c_hint;
    Dockapp *da;
    int have_hints;

    c_hint = XAllocClassHint();
    have_hints = XGetClassHint(display, window, c_hint);

    list<char *>::iterator it;
    list<DockappHandler *>::iterator dock_it;
    if (have_hints) {
        dock_it = docks->begin();
        for (; dock_it != docks->end(); ++dock_it) {
            it = (*dock_it)->style->order->begin();
            for (; it != (*dock_it)->style->order->end(); ++it) {
                if ((**it == 'N') &&
                    (! strcmp(*it + 2, c_hint->res_name))) {
                    da = new Dockapp(window, *dock_it);
                    da->c_hint = c_hint;
                    (*dock_it)->Update();
                    return;
                }
            }
            it = (*dock_it)->style->order->begin();
            for (; it != (*dock_it)->style->order->end(); ++it) {
                if ((**it == 'C') &&
                    (! strcmp(*it + 2, c_hint->res_class))) {
                    da = new Dockapp(window, *dock_it);
                    da->c_hint = c_hint;
                    (*dock_it)->Update();
                    return;
                }
            }
        }
    }
    dock_it = docks->begin();
    for (; dock_it != docks->end(); ++dock_it) {
        it = (*dock_it)->style->order->begin();
        for (; it != (*dock_it)->style->order->end(); ++it) {
            if (**it == 'U') {
                da = new Dockapp(window, *dock_it);
                da->c_hint = NULL;
                (*dock_it)->Update();
                if (have_hints) {
                    XFree(c_hint->res_name);
                    XFree(c_hint->res_class);
                }
                XFree(c_hint);
                return;
            }
        }
    }
}


/**
 * @fn    ScreenEdge(WaScreen *wascrn, int x, int y, int width, int height,
 *                   int type) : WindowObject(0, type)
 * @brief Constructor for ScreenEdge class
 *
 * Creates an always on top screen edge window.
 *
 * @param wascrn WaScreen Object
 * @param x X position
 * @param y Y position
 * @param width Width of ScreenEdge window
 * @param height Height of ScreenEdge window
 * @param type Type of WindowObject
 */
ScreenEdge::ScreenEdge(WaScreen *wascrn, int x, int y, int width, int height,
                       int type) : WindowObject(0, type) {
    XSetWindowAttributes attrib_set;
    
    wa = wascrn;
    attrib_set.override_redirect = true;
    attrib_set.event_mask = EnterWindowMask | LeaveWindowMask |
        ButtonPressMask | ButtonReleaseMask;
    
    id = XCreateWindow(wa->display, wa->id, x, y, width, height, 0,
                       CopyFromParent, InputOnly, CopyFromParent,
                       CWOverrideRedirect | CWEventMask, &attrib_set);

    wa->waimea->net->wXDNDMakeAwareness(id);
    
    XMapWindow(wa->display, id);
    wa->waimea->always_on_top_list->push_back(id);
    wa->waimea->WaRaiseWindow(0);
    wa->waimea->window_table->insert(make_pair(id, this));
}

/**
 * @fn    ~ScreenEdge(void)
 * @brief Destructor for ScreenEdge class
 *
 * Destroys ScreenEdge window
 */
ScreenEdge::~ScreenEdge(void) {
    wa->waimea->always_on_top_list->remove(id);
    wa->waimea->window_table->erase(id);
    XDestroyWindow(wa->display, id);
}
