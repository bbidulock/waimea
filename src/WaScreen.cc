/** -*- Mode: C++ -*-
 *
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

#include <stdio.h>
#include <stdlib.h>
#include <X11/cursorfont.h>

#include "WaScreen.hh"

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
    
    eventmask = SubstructureRedirectMask | StructureNotifyMask |
        PropertyChangeMask | ColormapChangeMask | KeyPressMask |
        KeyReleaseMask | ButtonPressMask | ButtonReleaseMask |
        EnterWindowMask | LeaveWindowMask | FocusChangeMask;
    
    XSetErrorHandler((XErrorHandler) wmrunningerror);
    XSelectInput(display, id, eventmask);
    XSync(display, False);
    XSetErrorHandler((XErrorHandler) xerrorhandler);
    
    waimea->window_table->insert(make_pair(id, this));
   
    sprintf(displaystring, "DISPLAY=%s", DisplayString(display));
    sprintf(displaystring + strlen(displaystring) - 1, "%d", screen_number);
	
    v_x = v_y = 0;
    
    ic = new WaImageControl(display, this, rh->image_dither,
                            rh->colors_per_channel, rh->cache_max);
    ic->installRootColormap();

    rh->LoadStyle(this);

    CreateFonts();
    CreateColors();
    RenderCommonImages();
    XDefineCursor(display, id, waimea->session_cursor);
    
    v_xmax = (rh->virtual_x - 1) * width;
    v_ymax = (rh->virtual_y - 1) * height;
    west = new ScreenEdge(this, 0, 0, 1, height, WEdgeType);
    east = new ScreenEdge(this, width - 1, 0, 1, height, EEdgeType);
    north = new ScreenEdge(this, 0, 0, width, 1, NEdgeType);
    south = new ScreenEdge(this, 0, height - 1, width, 1, SEdgeType);
    net->GetDesktopViewPort(this);
    net->SetDesktopViewPort(this);

#ifdef SHAPE
    int dummy;
    shape = XShapeQueryExtension(display, &shape_event, &dummy);
#endif // SHAPE

    dock = new DockappHandler(this);
    
    WaWindow *newwin;
    XQueryTree(display, id, &ro, &pa, &children, &nchild);
    for (i = 0; i < (int) nchild; ++i) {
        XGetWindowAttributes(display, children[i], &attr);
        if ((! attr.override_redirect) && (attr.map_state == IsViewable)) {
            newwin = new WaWindow(children[i], this);
            hash_map<Window, WindowObject *>::iterator it;
            if ((it = waimea->window_table->find(children[i]))
                != waimea->window_table->end()) {
                if (((*it).second)->type == WindowType) {
                    newwin->net->SetState(newwin, NormalState);
                }
            }
        }
    }
    XFree(children);
}

/**
 * @fn    ~WaScreen(void)
 * @brief Destructor for WaScreen class
 *
 * Deletes all created colors and fonts.
 */
WaScreen::~WaScreen(void) {
    delete dock;
    delete west;
    delete east;
    delete north;
    delete south;
    delete ic;
    XFreeGC(display, wstyle.b_pic_focus_gc);
    XFreeGC(display, wstyle.b_pic_unfocus_gc);
    XFreeGC(display, wstyle.b_pic_hilite_gc);
#ifdef XFT
    XFree(wstyle.xftfont);
    XFree(mstyle.f_xftfont);
    XFree(mstyle.t_xftfont);
#else // ! XFT    
    XFreeFont(display, wstyle.font);
    XFreeFont(display, mstyle.f_font);
    XFreeFont(display, mstyle.t_font);
    XFreeGC(display, wstyle.l_text_focus_gc);
    XFreeGC(display, wstyle.l_text_unfocus_gc);
    XFreeGC(display, mstyle.f_text_gc);
    XFreeGC(display, mstyle.fh_text_gc);
    XFreeGC(display, mstyle.t_text_gc);
#endif // XFT
    waimea->window_table->erase(id);
}

/**
 * @fn    CreateFonts(void)
 * @brief Open fonts
 *
 * Opens all fonts and sets frame height.
 */
