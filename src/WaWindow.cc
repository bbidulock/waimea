/** -*- Mode: C++ -*-
 *
 * @file   WaWindow.cc
 * @author David Reveman <c99drn@cs.umu.se>
 * @date   02-May-2001 21:43:03
 *
 * @brief Implementation of WaWindow and WaChildWindow classes
 *
 * An instance if this class manages one window. Contains functions for
 * creating window decorations, reading window hints and controlling the
 * window.
 *
 * Copyright (C) David Reveman. All rights reserved.
 *
 */

#include "WaWindow.hh"

#include <stdio.h>
#include <string.h>
#include <hash_set>

/**
 * @fn    WaWindow(Window win_id, WaScreen *scrn) :
 *        WindowObject(win_id, WindowType)
 * @brief Constructor for WaWindow class
 *
 * Reparents the window, reads window hints and creates decorations ...
 *
 * @param win_id Resource ID of window to manage
 * @param scrn The screen the window should be displayed on
 */
WaWindow::WaWindow(Window win_id, WaScreen *scrn) :
    WindowObject(win_id, WindowType) {
    XWindowAttributes init_attrib;
    
    id = win_id;
    wascreen = scrn;
    display = wascreen->display;
    screen_number = wascreen->screen_number;
    waimea = wascreen->waimea;
    ic = wascreen->ic;
    net = waimea->net;        
    
    waimea->window_table->insert(make_pair(id, this));
    waimea->wawindow_list->push_back(this);

    if (! XFetchName(display, id, &name)) {
        name = strdup("");
    }
    XGetWindowAttributes(display, id, &init_attrib);
    attrib.colormap = init_attrib.colormap;
    
    attrib.x = init_attrib.x;
    attrib.y = init_attrib.y;
    attrib.width  = init_attrib.width;
    attrib.height = init_attrib.height;
    
    want_focus = mapped = dontsend = False;
    
#ifdef SHAPE
    shaped = False;
#endif //SHAPE
    
    border_w = title_w = handle_w = 0;
    has_focus = True;
    flags.sticky = flags.shaded = flags.max_v = flags.max_h = False;

    net->GetWMHints(this);
    net->GetMWMHints(this);
    net->GetWMNormalHints(this);
    net->GetVirtualPos(this);

    CreateOutlineWindows();
    
    Gravitate(ApplyGravity);
    InitPosition();
    
    frame = new WaChildWindow(this, wascreen->id, border_w, FrameType);
    if (handle_w) {
        handle = new WaChildWindow(this, frame->id, border_w, HandleType);
        grip_l = new WaChildWindow(this, frame->id, border_w, LGripType);
        grip_r = new WaChildWindow(this, frame->id, border_w, RGripType);
        RenderHandle();
    }
    if (title_w)  {
        title = new WaChildWindow(this, frame->id, border_w, TitleType);
        label = new WaChildWindow(this, title->id, 0, LabelType);
        button_min = new WaChildWindow(this, title->id, 0, IButtonType);
        button_max = new WaChildWindow(this, title->id, 0, MButtonType);
        button_c = new WaChildWindow(this, title->id, 0, CButtonType);
        RenderLabel();
        RenderTitle();
#ifdef XFT        
        xftdraw = XftDrawCreate(display, (Drawable) label->id,
                                wascreen->visual, wascreen->colormap);
#endif // XFT
    }
    UnFocusWin();
    ReparentWin();
    Shape();
    net->GetStateSticky(this);
    net->GetStateShaded(this);
    net->GetStateMaxV(this);
    net->GetStateMaxH(this);
}

/**
 * @fn    ~WaWindow(void) 
 * @brief Destructor of WaWindow class
 *
 * Reparents the window back to the root window if it still exists. Destroys
 * all windows used for decorations.
 */
WaWindow::~WaWindow(void) {
#ifdef XFT
    if (title_w)
        XftDrawDestroy(xftdraw);
#endif // XFT
    XGrabServer(display);
    if (validateclient(id)) {
        XRemoveFromSaveSet(display, id);
        Gravitate(RemoveGravity);
        if (flags.shaded) attrib.height = restore_shade.height;
        XReparentWindow(display, id, wascreen->id, attrib.x -
                        (attrib.x / wascreen->width) * wascreen->width,
                        attrib.y -
                        (attrib.y / wascreen->height) * wascreen->height);
    }
    XUngrabServer(display);
    XDestroyWindow(display, frame->id);
    delete frame;
    if (title_w) {
        delete title;
        delete label;
        delete button_min;
        delete button_max;
        delete button_c;
    }
    if (handle_w) {
        delete handle;
        delete grip_l;
        delete grip_r;
    }
    waimea->always_on_top_list->remove(o_west);
    waimea->always_on_top_list->remove(o_east);
    waimea->always_on_top_list->remove(o_north);
    waimea->always_on_top_list->remove(o_south);
    XDestroyWindow(display, o_west);
    XDestroyWindow(display, o_east);
    XDestroyWindow(display, o_north);
    XDestroyWindow(display, o_south);
    XFree(name);
    waimea->window_table->erase(id);
    waimea->wawindow_list->remove(this);
}

/**
 * @fn    Gravitate(int multiplier)
 * @brief Applies or removes window gravity
 *
 * If multiplier parameter is RemoveGravity when we set the window
 * attributes so that the frame will match the windows position.
 * If multiplier parameter is ApplyGravity when we set the window
 * attributes so that the window will match the frames position.
 *
 * @param multiplier ApplyGravity or RemoveGravity
 */
void WaWindow::Gravitate(int multiplier) {
    switch (size.win_gravity) {
        case NorthWestGravity:
            attrib.x = attrib.x + (multiplier * border_w) * 2;
        case NorthEastGravity:
            attrib.x = attrib.x - multiplier * border_w;
        case NorthGravity:
            attrib.y = attrib.y + multiplier * (title_w + border_w * 2);
            break;
        case CenterGravity:
            attrib.x = attrib.x + multiplier * (border_w / 2);
            attrib.y = attrib.y + multiplier * (title_w / 2 + border_w);
            break;
        case StaticGravity:
            break;
    }
}

/**
 * @fn    InitPosition(void)
 * @brief Sets window position
 *
 * Initializes position for the window.
 */
void WaWindow::InitPosition(void) {
    if (size.min_width > attrib.width) attrib.width = size.min_width;
    if (size.min_height > attrib.height) attrib.height = size.min_height;
    old_attrib.x = restore_shade.x = restore_max.x = attrib.x;
    old_attrib.y = restore_shade.y = restore_max.y = attrib.y;
    old_attrib.width = restore_shade.width = restore_max.width =
        attrib.width;
    old_attrib.height = restore_shade.height = restore_max.height =
        attrib.height;
}

/**
 * @fn    RedrawWindow(void)
 * @brief Redraws Window
 *
 * Redraws the window at it's correct position with it's correct size.
 * We only redraw those things that need to be redrawn.
 */
void WaWindow::RedrawWindow(void) {
    Bool move = False, resize = False;
    
    if (old_attrib.x != attrib.x) {
        frame->attrib.x = attrib.x - border_w;
        old_attrib.x = attrib.x;

        move = True;
    }
    if (old_attrib.y != attrib.y) {
        frame->attrib.y = attrib.y - title_w - border_w * 2;
        old_attrib.y = attrib.y;
            
        move = True;
    }
    if (old_attrib.width != attrib.width) {
        if (flags.max_h) {
            net->SetStateMaxH(this, 2);
            DrawMaxButtonFg();
        }
        frame->attrib.width = attrib.width;
        old_attrib.width = attrib.width;

        resize = True;

        if (title_w) {
            title->attrib.width = attrib.width;
            label->attrib.width = attrib.width - 3 * title_w + 2;
            if (label->attrib.width < 1) label->attrib.width = 1;

            button_c->attrib.x = attrib.width - (title_w - 2);
            button_max->attrib.x = attrib.width - (title_w - 2) * 2;

            XMoveWindow(display, button_c->id, button_c->attrib.x,
                        button_c->attrib.y);
            XMoveWindow(display, button_max->id, button_max->attrib.x,
                        button_max->attrib.y);
            XResizeWindow(display, title->id, title->attrib.width,
                          title->attrib.height);
            XResizeWindow(display, label->id, label->attrib.width,
                          label->attrib.height);
            
            RenderTitle();
            RenderLabel();
            DrawTitlebar();
        }
        if (handle_w) {
            handle->attrib.width = attrib.width - 50 - border_w * 2;
            if (handle->attrib.width < 1) handle->attrib.width = 1;
            grip_r->attrib.x = attrib.width - 25 - border_w;

            XMoveWindow(display, grip_r->id, grip_r->attrib.x,
                        grip_r->attrib.y);
            XResizeWindow(display, handle->id, handle->attrib.width,
                          handle->attrib.height);
            
            RenderHandle();
            DrawHandle();
        }
    }
    if (old_attrib.height != attrib.height) {
        if (flags.max_v) {
            net->SetStateMaxV(this, 2);
            DrawMaxButtonFg();
        }
        frame->attrib.height = attrib.height + title_w + handle_w +
            border_w * 2;
        old_attrib.height = attrib.height;
        
        resize = True;
        if (handle_w) {
            handle->attrib.y = attrib.height + title_w + border_w;
            grip_l->attrib.y = attrib.height + title_w + border_w;
            grip_r->attrib.y = attrib.height + title_w + border_w;

            XMoveWindow(display, handle->id, handle->attrib.x,
                        handle->attrib.y);
            XMoveWindow(display, grip_l->id, grip_l->attrib.x,
                        grip_l->attrib.y);
            XMoveWindow(display, grip_r->id, grip_r->attrib.x,
                        grip_r->attrib.y);
        }
    }
    if (move) {
        XMoveWindow(display, frame->id, frame->attrib.x, frame->attrib.y);
        net->SetVirtualPos(this);
    }
    if (resize) {
        XGrabServer(display);
        if (validateclient(id)) {    
            if (flags.shaded) XResizeWindow(display, id, attrib.width,
                                      restore_shade.height);
            else XResizeWindow(display, id, attrib.width, attrib.height);
            XResizeWindow(display, frame->id, frame->attrib.width,
                          frame->attrib.height);
        }
        XUngrabServer(display);
        Shape();
    }
    if ((move || resize) && (! flags.shaded) && (! dontsend)) SendConfig();
}

/**
 * @fn    ReparentWin(void)
 * @brief Reparents the window into the frame
 *
 * Sets the input mask for the managed window so that it will report
 * FocusChange and PropertyChange events. Then we reparent the window into
 * our frame and activates needed grab buttons.
 */
void WaWindow::ReparentWin(void) {
    XGrabServer(display);
    if (validateclient(id)) {
        XSelectInput(display, id, NoEventMask);
        XAddToSaveSet(display, id);
        XSetWindowBorderWidth(display, id, 0);
        XReparentWindow(display, id, frame->id, 0, title_w + border_w);
        XLowerWindow(display, id);
        XSelectInput(display, id, FocusChangeMask | PropertyChangeMask |
                     StructureNotifyMask);

#ifdef SHAPE
        int n, order;
        XRectangle *dummy;
        if (wascreen->shape) {
            XShapeSelectInput(display, id, ShapeNotifyMask);
            dummy = XShapeGetRectangles(display, id, ShapeBounding, &n,
                                        &order);
            if (n > 1) shaped = True;
        }
        XFree(dummy);
#endif // SHAPE
        
        list<WaAction *>::iterator it = waimea->rh->winacts->begin();
        for (; it != waimea->rh->winacts->end(); ++it) {
            if ((*it)->type == ButtonPress || (*it)->type == ButtonRelease) {
                XGrabButton(display, (*it)->detail ? (*it)->detail: AnyButton,
                            (*it)->mod, id, True, ButtonPressMask |
                            ButtonReleaseMask | ButtonMotionMask,
                            GrabModeAsync, GrabModeAsync, None, None);
            } else if ((*it)->type == KeyPress || (*it)->type == KeyRelease) {
                XGrabKey(display, (*it)->detail ? (*it)->detail: AnyKey,
                         (*it)->mod, id, True, GrabModeAsync, GrabModeAsync); 
            }
        }
    }
    XUngrabServer(display);
}

#ifdef SHAPE
/**
 * @fn    Shape(void)
 * @brief Set Shape of frame window
 *
 * Shapes frame window after shape of client.
 */
void WaWindow::Shape(void) {
        int n;
        XRectangle xrect[2];
        
        if (shaped) {
            XGrabServer(display);
            if (validateclient(id)) {
                XShapeCombineShape(display, frame->id, ShapeBounding,
                                   border_w, title_w + border_w, id,
                                   ShapeBounding, ShapeSet);
                n = 1;
                xrect[0].x = -border_w;
                xrect[0].y = -border_w;
                xrect[0].width = attrib.width + border_w * 2;
                xrect[0].height = title_w + border_w * 2;
                if (handle_w) {
                    xrect[1].x = -border_w;
                    xrect[1].y = attrib.height + title_w + border_w;
                    xrect[1].width = attrib.width + border_w * 2;
                    xrect[1].height = handle_w + border_w * 2;
                    n++;
                }
                XShapeCombineRectangles(display, frame->id, ShapeBounding, 0, 0,
                                        xrect, n, ShapeUnion, Unsorted);
            }
            XUngrabServer(display);
        }
}
#endif // SHAPE

/**
 * @fn    SendConfig(void)
 * @brief Sends ConfigureNotify to window
 *
 * Sends a ConfigureNotify event to the window with all the current 
 * window attributes.
 */
void WaWindow::SendConfig(void) {
    XConfigureEvent ce;
    
    ce.type              = ConfigureNotify;
    ce.event             = id;
    ce.window            = id;
    ce.x                 = attrib.x;
    ce.y                 = attrib.y;
    ce.width             = attrib.width;
    ce.height            = attrib.height;
    ce.border_width      = 0;
    ce.above             = frame->id;
    ce.override_redirect = False;

    XGrabServer(display);
    if (validateclient(id)) 
        XSendEvent(display, id, True, NoEventMask, (XEvent *)&ce);
    XUngrabServer(display);
}

/**
 * @fn    CreateOutlineWindows(void)
 * @brief Creates outline windows
 *
 * Creates four windows used for displaying an outline when doing
 * non opaque moving of the window.
 */
void WaWindow::CreateOutlineWindows(void) {
    XSetWindowAttributes attrib_set;
    
    int create_mask = CWOverrideRedirect | CWBackPixel | CWEventMask |
        CWColormap;
    attrib_set.background_pixel = wascreen->wstyle.border_color.getPixel();
    attrib_set.colormap = wascreen->colormap;
    attrib_set.override_redirect = True;
    attrib_set.event_mask = NoEventMask;
    
    o_west = XCreateWindow(display, wascreen->id, 0, 0, 1, 1, 0,
                           screen_number, CopyFromParent, wascreen->visual,
                           create_mask, &attrib_set);
    o_east = XCreateWindow(display, wascreen->id, 0, 0, 1, 1, 0,
                           screen_number, CopyFromParent, wascreen->visual,
                           create_mask, &attrib_set);
    o_north = XCreateWindow(display, wascreen->id, 0, 0, 1, 1, 0,
                            screen_number, CopyFromParent, wascreen->visual,
                            create_mask, &attrib_set);
    o_south = XCreateWindow(display, wascreen->id, 0, 0, 1, 1, 0,
                            screen_number, CopyFromParent, wascreen->visual,
                            create_mask, &attrib_set);
    waimea->always_on_top_list->push_back(o_west);
    waimea->always_on_top_list->push_back(o_east);
    waimea->always_on_top_list->push_back(o_north);
    waimea->always_on_top_list->push_back(o_south);
    o_mapped = False;
}