void WaScreen::CreateFonts(void) {
#ifdef XFT
    double default_font_size = (8.5 / 1200.0) * (double) height;
    if (wstyle.xftsize < 2.0) wstyle.xftsize = default_font_size;
    if (! (wstyle.xftfont = XftFontOpen(display, screen_number, XFT_FAMILY,
                                        XftTypeString, wstyle.xftfontname,
                                        XFT_SIZE, XftTypeDouble,
                                        wstyle.xftsize, 0))) {
        ERROR << "couldn't load font \"" << wstyle.xftfontname << "\"" << endl;
        quit(1);
    }
    if (mstyle.f_xftsize < 2.0) mstyle.f_xftsize = default_font_size;
    if (! (mstyle.f_xftfont = XftFontOpen(display, screen_number, XFT_FAMILY,
                                          XftTypeString, mstyle.f_xftfontname,
                                          XFT_SIZE, XftTypeDouble,
                                          mstyle.f_xftsize, 0))) {
        ERROR << "couldn't load font \"" << mstyle.f_xftfontname << "\"" << endl;
        quit(1);
    }
    if (mstyle.t_xftsize < 2.0) mstyle.t_xftsize = default_font_size;
    if (! (mstyle.t_xftfont = XftFontOpen(display, screen_number, XFT_FAMILY,
                                          XftTypeString, mstyle.t_xftfontname,
                                          XFT_SIZE, XftTypeDouble,
                                          mstyle.t_xftsize, 0))) {
        ERROR << "couldn't load font \"" << mstyle.t_xftfontname << "\"" << endl;
        quit(1);
    }
    if (mstyle.b_xftsize < 2.0) mstyle.b_xftsize = default_font_size;
    if (! (mstyle.b_xftfont = XftFontOpen(display, screen_number, XFT_FAMILY,
                                          XftTypeString, mstyle.b_xftfontname,
                                          XFT_SIZE, XftTypeDouble,
                                          mstyle.b_xftsize, 0))) {
        ERROR << "couldn't load font \"" << mstyle.b_xftfontname << "\"" << endl;
        quit(1);
    }
    int w_diff = wstyle.xftfont->ascent - wstyle.xftfont->descent;
    int mf_diff = mstyle.f_xftfont->ascent - mstyle.f_xftfont->descent;
    int mt_diff = mstyle.t_xftfont->ascent - mstyle.t_xftfont->descent;
    int mb_diff = mstyle.b_xftfont->ascent - mstyle.b_xftfont->descent;

    if (! wstyle.title_height)
        wstyle.title_height = wstyle.xftfont->height + 4;
    if (! mstyle.title_height)
        mstyle.title_height = mstyle.t_xftfont->height + 2;
    if (! mstyle.item_height)
        mstyle.item_height = mstyle.f_xftfont->height + 2;
#else // ! XFT
    if (! (wstyle.font = XLoadQueryFont(display, wstyle.fontname))) {
        ERROR << "couldn't load font \"" << wstyle.fontname << "\"" << endl;
        quit(1);
    }
    if (! (mstyle.f_font = XLoadQueryFont(display, mstyle.f_fontname))) {
        ERROR << "couldn't load font \"" << mstyle.f_fontname << "\"" << endl;
        quit(1);
    }
    if (! (mstyle.t_font = XLoadQueryFont(display, mstyle.t_fontname))) {
        ERROR << "couldn't load font \"" << mstyle.t_fontname << "\"" << endl;
        quit(1);
    }
    if (! (mstyle.b_font = XLoadQueryFont(display, mstyle.b_fontname))) {
        ERROR << "couldn't load font \"" << mstyle.b_fontname << "\"" << endl;
        quit(1);
    }
    int w_diff = wstyle.font->ascent - wstyle.font->descent;
    int mf_diff = mstyle.f_font->ascent - mstyle.f_font->descent;
    int mt_diff = mstyle.t_font->ascent - mstyle.t_font->descent;
    int mb_diff = mstyle.b_font->ascent - mstyle.b_font->descent;
    
    if (! wstyle.title_height)
        wstyle.title_height = wstyle.font->ascent + wstyle.font->descent + 4;
    if (! mstyle.title_height)
        mstyle.title_height = mstyle.t_font->ascent +
            mstyle.t_font->descent + 2;
    if (! mstyle.item_height) mstyle.item_height = wstyle.font->ascent + 
	    wstyle.font->descent + 2;
#endif // XFT
    if (wstyle.title_height < 10) mstyle.title_height = 10;
    if (mstyle.title_height < 4) mstyle.title_height = 4;
    if (mstyle.item_height < 4) mstyle.item_height = 4;
    
    wstyle.y_pos = (wstyle.title_height / 2 - 2) + w_diff / 2 + w_diff % 2;
    mstyle.f_y_pos = (mstyle.item_height / 2) + mf_diff / 2 + mf_diff % 2;
    mstyle.t_y_pos = (mstyle.title_height / 2) + mt_diff / 2 + mt_diff % 2;
    mstyle.b_y_pos = (mstyle.item_height / 2) + mb_diff / 2 + mb_diff % 2;
}