/**
 * @fn    ToggleOutline(void)
 * @brief Toggles outline windows on/off
 *
 * Un-/ maps outline windows.
 */
void WaWindow::ToggleOutline(void) {
    if (o_mapped) {
        XUnmapWindow(display, o_west);
        XUnmapWindow(display, o_east);
        XUnmapWindow(display, o_north);
        XUnmapWindow(display, o_south);
        o_mapped = False;
    }
    else {
        XMapWindow(display, o_west);
        XMapWindow(display, o_east);
        XMapWindow(display, o_north);
        XMapWindow(display, o_south);
        waimea->WaRaiseWindow(0);
        o_mapped = True;
    }
        
}

/**
 * @fn    DrawOutline(int x, int y, int width, int height)
 * @brief Draw window outline
 *
 * Draws an outer line for a window with the parameters given.
 * 
 * @param x The x position
 * @param y The y position
 * @param width The width
 * @param height The height
 */
void WaWindow::DrawOutline(int x, int y, int width, int height) {
    int bw = (border_w) ? border_w: 2;
    
    XResizeWindow(display, o_west, bw, bw * 2 + title_w + handle_w + height +
                  border_w * 2);
    XResizeWindow(display, o_east, bw, bw * 2 + title_w + handle_w + height +
                  border_w * 2);
    XResizeWindow(display, o_north, width + bw * 2, bw);
    XResizeWindow(display, o_south, width + bw * 2, bw);

    XMoveWindow(display, o_west, x - bw, y - title_w - border_w - bw);
    XMoveWindow(display, o_east, x + width, y - title_w - border_w - bw);
    XMoveWindow(display, o_north, x - bw, y - title_w - border_w - bw);
    XMoveWindow(display, o_south, x - bw, y + height + handle_w + border_w);
}

/**
 * @fn    FocusWin(void)
 * @brief Sets window to have the look of a focused window
 *
 * Sets window decoration pointers to point so that window decorations will
 * represent a focused window. Redraws needed graphics.
 */
void WaWindow::FocusWin(void) {
    if (has_focus) return;

    has_focus = True;
    
    ptitle  = &ftitle;
    plabel  = &flabel;
    phandle = &fhandle;
    pbutton = &wascreen->fbutton;
    pgrip   = &wascreen->fgrip;

    title_pixel  = &ftitle_pixel;
    label_pixel  = &flabel_pixel;
    handle_pixel = &fhandle_pixel;
    button_pixel = &wascreen->fbutton_pixel;
    grip_pixel   = &wascreen->fgrip_pixel;

    b_cpic_gc  = &wascreen->wstyle.b_pic_focus_gc;
    b_ipic_gc  = &wascreen->wstyle.b_pic_focus_gc;
    b_mpic_gc  = &wascreen->wstyle.b_pic_focus_gc;
#ifdef XFT
    xftcolor = &wascreen->wstyle.xftfcolor;
#else // ! XFT
    l_text_gc = &wascreen->wstyle.l_text_focus_gc;
#endif // XFT
    if (title_w)  DrawTitlebar();
    if (handle_w) DrawHandlebar();
}

/**
 * @fn    UnFocusWin(void)
 * @brief Sets window to have the look of a unfocused window
 *
 * Sets window decoration pointers to point so that window decorations will
 * represent an unfocused window. Redraws needed graphics.
 */
void WaWindow::UnFocusWin(void) {
    if (! has_focus) return;

    has_focus = False;
    
    ptitle  = &utitle;
    plabel  = &ulabel;
    phandle = &uhandle;
    pbutton = &wascreen->ubutton;
    pgrip   = &wascreen->ugrip;

    title_pixel  = &utitle_pixel;
    label_pixel  = &ulabel_pixel;
    handle_pixel = &uhandle_pixel;
    button_pixel = &wascreen->ubutton_pixel;
    grip_pixel   = &wascreen->ugrip_pixel;

    b_cpic_gc  = &wascreen->wstyle.b_pic_unfocus_gc;
    b_ipic_gc  = &wascreen->wstyle.b_pic_unfocus_gc;
    b_mpic_gc  = &wascreen->wstyle.b_pic_unfocus_gc;
#ifdef XFT
    xftcolor = &wascreen->wstyle.xftucolor;
#else // ! XFT
    l_text_gc = &wascreen->wstyle.l_text_unfocus_gc;
#endif // XFT    
    if (title_w)  DrawTitlebar();
    if (handle_w) DrawHandlebar();
}

/**
 * @fn    RenderTitle(void)
 * @brief Render title graphics
 *
 * Renders new title graphics for the current window size.
 */
void WaWindow::RenderTitle(void) {
    WaTexture *texture = &(wascreen->wstyle.t_focus);
    if (texture->getTexture() == (WaImage_Flat | WaImage_Solid)) {
        ftitle = None;
        ftitle_pixel = texture->getColor()->getPixel();
    } else
        ftitle = ic->renderImage(title->attrib.width,
                                 title->attrib.height, texture);

    texture = &(wascreen->wstyle.t_unfocus);
    if (texture->getTexture() == (WaImage_Flat | WaImage_Solid)) {
        utitle = None;
        utitle_pixel = texture->getColor()->getPixel();
    } else
        utitle = ic->renderImage(title->attrib.width,
                                 title->attrib.height, texture);
}

/**
 * @fn    RenderLabel(void)
 * @brief Render label graphics
 *
 * Renders new label graphics for the current window size.
 */
void WaWindow::RenderLabel(void) {
    WaTexture *texture = &(wascreen->wstyle.l_focus);
    if (texture->getTexture() == (WaImage_Flat | WaImage_Solid)) {
        flabel = None;
        flabel_pixel = texture->getColor()->getPixel();
    } else
        flabel = ic->renderImage(label->attrib.width,
                                 label->attrib.height, texture);

    texture = &(wascreen->wstyle.l_unfocus);
    if (texture->getTexture() == (WaImage_Flat | WaImage_Solid)) {
        ulabel = None;
        ulabel_pixel = texture->getColor()->getPixel();
    } else
        ulabel = ic->renderImage(label->attrib.width,
                                 label->attrib.height, texture);
}

/**
 * @fn    RenderHandle(void)
 * @brief Render handle graphics
 *
 * Renders new handle graphics for the current window size.
 */
void WaWindow::RenderHandle(void) {
    WaTexture *texture = &(wascreen->wstyle.h_focus);
    if (texture->getTexture() == (WaImage_Flat | WaImage_Solid)) {
        fhandle = None;
        fhandle_pixel = texture->getColor()->getPixel();
    } else
        fhandle = ic->renderImage(handle->attrib.width,
                                  handle->attrib.height, texture);
    
    texture = &(wascreen->wstyle.h_unfocus);
    if (texture->getTexture() == (WaImage_Flat | WaImage_Solid)) {
        uhandle = None;
        uhandle_pixel = texture->getColor()->getPixel();
    } else
        uhandle = ic->renderImage(handle->attrib.width,
                                  handle->attrib.height, texture);
}

/**
 * @fn    DrawTitle(void)
 * @brief Draw title graphics
 *
 * Draws title graphics in title window
 */
void WaWindow::DrawTitle(void) {
    if (*ptitle)
        XSetWindowBackgroundPixmap(display, title->id, *ptitle);
    else
        XSetWindowBackground(display, title->id, *title_pixel);
    
    XClearWindow(display, title->id);
}

/**
 * @fn    DrawLabel(void)
 * @brief Draw label graphics
 *
 * Draws label graphics in label window
 */
void WaWindow::DrawLabel(void) {
    if (*plabel)
        XSetWindowBackgroundPixmap(display, label->id, *plabel);
    else
        XSetWindowBackground(display, label->id, *label_pixel);
    
    DrawLabelFg();
}

/**
 * @fn    DrawLabelFg(void)
 * @brief Draw label foreground graphics
 *
 * Draws window title in label window.
 */
void WaWindow::DrawLabelFg(void) {
    int x = 0, length, text_w;
    
    XClearWindow(display, label->id);
    length = strlen(name);
#ifdef XFT
    XGlyphInfo extents;
    XftTextExtents8(display, wascreen->wstyle.xftfont,
                    (unsigned char *) name, length, &extents);
    text_w = extents.width;
#else // ! XFT
    text_w = XTextWidth(wascreen->wstyle.font, name, length);
#endif // XFT
    if (text_w > (label->attrib.width - 10))
        x = 5;
    else {
        switch (wascreen->wstyle.justify) {
            case LeftJustify: x = 5; break;
            case CenterJustify:
                x = (label->attrib.width / 2) - (text_w / 2);
                break;
            case RightJustify:
                x = (label->attrib.width - text_w) - 5;
                break;
        }
    }
#ifdef XFT
    XftDrawString8(xftdraw, xftcolor, wascreen->wstyle.xftfont, x,
                   wascreen->wstyle.y_pos, (unsigned char *) name, length);
#else // ! XFT
    XDrawString(display, (Drawable) label->id, *l_text_gc, x,
                wascreen->wstyle.y_pos, name, length);
#endif // XFT
}

/**
 * @fn    DrawHandle(void)
 * @brief Draw title graphics
 *
 * Draws handle graphics in handle window.
 */
void WaWindow::DrawHandle(void) {
    if (*phandle)
        XSetWindowBackgroundPixmap(display, handle->id, *phandle);
    else
        XSetWindowBackground(display, handle->id, *handle_pixel);
    
    XClearWindow(display, handle->id);
}

/**
 * @fn    DrawIconifyButton(void)
 * @brief Draw iconify button graphics
 *
 * Draws iconify button graphics in iconify button window.
 */
void WaWindow::DrawIconifyButton(void) {
    if (*pbutton)
        XSetWindowBackgroundPixmap(display, button_min->id, *pbutton);
    else
        XSetWindowBackground(display, button_min->id, *button_pixel);
    
    DrawIconifyButtonFg();
}

/**
 * @fn    DrawIconifyButtonFg(void)
 * @brief Draw iconify button foreground graphics
 *
 * Draws iconify button foreground graphics in iconify button window.
 */
void WaWindow::DrawIconifyButtonFg(void) {
    XClearWindow(display, button_min->id);
    XDrawRectangle(display, button_min->id, *b_ipic_gc,
                   2, title_w - 9, title_w - 9, 2);
}

/**
 * @fn    DrawMaxButton(void)
 * @brief Draw maximize button graphics
 *
 * Draws maximize button graphics in maximize button window.
 */
void WaWindow::DrawMaxButton(void) {
    if (*pbutton)
        XSetWindowBackgroundPixmap(display, button_max->id, *pbutton);
    else
        XSetWindowBackground(display, button_max->id, *button_pixel);
    
    DrawMaxButtonFg();
}

/**
 * @fn    DrawMaxButtonFg(void)
 * @brief Draw maximize button foreground graphics
 *
 * If window is maximized we draw unmaximize foreground graphics otherwise
 * we draw normal maximize foreground graphics in maximize button window.
 */
void WaWindow::DrawMaxButtonFg(void) {
    XClearWindow(display, button_max->id);
    if (flags.max_v || flags.max_h) {
        int w = (2*(title_w - 8))/3;
        int h = (2*(title_w - 8))/3 - 1;
        int y = (title_w - 8) - h + 1;
        int x = (title_w - 8) - w + 1;
        XDrawRectangle(display, button_max->id, *b_mpic_gc, 2, y, w, h);
        XDrawLine(display, button_max->id, *b_mpic_gc, 2, y + 1, 2 + w, y + 1);
        XDrawLine(display, button_max->id, *b_mpic_gc, x, 2, x + w, 2);
        XDrawLine(display, button_max->id, *b_mpic_gc, x, 3, x + w, 3);
        XDrawLine(display, button_max->id, *b_mpic_gc, x, 2, x, y);
        XDrawLine(display, button_max->id, *b_mpic_gc, x + w, 2, x + w, 2 + h);
        XDrawLine(display, button_max->id, *b_mpic_gc, 2 + w, 2 + h, x + w,
                  2 + h);
    } else {
        XDrawRectangle(display, button_max->id, *b_mpic_gc,
                       2, 2, title_w - 9, title_w - 9);
        XDrawLine(display, button_max->id, *b_mpic_gc, 2, 3, title_w - 8, 3);
    }
}

/**
 * @fn    DrawCloseButton(void)
 * @brief Draw close button graphics
 *
 * Draws close button graphics in close button window.
 */
void WaWindow::DrawCloseButton(void) {
    if (*pbutton)
        XSetWindowBackgroundPixmap(display, button_c->id, *pbutton);
    else
        XSetWindowBackground(display, button_c->id, *button_pixel);
    
    DrawCloseButtonFg();
}

/**
 * @fn    DrawCloseButtonFg(void)
 * @brief Draw close button foreground graphics
 *
 * Draws close button foreground graphics in close button window.
 */
void WaWindow::DrawCloseButtonFg(void) {
    XClearWindow(display, button_c->id);   
    XDrawLine(display, button_c->id, *b_cpic_gc, 2, 2, title_w - 7,
              title_w - 7);
    XDrawLine(display, button_c->id, *b_cpic_gc, 2, title_w - 7, title_w - 7,
              2);
}

/**
 * @fn    DrawLeftGrip(void)
 * @brief Draw left grip graphics
 *
 * Draws grip graphics in left grip window.
 */
void WaWindow::DrawLeftGrip(void) {
    if (*pgrip)
        XSetWindowBackgroundPixmap(display, grip_l->id, *pgrip);
    else
        XSetWindowBackground(display, grip_l->id, *grip_pixel);
    
    XClearWindow(display, grip_l->id);
}

/**
 * @fn    DrawRightGrip(void)
 * @brief Draw right grip graphics
 *
 * Draws grip graphics in right grip window.
 */
void WaWindow::DrawRightGrip(void) {
    if (*pgrip)
        XSetWindowBackgroundPixmap(display, grip_r->id, *pgrip);
    else
        XSetWindowBackground(display, grip_r->id, *grip_pixel);
    
    XClearWindow(display, grip_r->id);
}

/**
 * @fn    ButtonHilite(void)
 * @brief Hilites button
 *
 * Hilites a button through changing its foreground color.
 *
 * @param type Type of button
 */
void WaWindow::ButtonHilite(int type) {
    switch (type) {
        case CButtonType:
            b_cpic_gc = &wascreen->wstyle.b_pic_hilite_gc;
            DrawCloseButtonFg(); break;
        case IButtonType:
            b_ipic_gc = &wascreen->wstyle.b_pic_hilite_gc;
            DrawIconifyButtonFg(); break;
        case MButtonType:
            b_mpic_gc = &wascreen->wstyle.b_pic_hilite_gc;
            DrawMaxButtonFg(); break;
    }
}

/**
 * @fn    ButtonDehilite(void)
 * @brief Dehilites button
 *
 * Dehilites a button through changing its foreground color.
 *
 * @param type Type of button
 */
void WaWindow::ButtonDehilite(int type) {
    switch (type) {
        case CButtonType:
            if (has_focus)
                b_cpic_gc = &wascreen->wstyle.b_pic_focus_gc;
            else
                b_cpic_gc = &wascreen->wstyle.b_pic_unfocus_gc;
            DrawCloseButtonFg(); break;
        case IButtonType:
            if (has_focus)
                b_ipic_gc = &wascreen->wstyle.b_pic_focus_gc;
            else
                b_ipic_gc = &wascreen->wstyle.b_pic_unfocus_gc;
            DrawIconifyButtonFg(); break;
        case MButtonType:
            if (has_focus)
                b_mpic_gc = &wascreen->wstyle.b_pic_focus_gc;
            else
                b_mpic_gc = &wascreen->wstyle.b_pic_unfocus_gc;
            DrawMaxButtonFg(); break;
    }
}