/**
 * @fn    CreateColors(void)
 * @brief Creates all colors
 *
 * Creates all color GCs.
 */
void WaScreen::CreateColors(void) {
    XGCValues gcv;    
    
    gcv.foreground = wstyle.b_pic_focus.getPixel();
    wstyle.b_pic_focus_gc = XCreateGC(display, id, GCForeground, &gcv);
    
    gcv.foreground = wstyle.b_pic_unfocus.getPixel();
    wstyle.b_pic_unfocus_gc = XCreateGC(display, id, GCForeground, &gcv);
    
    gcv.foreground = wstyle.b_pic_hilite.getPixel();
    wstyle.b_pic_hilite_gc = XCreateGC(display, id, GCForeground, &gcv);
#ifdef XFT
    CreateXftColor(&wstyle.l_text_focus, &wstyle.xftfcolor);
    CreateXftColor(&wstyle.l_text_unfocus, &wstyle.xftucolor);
    CreateXftColor(&mstyle.f_text, &mstyle.f_xftcolor);
    CreateXftColor(&mstyle.f_hilite_text, &mstyle.fh_xftcolor);
    CreateXftColor(&mstyle.t_text, &mstyle.t_xftcolor);
#else // ! XFT
    gcv.foreground = wstyle.l_text_focus.getPixel();
    gcv.font = wstyle.font->fid;
    wstyle.l_text_focus_gc = XCreateGC(display, id, GCForeground | GCFont,
                                       &gcv);
    
    gcv.foreground = wstyle.l_text_unfocus.getPixel();
    gcv.font = wstyle.font->fid;
    wstyle.l_text_unfocus_gc = XCreateGC(display, id, GCForeground | GCFont,
                                         &gcv);

    gcv.foreground = mstyle.f_text.getPixel();
    gcv.font = mstyle.f_font->fid;
    mstyle.f_text_gc = XCreateGC(display, id, GCForeground | GCFont, &gcv);

    gcv.foreground = mstyle.f_hilite_text.getPixel();
    gcv.font = mstyle.f_font->fid;
    mstyle.fh_text_gc = XCreateGC(display, id, GCForeground | GCFont, &gcv);

    gcv.foreground = mstyle.t_text.getPixel();
    gcv.font = mstyle.t_font->fid;
    mstyle.t_text_gc = XCreateGC(display, id, GCForeground | GCFont, &gcv);

    gcv.foreground = mstyle.f_text.getPixel();
    gcv.font = mstyle.b_font->fid;
    mstyle.b_text_gc = XCreateGC(display, id, GCForeground | GCFont, &gcv);

    gcv.foreground = mstyle.f_hilite_text.getPixel();
    gcv.font = mstyle.b_font->fid;
    mstyle.bh_text_gc = XCreateGC(display, id, GCForeground | GCFont, &gcv);
#endif // XFT
}

/**
 * @fn    RenderCommonImages(void)
 * @brief Render common images
 *
 * Render images which are common for all windows.
 */
void WaScreen::RenderCommonImages(void) {
    WaTexture *texture = &wstyle.b_focus;
    if (texture->getTexture() == (WaImage_Flat | WaImage_Solid)) {
        fbutton = None;
        fbutton_pixel = texture->getColor()->getPixel();
    } else
        fbutton = ic->renderImage(wstyle.title_height - 4,
                                  wstyle.title_height - 4, texture);

    texture = &wstyle.b_unfocus;
    if (texture->getTexture() == (WaImage_Flat | WaImage_Solid)) {
        ubutton = None;
        ubutton_pixel = texture->getColor()->getPixel();
    } else
        ubutton = ic->renderImage(wstyle.title_height - 4,
                                  wstyle.title_height - 4, texture);
    
    texture = &wstyle.b_pressed;
    if (texture->getTexture() == (WaImage_Flat | WaImage_Solid)) {
        pbutton = None;
        pbutton_pixel = texture->getColor()->getPixel();
    } else
        pbutton = ic->renderImage(wstyle.title_height - 4,
                                  wstyle.title_height - 4, texture);

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

#ifdef XFT
/**
 * @fn    CreateXftColor(WaColor *wac, XftColor *xftc)
 * @brief Creates xft color
 *
 * Stupid process for creating a xft color, only way I got it working.
 * Should probably look in to this when have time.
 *
 * @param wac WaColor to use then creating xft color.
 * @param xftc Pointer used for returning xft color.
 */
void WaScreen::CreateXftColor(WaColor *wac, XftColor *xftc) {
    XGCValues gcv;
    XColor tmp_color;
    GC tmp_gc;
    
    gcv.foreground = wac->getPixel();
    tmp_gc = XCreateGC(display, id, GCForeground, &gcv);
    XGetGCValues(display, tmp_gc, GCForeground, &gcv);
    tmp_color.pixel = gcv.foreground;
    XQueryColor(display, colormap, &tmp_color);
    xftc->color.red = tmp_color.red;
    xftc->color.green = tmp_color.green;
    xftc->color.blue = tmp_color.blue;
    xftc->color.alpha = 0xffff;
    xftc->pixel = gcv.foreground;

    XFreeGC(display, tmp_gc);
}
#endif // XFT

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
            (*it)->RedrawWindow();
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
 * @param warp True if we should warp pointer
 * @param direction Direction to move viewport
 */
void WaScreen::MoveViewport(int direction, bool warp) {
    int vd;
    
    switch (direction) {
        case WestDirection:
            if (v_x > 0) {
                if ((v_x - width) < 0) vd = v_x;
                else vd = width;
                if (warp) XWarpPointer(display, None, None, 0, 0, 0, 0, 
				       vd - 2, 0);
                MoveViewportTo(v_x - vd, v_y);
            }
            break;
        case EastDirection:
            if (v_x < v_xmax) {
                if ((v_x + width) > v_xmax) vd = v_xmax - v_x;
                else vd = width;
                if (warp) XWarpPointer(display, None, None, 0, 0, 0, 0,
                                       2 - vd, 0);
                MoveViewportTo(v_x + vd, v_y);
            }
            break;
        case NorthDirection:
            if (v_y > 0) {
                if ((v_y - height) < 0) vd = v_y;
                else vd = height;
                if (warp) XWarpPointer(display, None, None, 0, 0, 0, 0,
                                       0, vd - 2);
                MoveViewportTo(v_x, v_y - vd);
            }
            break;
        case SouthDirection:
            if (v_y < v_ymax) {
                if ((v_y + height) > v_ymax) vd = v_ymax - v_y;
                else vd = height;
                if (warp) XWarpPointer(display, None, None, 0, 0, 0, 0,
                                       0, 2 - vd);
                MoveViewportTo(v_x, v_y + vd);
            }
    }
}

/**
 * @fn    ScrollViewport(int direction)
 * @brief Scroll viewport
 *
 * Scrolls viewport a number of pixels in direction specified by direction
 * parameter. Direction can be one of WestDirection, EastDirection,
 * NorthDirection and SouthDirection. The 'param' variable in 'ac' decides
 * how many pixels we should scroll. If 'param' has no value we scroll the
 * default value of 30 pixels.
 *
 * @param direction Direction to move viewport
 * @param warp True if we should warp pointer
 * @param ac Action causing function call
 */
void WaScreen::ScrollViewport(int direction, bool warp, WaAction *ac) {
    int vd, scroll = 30;

    if (ac->param > 0) {
        scroll = ac->param;
        if (scroll > v_xmax) scroll = v_xmax;
    }
    switch (direction) {
        case WestDirection:
            if (v_x > 0) {
                if ((v_x - scroll) < 0) vd = v_x;
                else vd = scroll;
                if (warp) XWarpPointer(display, None, None, 0, 0, 0, 0, vd, 0);
                MoveViewportTo(v_x - vd, v_y);
            }
            break;
        case EastDirection:
            if (v_x < v_xmax) {
                if ((v_x + scroll) > v_xmax) vd = v_xmax - v_x;
                else vd = scroll;
                if (warp) XWarpPointer(display, None, None, 0, 0, 0, 0, -vd, 0);
                MoveViewportTo(v_x + vd, v_y);
            }
            break;
        case NorthDirection:
            if (v_y > 0) {
                if ((v_y - scroll) < 0) vd = v_y;
                else vd = scroll;
                if (warp) XWarpPointer(display, None, None, 0, 0, 0, 0, 0, vd);
                MoveViewportTo(v_x, v_y - vd);
            }
            break;
        case SouthDirection:
            if (v_y < v_ymax) {
                if ((v_y + scroll) > v_ymax) vd = v_ymax - v_y;
                else vd = scroll;
                if (warp) XWarpPointer(display, None, None, 0, 0, 0, 0, 0, -vd);
                MoveViewportTo(v_x, v_y + vd);
            }
    }
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
    XEvent *event;
    int px, py, nx, ny, i;
    list<XEvent *> *maprequest_list;
    Window w;
    unsigned int ui;
    
    XQueryPointer(display, id, &w, &w, &px, &py, &i, &i, &ui);
    
    maprequest_list = new list<XEvent *>;
    XGrabPointer(display, id, True, ButtonReleaseMask |
                 ButtonPressMask | PointerMotionMask | EnterWindowMask |
                 LeaveWindowMask, GrabModeAsync, GrabModeAsync, None,
                 waimea->move_cursor, CurrentTime);
    for (;;) {
        event =
            waimea->eh->EventLoop(waimea->eh->menu_viewport_move_return_mask);
        switch (event->type) {
            case MotionNotify:
                MoveViewportTo(v_x - (event->xmotion.x_root - px),
                               v_y - (event->xmotion.y_root - py));
                px = event->xmotion.x_root;
                py = event->xmotion.y_root;
                break;
            case LeaveNotify:
            case EnterNotify:
                break;
            case DestroyNotify:
            case UnmapNotify:
                if ((((event->type == UnmapNotify)? event->xunmap.window:
                      event->xdestroywindow.window) == id)) {
                    while (! maprequest_list->empty()) {
                        XPutBackEvent(display, maprequest_list->front());
                        maprequest_list->pop_front();
                    }
                    delete maprequest_list;
                    XPutBackEvent(display, event);
                    return;
                }
                waimea->eh->EvUnmapDestroy(event);
                break;
            case MapRequest:
                maprequest_list->push_front(event); break;
            case ButtonPress:
            case ButtonRelease:
                if (e->type == event->type) break;
                XUngrabPointer(display, CurrentTime);
                while (! maprequest_list->empty()) {
                    XPutBackEvent(display, maprequest_list->front());
                    maprequest_list->pop_front();
                }
                delete maprequest_list;
                return;
        }
    }
}

/**
 * @fn    Focus(XEvent *, WaAction *)
 * @brief Set input focus to root image
 *
 * Sets the keyboard input focus to the WaScreens root window.
 */
void WaScreen::Focus(XEvent *, WaAction *) {
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
    int i, rx, ry;
    unsigned int ui;
    WaMenu *menu = (WaMenu *) ac->param;

    if (XQueryPointer(display, id, &w, &w, &rx, &ry, &i, &i, &ui)) {
        if (menu->tasksw) menu->Build(this);
        menu->rf = this;
        menu->ftype = MenuRFuncMask;
        menu->Map(rx - (menu->width / 2),
                  ry - (menu->item_list->front()->height / 2));
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
    int i, rx, ry;
    unsigned int ui;
    WaMenu *menu = (WaMenu *) ac->param;
    
    if (XQueryPointer(display, id, &w, &w, &rx, &ry, &i, &i, &ui)) {
        if (menu->tasksw) waimea->taskswitch->Build(this);
        menu->rf = this;
        menu->ftype = MenuRFuncMask;
        menu->ReMap(rx - (menu->width / 2),
                    ry - (menu->item_list->front()->height / 2));
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
    WaMenu *menu = (WaMenu *) ac->param;
    
    menu->Unmap(focus);
    menu->UnmapSubmenus(focus);
}

/**
 * @fn    Restart(XEvent *, WaAction *)
 * @brief Restarts window manager
 *
 * Restarts window manager by deleting all objects and executing the program
 * file again.
 */
void WaScreen::Restart(XEvent *, WaAction *) {
    restart();
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
    waimea->taskswitch->Build(this);
    waimea->taskswitch->Map(width / 2 - waimea->taskswitch->width / 2,
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
 * @param ed Event details
 */
void WaScreen::PreviousTask(XEvent *e, WaAction *ac) {
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
 * @param ed Event details
 */
void WaScreen::NextTask(XEvent *e, WaAction *ac) {
    waimea->wawindow_list->back()->Raise(e, ac);
    waimea->wawindow_list->back()->FocusVis(e, ac);
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
    list<WaAction *>::iterator it = acts->begin();
    for (; it != acts->end(); ++it) {
        if (eventmatch(*it, ed)) {
            if ((*it)->exec)
                waexec((*it)->exec, displaystring);
            else
                ((*this).*((*it)->rootfunc))(e, *it);
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
    attrib_set.override_redirect = True;
    attrib_set.event_mask = EnterWindowMask | LeaveWindowMask |
        ButtonPressMask | ButtonReleaseMask;
    
    id = XCreateWindow(wa->display, wa->id, x, y, width, height, 0,
                       CopyFromParent, InputOnly, CopyFromParent,
                       CWOverrideRedirect | CWEventMask, &attrib_set);

    wa->waimea->net->wXDNDMakeAwareness(id);
    
    XMapWindow(wa->display, id);
    wa->waimea->WaRaiseWindow(0);
    wa->waimea->always_on_top_list->push_back(id);
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