/**
 * @fn    ButtonPressed(int type)
 * @brief Titlebar buttonpress
 *
 * Performes button press animation on one of the titlebar buttons.
 *
 * @param type Type of button to press
 */
void WaWindow::ButtonPressed(int type) {
    XEvent e;
    bool in_window = True;
    
    pbutton = &wascreen->pbutton;
    button_pixel = &wascreen->pbutton_pixel;
    switch (type) {
        case CButtonType: DrawCloseButton(); break;
        case IButtonType: DrawIconifyButton(); break;
        case MButtonType: DrawMaxButton(); break;
    }
    for (;;) {
        XMaskEvent(display, ButtonReleaseMask | EnterWindowMask |
                   LeaveWindowMask, &e);        
        switch (e.type) {
            case EnterNotify:
                pbutton = &wascreen->pbutton;
                button_pixel = &wascreen->pbutton_pixel;
                in_window = True;
                ButtonHilite(type);
                break;
            case LeaveNotify:
                ButtonDehilite(type);
                in_window = False;
            case ButtonRelease:
                pbutton = (has_focus) ? &wascreen->fbutton: &wascreen->ubutton;
                button_pixel = (has_focus) ?
                    &wascreen->fbutton_pixel: &wascreen->ubutton_pixel;
        }
        switch (type) {
            case CButtonType: DrawCloseButton(); break;
            case IButtonType: DrawIconifyButton(); break;
            case MButtonType: DrawMaxButton(); break;
        }
        if (e.type == ButtonRelease) {
            if (in_window) XSendEvent(display, e.xany.window, True,
                                      ButtonReleaseMask, &e);
            return;
        }
    }
}

/**
 * @fn    IncSizeCheck(int width, int height, int *n_w, int *n_h)
 * @brief Calculate increasement sizes
 *
 * Given a new width and height this functions calculates if the windows
 * increasement sizes allows a resize of the window. The n_w parameter 
 * is used for returning allowed width and the n_h parameter is used for
 * returning allowed height.
 *
 * @param width Width we want to resize to
 * @param height Height we want to resize to
 * @param n_w Return of allowed width
 * @param n_h Return of allowed height
 *
 * @return True if a resize is allowed otherwise false
 */
bool WaWindow::IncSizeCheck(int width, int height, int *n_w, int *n_h) {
    bool resize = False;

    *n_w = attrib.width;
    *n_h = attrib.height;
    if ((width >= (attrib.width + size.width_inc)) ||
        (width <= (attrib.width - size.width_inc)) ||
	attrib.width == width) {
        if (width >= size.min_width && width <= size.max_width) {
            resize = True;
            if (size.width_inc == 1)
                *n_w = width;
            else
                *n_w = width - ((width - size.base_width) % size.width_inc);
        }
    }
    if ((height <= -(handle_w + border_w * 2)) && title_w) {
        if (! flags.shaded) {
            resize = True;
            flags.shaded = True;
            restore_shade.height = attrib.height;
        }
        *n_h = -(handle_w + border_w * 2);
        return resize;
    }
    if ((height >= (attrib.height + size.height_inc)) ||
        (height <= (attrib.height - size.height_inc)) ||
	attrib.height == height) {
        if ((height < 1) && (size.min_height <= 1) && title_w) {
            resize = True;
            if (! flags.shaded) {
                flags.shaded = True;
                restore_shade.height = attrib.height;
            }
            if (size.height_inc == 1)
                *n_h = height;
            else
                *n_h = height -
                    ((height - size.base_height) % size.height_inc);
        }
        else if (height >= size.min_height && height <= size.max_height) {
            resize = True;
            flags.shaded = False;
            if (size.height_inc == 1)
                *n_h = height;
            else
                *n_h = height -
                    ((height - size.base_height) % size.height_inc);
        }
    }
    return resize;
}

/**
 * @fn    Raise(XEvent *, WaAction *)
 * @brief Raises the window
 *
 * Raises the window to the top of the display stack and redraws window 
 * foreground.
 */
void WaWindow::Raise(XEvent *, WaAction *) {
    waimea->WaRaiseWindow(frame->id);
    if (title_w) {
        DrawLabel();
        DrawCloseButton();
        DrawMaxButton();
        DrawIconifyButton();							
    }
}

/**
 * @fn    Lower(XEvent *, WaAction *)
 * @brief Lowers the window
 *
 * Lowers the window to the bottom of the display stack
 */
void WaWindow::Lower(XEvent *, WaAction *) {
    XLowerWindow(display, frame->id);
}

/**
 * @fn    Focus(XEvent *, WaAction *)
 * @brief Sets input focus to the window
 *
 * Gives window keyboard input focus.
 */
void WaWindow::Focus(XEvent *, WaAction *) {
    if (mapped) {
        XGrabServer(display);
        if (validateclient(id)) {
            XSetInputFocus(display, id, RevertToPointerRoot, CurrentTime);
            XInstallColormap(display, attrib.colormap);
        }
        XUngrabServer(display);
    } else
        want_focus = True;
}

/**
 * @fn    Move(XEvent *e)
 * @brief Moves the window
 *
 * Moves the window through displaying a outline of the window while dragging
 * the mouse.
 *
 * @param e XEvent causing start of move
 */
void WaWindow::Move(XEvent *e, WaAction *) {
    XEvent *event;
    int px, py, nx, ny;
    list<XEvent *> *maprequest_list;
    bool started = False;
    
    switch (e->type) {
        case ButtonRelease:
            if (! mapped) {
                attrib.x = - (wascreen->v_x + attrib.width + border_w * 2);
                attrib.y = - (wascreen->v_y + attrib.height + border_w * 4 +
                              handle_w + title_w);
                RedrawWindow();
                net->SetState(this, NormalState);
            }
            nx = e->xbutton.x_root + border_w - 3;
            ny = e->xbutton.y_root + title_w +
                border_w - 3;
            px = e->xbutton.x_root;
            py = e->xbutton.y_root;
            DrawOutline(nx, ny, attrib.width, attrib.height);
            ToggleOutline();
            started = True;
            break;
        case ButtonPress:
            nx = attrib.x;
            ny = attrib.y;
            px = e->xbutton.x_root;
            py = e->xbutton.y_root; break;
        default: return;
    }
    maprequest_list = new list<XEvent *>;
    XGrabServer(display);
    if (validateclient(id))
        XGrabPointer(display, e->xany.window, True, ButtonReleaseMask |
                     ButtonPressMask | PointerMotionMask | EnterWindowMask |
                     LeaveWindowMask, GrabModeAsync, GrabModeAsync, None,
                     waimea->move_cursor, CurrentTime);
    XUngrabServer(display);
    for (;;) {
        event = waimea->eh->EventLoop(waimea->eh->moveresize_return_mask);
        switch (event->type) {
            case MotionNotify:
                nx += event->xmotion.x_root - px;
                ny += event->xmotion.y_root - py;
                px  = event->xmotion.x_root;
                py  = event->xmotion.y_root;
                if (! started) {
                    ToggleOutline();
                    started = True;
                }
                DrawOutline(nx, ny, attrib.width, attrib.height);
                break;
            case LeaveNotify:
            case EnterNotify:
                if (wascreen->west->id == event->xcrossing.window ||
                    wascreen->east->id == event->xcrossing.window ||
                    wascreen->north->id == event->xcrossing.window ||
                    wascreen->south->id == event->xcrossing.window) {
                    waimea->eh->ed.type = event->type;
                    waimea->eh->ed.mod = event->xcrossing.state;
                    waimea->eh->ed.detail = 0;
                    waimea->eh->EvAct(event, event->xcrossing.window);
                }
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
                    if (started) ToggleOutline();
                    return;
                }
                waimea->eh->EvUnmapDestroy(event);
                break;
            case ConfigureRequest:
                if (event->xconfigurerequest.window != id)
                    waimea->eh->EvConfigureRequest(&event->xconfigurerequest);
                break;
            case MapRequest:
                maprequest_list->push_front(event); break;
            case ButtonPress:
            case ButtonRelease:
                if (e->type == event->type) break;
                XUngrabPointer(display, CurrentTime);
                if (started) ToggleOutline();
                attrib.x = nx;
                attrib.y = ny;
                RedrawWindow();
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
 * @fn    MoveOpaque(XEvent *e, WaAction *)
 * @brief Moves the window
 *
 * Moves the window using the opaque moving process, which means that the
 * window is moved after the mouse pointers every motion event.
 *
 * @param e XEvent causing start of move
 */
void WaWindow::MoveOpaque(XEvent *e, WaAction *) {
    XEvent *event;
    int px, py, nx = attrib.x, ny = attrib.y;
    list<XEvent *> *maprequest_list;
    
    switch (e->type) {
        case ButtonRelease:
            nx = attrib.x = e->xbutton.x_root + border_w - 3;
            ny = attrib.y = e->xbutton.y_root + title_w + border_w - 3;
            if (! mapped) {
                RedrawWindow();
                net->SetState(this, NormalState);
            }
        case ButtonPress:
            px = e->xbutton.x_root;
            py = e->xbutton.y_root; break;
        default: return;
    }
    dontsend = True;
    maprequest_list = new list<XEvent *>;
    XGrabServer(display);
    if (validateclient(id))
        XGrabPointer(display, e->xany.window, True, ButtonReleaseMask |
                     ButtonPressMask | PointerMotionMask | EnterWindowMask |
                     LeaveWindowMask, GrabModeAsync, GrabModeAsync, None,
                     waimea->move_cursor, CurrentTime);
    XUngrabServer(display);
    for (;;) {
        event = waimea->eh->EventLoop(waimea->eh->moveresize_return_mask);
        switch (event->type) {
            case MotionNotify:
                nx += event->xmotion.x_root - px;
                ny += event->xmotion.y_root - py;
                px = event->xmotion.x_root;
                py = event->xmotion.y_root;
                attrib.x = nx;
                attrib.y = ny;
                RedrawWindow();
                break;
            case LeaveNotify:
            case EnterNotify:
                if (wascreen->west->id == event->xcrossing.window ||
                    wascreen->east->id == event->xcrossing.window ||
                    wascreen->north->id == event->xcrossing.window ||
                    wascreen->south->id == event->xcrossing.window) {
                    waimea->eh->ed.type = event->type;
                    waimea->eh->ed.mod = event->xcrossing.state;
                    waimea->eh->ed.detail = 0;
                    waimea->eh->EvAct(event, event->xcrossing.window);
                }
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
            case ConfigureRequest:
                if (event->xconfigurerequest.window != id)
                    waimea->eh->EvConfigureRequest(&event->xconfigurerequest);
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
                SendConfig();
                dontsend = False;
                return;
        }
    }
}

/**
 * @fn    Resize(XEvent *e, int how)
 * @brief Resizes the window
 *
 * Resizes the window through displaying a outline of the window while dragging
 * the mouse. If how parameter is RightType when the window is resized in 
 * south-east direction and if how parameter is LeftType the resize is
 * being performed in south-west direction.
 *
 * @param e XEvent causing start of resize
 */
void WaWindow::Resize(XEvent *e, int how) {
    XEvent *event;
    int px, py, width, height, n_w, n_h, o_w, o_h, n_x, o_x;
    list<XEvent *> *maprequest_list;
    bool started = False;
    
    switch (e->type) {
        case ButtonPress:
            px = e->xbutton.x_root;
            py = e->xbutton.y_root; break;
        default: return;
    }
    n_x    = o_x = attrib.x;
    width  = n_w = o_w = attrib.width;
    height = n_h = o_h = attrib.height;
    maprequest_list = new list<XEvent *>;
    XGrabServer(display);
    if (validateclient(id))
        XGrabPointer(display, e->xany.window, False, ButtonReleaseMask |
                     ButtonMotionMask, GrabModeAsync, GrabModeAsync, None,
                     (how > 0) ? waimea->resizeright_cursor:
                     waimea->resizeleft_cursor, CurrentTime);
    XUngrabServer(display);
    for (;;) {
        event = waimea->eh->EventLoop(waimea->eh->moveresize_return_mask);
        switch (event->type) {
            case MotionNotify:
                width  += (event->xmotion.x_root - px) * how;
                height += event->xmotion.y_root - py;
                px = event->xmotion.x_root;
                py = event->xmotion.y_root;
                if (IncSizeCheck(width, height, &n_w, &n_h)) {
                    if (how == WestType) n_x -= n_w - o_w;
                    if (! started) {
                        ToggleOutline();
                        started = True;
                    }
                    o_x = n_x;
                    o_w = n_w;
                    o_h = n_h;
                    DrawOutline(n_x, attrib.y, n_w, n_h);
                }
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
                    if (started) ToggleOutline();
                    return;
                }
                waimea->eh->EvUnmapDestroy(event);
                break;
            case ConfigureRequest:
                if (event->xconfigurerequest.window != id)
                    waimea->eh->EvConfigureRequest(&event->xconfigurerequest);
                break;
            case MapRequest:
                maprequest_list->push_front(event); break;
            case ButtonRelease:
                if (started) ToggleOutline();
                attrib.x      = n_x;
                attrib.width  = n_w;
                attrib.height = n_h;
                RedrawWindow();
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
 * @fn    ResizeOpaque(XEvent *e, int how)
 * @brief Resizes the window
 *
 * Resizes the window using the opaque resizing process, which means that 
 * the window is resized after the mouse pointers every motion event. 
 * If how parameter is RightType when the window is resized in 
 * south-east direction and if how parameter is LeftType the resize is
 * being performed in south-west direction.
 *
 * @param e XEvent causing start of resize
 */
void WaWindow::ResizeOpaque(XEvent *e, int how) {
    XEvent *event;
    int px, py, width, height, n_w, n_h;
    list<XEvent *> *maprequest_list;

    switch (e->type) {
        case ButtonPress:
            px = e->xbutton.x_root;
            py = e->xbutton.y_root; break;
        default: return;
    }
    dontsend = True;
    width  = n_w = attrib.width;
    height = n_h = attrib.height;
    maprequest_list = new list<XEvent *>;
    XGrabServer(display);
    if (validateclient(id))
        XGrabPointer(display, e->xany.window, False, ButtonReleaseMask |
                     ButtonMotionMask, GrabModeAsync, GrabModeAsync, None,
                     (how > 0) ? waimea->resizeright_cursor:
                     waimea->resizeleft_cursor, CurrentTime);
    XUngrabServer(display);
    for (;;) {
        event = waimea->eh->EventLoop(waimea->eh->moveresize_return_mask);
        switch (event->type) {
            case MotionNotify:
                width  += (event->xmotion.x_root - px) * how;
                height += event->xmotion.y_root - py;
                px  = event->xmotion.x_root;
                py  = event->xmotion.y_root;
                if (IncSizeCheck(width, height, &n_w, &n_h)) {
                    if (how == WestType) attrib.x -= n_w - attrib.width;
                    attrib.width  = n_w;
                    attrib.height = n_h;
                    RedrawWindow();
                }
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
            case ConfigureRequest:
                if (event->xconfigurerequest.window != id)
                    waimea->eh->EvConfigureRequest(&event->xconfigurerequest);
                break;
            case MapRequest:
                maprequest_list->push_front(event); break;
            case ButtonRelease:
                XUngrabPointer(display, CurrentTime);
                while (! maprequest_list->empty()) {
                    XPutBackEvent(display, maprequest_list->front());
                    maprequest_list->pop_front();
                }
                delete maprequest_list;
                SendConfig();
                dontsend = False;
                return;
        }
    }
}

/**
 * @fn    Maximize(XEvent *, WaAction *)
 * @brief Maximize the window
 *
 * Maximizes the window so that the window with it's decorations fills up
 * the hole screen.
 */
void WaWindow::Maximize(XEvent *, WaAction *) {
    net->SetStateMaxV(this, 2);
    net->SetStateMaxH(this, 1);
}

/**
 * @fn    UnMaximize(XEvent *, WaAction *)
 * @brief Unmaximize the window
 *
 * Restores size and position of maximized window.
 */
void WaWindow::UnMaximize(XEvent *, WaAction *) {
    net->SetStateMaxV(this, 3);
    net->SetStateMaxH(this, 0);
}

/**
 * @fn    ToggleMaximize(XEvent *e, WaAction *ac)
 * @brief Maximizes or unmaximize window
 *
 * If window isn't maximized this function maximizes it and if it is already
 * maximized then function will unmaximized window.
 *
 * @param e XEvent causing function call
 * @param ac WaAction object
 */
void WaWindow::ToggleMaximize(XEvent *e, WaAction *ac) {
    if (flags.max_v == flags.max_h) {
        net->SetStateMaxH(this, ! flags.max_h);
        net->SetStateMaxV(this, ! flags.max_v);
    }
    else if (flags.max_v)
        net->SetStateMaxV(this, 0);
    else
        net->SetStateMaxH(this, 0);
}

/**
 * @fn    MaximizeHorz(XEvent *, WaAction *)
 * @brief Maximize the window horizontally
 *
 * Maximizes the window so that the window with it's decorations fills up
 * the hole screen.
 */
void WaWindow::MaximizeHorz(XEvent *, WaAction *) {
    net->SetStateMaxH(this, 1);
}

/**
 * @fn    UnMaximizeHorz(XEvent *, WaAction *)
 * @brief Unmaximize the window horizontally
 *
 * Restores size and position of maximized window.
 */
void WaWindow::UnMaximizeHorz(XEvent *, WaAction *) {
    net->SetStateMaxH(this, 0);
}

/**
 * @fn    ToggleMaximizeHorz(XEvent *e, WaAction *ac)
 * @brief Maximizes or unmaximize window horizontally
 *
 * If window isn't maximized this function maximizes it and if it is already
 * maximized then function will unmaximized window.
 *
 * @param e XEvent causing function call
 * @param ac WaAction object
 */
void WaWindow::ToggleMaximizeHorz(XEvent *e, WaAction *ac) {
    net->SetStateMaxH(this, ! flags.max_h);
}

/**
 * @fn    MaximizeHorz(XEvent *, WaAction *)
 * @brief Maximize the window vertically
 *
 * Maximizes the window so that the window with it's decorations fills up
 * the hole screen.
 */
void WaWindow::MaximizeVert(XEvent *, WaAction *) {
    net->SetStateMaxV(this, 1);
}

/**
 * @fn    UnMaximizeHorz(XEvent *, WaAction *)
 * @brief Unmaximize the window vertically
 *
 * Restores size and position of maximized window.
 */
void WaWindow::UnMaximizeVert(XEvent *, WaAction *) {
    net->SetStateMaxV(this, 0);
}

/**
 * @fn    ToggleMaximizeVert(XEvent *e, WaAction *ac)
 * @brief Maximizes or unmaximize window vertically
 *
 * If window isn't maximized this function maximizes it and if it is already
 * maximized then function will unmaximized window.
 *
 * @param e XEvent causing function call
 * @param ac WaAction object
 */
void WaWindow::ToggleMaximizeVert(XEvent *e, WaAction *ac) {
    net->SetStateMaxV(this, ! flags.max_v);
}

/**
 * @fn    Close(XEvent *, WaAction *)
 * @brief Close the window
 *
 * Sends a delete message to the client window. A normal running X window
 * should accept this event and destroy itself. 
 */
void WaWindow::Close(XEvent *, WaAction *) {
    XEvent ev;

    ev.type = ClientMessage;
    ev.xclient.window = id;
    ev.xclient.message_type = XInternAtom(display, "WM_PROTOCOLS", False);
    ev.xclient.format = 32;
    ev.xclient.data.l[0] = XInternAtom(display, "WM_DELETE_WINDOW", False);
    ev.xclient.data.l[1] = CurrentTime;

    XGrabServer(display);
    if (validateclient(id))
        XSendEvent(display, id, False, NoEventMask, &ev);
    XUngrabServer(display);

}

/**
 * @fn    Kill(XEvent *, WaAction *)
 * @brief Kill the window
 *
 * Tells the the X server to remove the window from the screen through
 * killing the process that created it.
 */
void WaWindow::Kill(XEvent *, WaAction *) {
    XGrabServer(display);
    if (validateclient(id))
        XKillClient(display, id);
    XUngrabServer(display);
}

/**
 * @fn    CloseKill(XEvent *e, WaAction *ac)
 * @brief Close/Kill the window
 *
 * Checks if the window will accept a delete message. If it will, then we
 * use that method for closing the window otherwise we use the kill method.
 *
 * @param e XEvent causing close/kill of window
 * @param ac WaAction object
 */
void WaWindow::CloseKill(XEvent *e, WaAction *ac) {
    int i, n;
    bool close = False;
    Atom *protocols;
    Atom del_atom = XInternAtom(display, "WM_DELETE_WINDOW", False);

    XGrabServer(display);
    if (validateclient(id))
        if (XGetWMProtocols(display, id, &protocols, &n)) {
            for (i = 0; i < n; i++) if (protocols[i] == del_atom) close = True;
            XFree(protocols);
        }
    XUngrabServer(display);
    if (close) Close(e, ac);
    else Kill(e, ac);
}

/**
 * @fn    MenuMap(XEvent *e, WaAction *ac)
 * @brief Maps a menu
 *
 * Links the window to the menu and maps it at the position of the button
 * event causing mapping. If menu is already mapped nothing is done.
 *
 * @param e XEvent causing mapping of menu
 * @param ac WaAction object
 */
void WaWindow::MenuMap(XEvent *e, WaAction *ac) {
    Window w;
    int i, rx, ry;
    unsigned int ui;
    WaMenu *menu = (WaMenu *) ac->param;
    
    if (XQueryPointer(display, wascreen->id, &w, &w, &rx, &ry, &i, &i, &ui)) {
        menu->wf = id;
        menu->ftype = MenuWFuncMask;
        menu->Map(rx - (menu->width / 2),
                  ry - (menu->item_list->front()->height / 2));
    }
}

/**
 * @fn    MenuMap(XEvent *e)
 * @brief Remaps a menu
 *
 * Links the window to the menu and maps it at the position of the button
 * event causing mapping. If the window is already mapped then we just move
 * it to the new position.
 *
 * @param e XEvent causing remapping of menu
 * @param ac WaAction object
 */
void WaWindow::MenuReMap(XEvent *e, WaAction *ac) {
    Window w;
    int i, rx, ry;
    unsigned int ui;
    WaMenu *menu = (WaMenu *) ac->param;
    
    if (XQueryPointer(display, wascreen->id, &w, &w, &rx, &ry, &i, &i, &ui)) {
        menu->wf = id;
        menu->ftype = MenuWFuncMask;
        menu->ReMap(rx - (menu->width / 2),
                    ry - (menu->item_list->front()->height / 2));
    }
}

/**
 * @fn    MenuUnmap(XEvent *, WaAction *ac)
 * @brief Unmaps a menu
 *
 * Unmaps a menu and all its submenus.
 *
 * @param ac WaAction object
 */
void WaWindow::MenuUnmap(XEvent *, WaAction *ac) {
    ((WaMenu *)(ac->param))->Unmap();
    ((WaMenu *)(ac->param))->UnmapSubmenus();
}

/**
 * @fn    Shade(XEvent *, WaAction *)
 * @brief Shade window
 *
 * Resizes window to so that only the titlebar is shown. Remembers previous
 * height of window that will be restored when unshading.
 */
void WaWindow::Shade(XEvent *, WaAction *) {
    net->SetStateShaded(this, True);
}

/**
 * @fn    UnShade(XEvent *, WaAction *)
 * @brief Unshade window
 *
 * Restores height of shaded window.
 */
void WaWindow::UnShade(XEvent *, WaAction *) {
    net->SetStateShaded(this, False);
}

/**
 * @fn    ToggleShade(XEvent *e, WaAction *ac)
 * @brief Shades window or unshades it
 *
 * If window isn't shaded this function will shade it and if it is already
 * shaded function unshades it.
 *
 * @param e XEvent causing function call
 * @param ac WaAction object
 *
 */
void WaWindow::ToggleShade(XEvent *e, WaAction *ac) {
    net->SetStateShaded(this, ! flags.shaded);
}

/**
 * @fn    Sticky(XEvent *, WaAction *)
 * @brief Makes window sticky
 *
 * Sets the sticky flag to True. This makes viewport moving functions
 * to ignore this window.
 */
void WaWindow::Sticky(XEvent *, WaAction *) {
    net->SetStateSticky(this, True);
}

/**
 * @fn    UnSticky(XEvent *, WaAction *)
 * @brief Makes window normal
 *
 * Sets the sticky flag to False. This makes viewport moving functions
 * treat this window as a normal window.
 */
void WaWindow::UnSticky(XEvent *, WaAction *) {
    net->SetStateSticky(this, False);
}

/**
 * @fn    ToggleSticky(XEvent *, WaAction *)
 * @brief Toggles sticky flag
 *
 * Inverts the sticky flag.
 */
void WaWindow::ToggleSticky(XEvent *, WaAction *) {
    net->SetStateSticky(this, ! flags.sticky);
}

/**
 * @fn    EvAct(XEvent *e, EventDetail *ed, list<WaAction *> *acts,
 *              int etype)
 * @brief Calls WaWindow function
 *
 * Tries to match an occurred X event with the actions in an action list.
 * If we have a match then we execute that action.
 *
 * @param e X event that have occurred
 * @param ed Event details
 * @param acts List with actions to match event with
 * @param etype Type of window event occurred on
 */
void WaWindow::EvAct(XEvent *e, EventDetail *ed, list<WaAction *> *acts,
                int etype) {
    list<WaAction *>::iterator it = acts->begin();
    for (; it != acts->end(); ++it) {
        if (eventmatch(*it, ed)) {
            ((*this).*((*it)->winfunc))(e, *it);
        }
    }
    switch (etype) {
        case WindowType:
            if (ed->type == MapRequest) {
                net->SetState(this, NormalState);
                net->SetVirtualPos(this);
            }
            break;
        case CButtonType:
        case IButtonType:
        case MButtonType:
            if (ed->type == ButtonPress)
                ButtonPressed(etype);
            if (ed->type == EnterNotify)
                ButtonHilite(etype);
            else if (ed->type == LeaveNotify)
                ButtonDehilite(etype);
    }
}

/**
 * @fn    WaChildWindow(WaWindow *wa_win, Window parent, int bw, int type) :
 *        WindowObject(0, type)
 * @brief Constructor for WaChildWindow class
 *
 * Creates a child window, could be of one of these types: FrameType,
 * TitleType, LabelType, HandleType, CButtonType, IButtonType, MButtonType,
 * LGripType, RGripType.
 *
 * @param wa_win WaWindow who wish to use the child window
 * @param parent Parent window to child window
 * @param bw Border width
 * @param type Type of window
 */
WaChildWindow::WaChildWindow(WaWindow *wa_win, Window parent, int bw, int type) :
    WindowObject(0, type) {
    XSetWindowAttributes attrib_set;
    
    wa = wa_win;
    int create_mask = CWOverrideRedirect | CWBackPixel | CWEventMask |
        CWColormap | CWBorderPixel;
    attrib_set.background_pixel = None;
    attrib_set.border_pixel = wa->wascreen->wstyle.border_color.getPixel();
    attrib_set.colormap = wa->wascreen->colormap;
    attrib_set.override_redirect = True;
    attrib_set.event_mask = ButtonPressMask | ButtonReleaseMask |
        EnterWindowMask | LeaveWindowMask;
    
    switch (type) {
        case FrameType:
            attrib_set.event_mask |= SubstructureRedirectMask;
            attrib.x = wa->attrib.x - wa->border_w;
            attrib.y = wa->attrib.y - wa->title_w - wa->border_w * 2;
            attrib.width = wa->attrib.width;
            attrib.height = wa->attrib.height + wa->title_w + wa->handle_w +
                wa->border_w * 2;
            break;
        case TitleType:
            attrib.x = - wa->border_w;
            attrib.y = - wa->border_w;
            attrib.width  = wa->attrib.width;
            attrib.height = wa->title_w;
            break;
        case LabelType:
            attrib_set.event_mask |= ExposureMask;
            attrib.x = wa->title_w;
            attrib.y = 2;
            attrib.width  = wa->attrib.width - 3 * wa->title_w + 2;
            if (attrib.width < 1) attrib.width = 1;
            attrib.height = wa->title_w - 4;
            break;
        case HandleType:
            attrib.x = 25;
            attrib.y = wa->attrib.height + wa->title_w + wa->border_w;
            attrib.width = wa->attrib.width - 50 - wa->border_w * 2;
            if (attrib.width < 1) attrib.width = 1;
            attrib.height = wa->wascreen->wstyle.handle_width;
            break;
        case CButtonType:
            attrib_set.event_mask |= ExposureMask;
            attrib.x = wa->attrib.width - (wa->title_w - 2);
            attrib.y = 2;
            attrib.width  = wa->title_w - 4;
            attrib.height = wa->title_w - 4;
            break;
        case IButtonType:
            attrib_set.event_mask |= ExposureMask;
            attrib.x = 2;
            attrib.y = 2;
            attrib.width  =  wa->title_w - 4;
            attrib.height = wa->title_w - 4;
            break;
        case MButtonType:
            attrib_set.event_mask |= ExposureMask;
            attrib.x = wa->attrib.width - (wa->title_w - 2) * 2;
            attrib.y = 2;
            attrib.width  = wa->title_w - 4;
            attrib.height = wa->title_w - 4;
            break;
        case LGripType:
            attrib.x = - wa->border_w;
            attrib.y = wa->attrib.height + wa->title_w + wa->border_w;
            attrib.width  = 25;
            attrib.height = wa->wascreen->wstyle.handle_width;
            create_mask |= CWCursor;
            attrib_set.cursor = wa->waimea->resizeleft_cursor;
            break;
        case RGripType:
            attrib.x = wa->attrib.width - 25 - wa->border_w;
            attrib.y = wa->attrib.height + wa->title_w + wa->border_w;
            attrib.width  = 25;
            attrib.height = wa->wascreen->wstyle.handle_width;
            create_mask |= CWCursor;
            attrib_set.cursor = wa->waimea->resizeright_cursor;
            break;
    }
    id = XCreateWindow(wa->display, parent, attrib.x, attrib.y,
                       attrib.width, attrib.height, bw, wa->screen_number,
                       CopyFromParent, wa->wascreen->visual, create_mask,
                       &attrib_set);

    wa->waimea->window_table->insert(make_pair(id, this));
}

/**
 * @fn    ~WaChildWindow()
 * @brief Destructor for WaChildWindow class
 *
 * Destroys the window and removes it from the window_table hash_map.
 */
WaChildWindow::~WaChildWindow(void) {
    wa->waimea->window_table->erase(id);
}
