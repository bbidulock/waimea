/**
 * @file   WaMenu.cc
 * @author David Reveman <c99drn@cs.umu.se>
 * @date   02-Aug-2001 22:40:20
 *
 * @brief Implementation of WaMenu, WaMenuItem and TaskSwitcher classes  
 *
 * Implementation of menu system. A menu consists of a list of menu items
 * each menu item can be one of these three kinds Title, Item or Sub. The only
 * thing separating these items are the looks of them and action lists. All
 * three types can perform the same actions. Possible actions are execution of
 * program, call to function and mapping of menu as submenu.
 *
 * Copyright (C) David Reveman. All rights reserved.
 *
 */

#include <stdio.h>
#include <string.h>

#include "WaMenu.hh"
#include "EventHandler.hh"

/**
 * @fn    WaMenu(char *n)
 * @brief Constructor for WaMenu class
 *
 * Creates a new menu with no items
 *
 * @param n Name of menu
 */
WaMenu::WaMenu(char *n) {
    name = n;
    
    height = 0;
    width = 0;
    mapped = has_focus = built = tasksw = false;
    root_menu = NULL;
    root_item = NULL;
    wf = (Window) 0;
    rf = NULL;
    mf = NULL;

#ifdef XRENDER
    pixmap = None;
#endif // XRENDER
    
    item_list = new list<WaMenuItem *>;
}

/**
 * @fn    ~WaMenu(void)
 * @brief Destructor for WaMenu class
 *
 * Deletes all menu items in the menu and removes the frame.
 */
WaMenu::~WaMenu(void) {
    LISTCLEAR2(item_list);
    if (built) {
        XDestroyWindow(display, frame);
        waimea->always_on_top_list->remove(o_west);
        waimea->always_on_top_list->remove(o_east);
        waimea->always_on_top_list->remove(o_north);
        waimea->always_on_top_list->remove(o_south);
        XDestroyWindow(display, o_west);
        XDestroyWindow(display, o_east);
        XDestroyWindow(display, o_north);
        XDestroyWindow(display, o_south);

#ifdef XRENDER
        if (pixmap != None) XFreePixmap(display, pixmap);
#endif // XRENDER

    }
    delete [] name;
}

/**
 * @fn    AddItem(WaMenuItem *item)
 * @brief Adds item to menu
 *
 * Adds item to menu by inserting a WaMenuItem object to the menus item list.
 * 
 * @param item Item to add
 */
void WaMenu::AddItem(WaMenuItem *item) {
    item->menu = this;
    item->hilited = false;
    item_list->push_back(item);
}

/**
 * @fn    Build(Wascreen *screen)
 * @brief Builds Menu
 *
 * Calculates size of menu, finds submenus sets pointers to them, renders 
 * graphics and creates windows. If menu has already been built, when we just 
 * resize the windows instead of creating new ones. 
 *
 * @param screen Screen to create graphics and windows in
 */
void WaMenu::Build(WaScreen *screen) {
    XSetWindowAttributes attrib_set;
    unsigned int i;

    height = 0;
    width = 0;
    
    if (! built) {
        wascreen = screen;
        waimea = wascreen->waimea;
        display = wascreen->display;
        ic = wascreen->ic;
        CreateOutlineWindows();
    }
    bullet_width = cb_width = 0;

    f_height = wascreen->mstyle.item_height;
    t_height = wascreen->mstyle.title_height;
    
    list<WaMenu *>::iterator menu_it;
    list<WaMenuItem *>::iterator it = item_list->begin();
    for (; it != item_list->end(); ++it) {
        if ((*it)->func_mask & MenuSubMask) {
            menu_it = waimea->wamenu_list->begin();
            for (; menu_it != waimea->wamenu_list->end(); ++menu_it) {
                if (! strcmp((*menu_it)->name, (*it)->sub)) {
                    (*it)->submenu = *menu_it;
                    break;
                }
            }
            if (menu_it == waimea->wamenu_list->end()) {
                WARNING << "no menu named \"" << (*it)->sub << "\"" << endl;
                delete *it;
                it = item_list->begin();
            }
        }
        if ((*it)->func_mask2 & MenuSubMask) {
            menu_it = waimea->wamenu_list->begin();
            for (; menu_it != waimea->wamenu_list->end(); ++menu_it) {
                if (! strcmp((*menu_it)->name, (*it)->sub2)) {
                    (*it)->submenu2 = *menu_it;
                    break;
                }
            }
            if (menu_it == waimea->wamenu_list->end()) {
                WARNING << "no menu named \"" << (*it)->sub2 << "\"" << endl;
                delete *it;
                it = item_list->begin();
            }
        }
    }

#ifdef XFT
    XGlyphInfo extents;
    XftFont *xft_font;
    for (it = item_list->begin(); it != item_list->end(); ++it) {
        if ((*it)->type == MenuSubType && wascreen->mstyle.wa_f_font.xft) {
            XftTextExtents8(display, wascreen->mstyle.b_xftfont,
                            (unsigned char *) wascreen->mstyle.bullet,
                            strlen(wascreen->mstyle.bullet), &extents);
            bullet_width = extents.width;
        }
        else if ((*it)->type == MenuCBItemType) {
            if (wascreen->mstyle.wa_ct_font.xft) {
                XftTextExtents8(display, wascreen->mstyle.ct_xftfont,
                                (unsigned char *)
                                wascreen->mstyle.checkbox_true,
                                strlen(wascreen->mstyle.checkbox_true),
                                &extents);
                (*it)->cb_width2 = cb_width = extents.width;
            }

            if (wascreen->mstyle.wa_cf_font.xft) {
                XftTextExtents8(display, wascreen->mstyle.cf_xftfont,
                                (unsigned char *)
                                wascreen->mstyle.checkbox_false,
                                strlen(wascreen->mstyle.checkbox_false),
                                &extents);
                if (extents.width > cb_width) cb_width = extents.width;
                (*it)->cb_width1 = extents.width;
            }
        }
    }
#endif // XFT
    
    XFontStruct *font;
    int tmp_w;
    for (it = item_list->begin(); it != item_list->end(); ++it) {    
        if ((*it)->type == MenuSubType && !wascreen->mstyle.wa_f_font.xft)
            
            bullet_width = XTextWidth(wascreen->mstyle.b_font,
                                      wascreen->mstyle.bullet,
                                      strlen(wascreen->mstyle.bullet));
        else if ((*it)->type == MenuCBItemType) {
            if (! wascreen->mstyle.wa_ct_font.xft) {
                (*it)->cb_width2 = cb_width = XTextWidth(
                    wascreen->mstyle.ct_font,
                    wascreen->mstyle.checkbox_true,
                    strlen(wascreen->mstyle.checkbox_true));
                if (! wascreen->mstyle.wa_cf_font.xft) {
                    tmp_w = XTextWidth(wascreen->mstyle.cf_font,
                                       wascreen->mstyle.checkbox_false,
                                       strlen(
                                           wascreen->mstyle.checkbox_false));
                    if (tmp_w > cb_width) cb_width = tmp_w;
                    (*it)->cb_width1 = tmp_w;
                }
            }
        }
    }
    
    extra_width = (bullet_width >= cb_width) ? bullet_width: cb_width;
    
    int lasttype = 0;
    it = item_list->begin();
    for (i = 1; it != item_list->end(); ++it, ++i) {
#ifdef XFT
        xft_font = NULL;
        if ((*it)->type == MenuTitleType && wascreen->mstyle.wa_t_font.xft)
            xft_font = wascreen->mstyle.t_xftfont;
        else if (wascreen->mstyle.wa_f_font.xft)
            xft_font = wascreen->mstyle.f_xftfont;

        if (xft_font) {
            XftTextExtents8(display, xft_font, (unsigned char *) (*it)->label,
                            strlen((*it)->label), &extents);
            (*it)->width = extents.width + 20;
            if ((*it)->type == MenuCBItemType) {
                XftTextExtents8(display, xft_font, (unsigned char *)
                                (*it)->label2, strlen((*it)->label2),
                                &extents);
                if ((extents.width + 20) > (*it)->width)
                    (*it)->width = extents.width + 20;
            }
        }
#endif // XFT
        
        font = NULL;
        if ((*it)->type == MenuTitleType && !wascreen->mstyle.wa_t_font.xft)
            font = wascreen->mstyle.t_font;
        else if (!wascreen->mstyle.wa_f_font.xft)
            font = wascreen->mstyle.f_font;

        if (font) {
            (*it)->width = XTextWidth(font, (*it)->label,
                                      strlen((*it)->label)) + 20;
            if ((*it)->type == MenuCBItemType) {
                tmp_w = XTextWidth(font, (*it)->label2,
                                   strlen((*it)->label2)) + 20;
                if (tmp_w > (*it)->width) (*it)->width = tmp_w;
            }
        }
        
        if (((*it)->width + extra_width) > width)
            width = (*it)->width + extra_width;
        
        if ((*it)->type == MenuTitleType) {
            if ((i == 1) || (i == item_list->size()) ||
                (lasttype == MenuTitleType)) {
                height += t_height + wascreen->wstyle.border_width;
                (*it)->realheight = t_height + wascreen->wstyle.border_width;
            }
            else {
                height += t_height + wascreen->wstyle.border_width * 2;
                (*it)->realheight = t_height + wascreen->wstyle.border_width *
                    2;
            }
            (*it)->height = t_height;
        }
        else {
            height += f_height;
            (*it)->height = (*it)->realheight = f_height;
        }
        lasttype = (*it)->type;
    }
    if (width > (wascreen->width / 2)) width = wascreen->width / 2;
    
    WaTexture *texture = &wascreen->mstyle.back_frame;
    if (texture->getTexture() == (WaImage_Flat | WaImage_Solid)) {
        pbackframe = None;
        backframe_pixel = texture->getColor()->getPixel();
    } else
        pbackframe = ic->renderImage(width, height, texture);
    
#ifdef XRENDER
    pixmap = XCreatePixmap(display, wascreen->id, width,
                           height, wascreen->screen_depth);
#endif // XRENDER
    
    texture = &wascreen->mstyle.title;
    if (texture->getTexture() == (WaImage_Flat | WaImage_Solid)) {
        ptitle = None;
        title_pixel = texture->getColor()->getPixel();
    } else
        ptitle = ic->renderImage(width, t_height, texture);
    
    texture = &wascreen->mstyle.hilite;
    if (texture->getTexture() == (WaImage_Flat | WaImage_Solid)) {
        philite = None;
        hilite_pixel = texture->getColor()->getPixel();
    } else
        philite = ic->renderImage(width, f_height, texture);
    
    attrib_set.background_pixmap = ParentRelative;
    attrib_set.border_pixel = wascreen->mstyle.border_color.getPixel();
    attrib_set.colormap = wascreen->colormap;
    attrib_set.override_redirect = true;
    attrib_set.event_mask = NoEventMask;
    
    if (! built) {
        frame = XCreateWindow(display, wascreen->id, 0, 0, width, height,
                              wascreen->mstyle.border_width,
                              wascreen->screen_number,
                              CopyFromParent, wascreen->visual,
                              CWOverrideRedirect | CWBackPixmap | CWEventMask |
                              CWColormap | CWBorderPixel, &attrib_set);
        if (waimea->rh->menu_stacking == AlwaysOnTop)
            waimea->always_on_top_list->push_back(frame);
        else if (waimea->rh->menu_stacking == AlwaysAtBottom)
            waimea->always_at_bottom_list->push_back(frame);
    } else XResizeWindow(display, frame, width, height);

    if (pbackframe)
        XSetWindowBackgroundPixmap(display, frame, pbackframe);
    else
        XSetWindowBackground(display, frame, backframe_pixel);

    XClearWindow(display, frame);

    attrib_set.event_mask = ButtonPressMask | ButtonReleaseMask |
        EnterWindowMask | LeaveWindowMask | KeyPressMask |
        KeyReleaseMask | ExposureMask | FocusChangeMask;

    int y, x, bw;
    it = item_list->begin();
    for (y = 0, lasttype = 0; it != item_list->end(); ++it) {
        x = bw = 0;
        if ((*it)->type == MenuTitleType) {
            bw = wascreen->mstyle.border_width;
            x = -bw;
            if ((y == 0) || (lasttype == MenuTitleType))
                y -= bw;
        }
        (*it)->id = XCreateWindow(display, frame, x, y, width,
                                  (*it)->height, bw,
                                  wascreen->screen_number, CopyFromParent,
                                  wascreen->visual, CWOverrideRedirect | 
                                  CWBackPixel | CWEventMask | CWColormap,
                                  &attrib_set);
        waimea->window_table->insert(make_pair((*it)->id, (*it)));
        (*it)->dy = y;
        y += (*it)->height + bw * 2;
#ifdef XFT
        (*it)->xftdraw = XftDrawCreate(display, (Drawable) (*it)->id,
                                       wascreen->visual,
                                       wascreen->colormap);
#endif // XFT

#ifdef XRENDER
        (*it)->pixmap = XCreatePixmap(display, wascreen->id, width,
                                      (*it)->height, wascreen->screen_depth);
#endif // XRENDER
        
        if ((*it)->type == MenuTitleType) {
            (*it)->texture = &wascreen->mstyle.title;
            if (ptitle)
                XSetWindowBackgroundPixmap(display, (*it)->id, ptitle);
            else
                XSetWindowBackground(display, (*it)->id, title_pixel);
        } else {
            (*it)->texture = &wascreen->mstyle.back_frame;
            XSetWindowBackgroundPixmap(display, (*it)->id, ParentRelative);
            XClearWindow(display, (*it)->id);
        }
        
        lasttype = (*it)->type;
    }
    built = true;
}

#ifdef XRENDER
/**
 * @fn    Render(void)
 * @brief Render menu background
 *
 * Renders frame background and all item backgrounds.
 */
void WaMenu::Render(void) {
    if (((x + width) > 0 && x < wascreen->width) && 
        ((y + height) > 0 && y < wascreen->height)) {
        WaTexture *texture = &wascreen->mstyle.back_frame;
        if (texture->getOpacity()) {
            pixmap = wascreen->ic->xrender(pbackframe, width, height, texture,
                                           wascreen->xrootpmap_id,
                                           x + wascreen->mstyle.border_width,
                                           y +wascreen->mstyle.border_width,
                                           pixmap);
            XSetWindowBackgroundPixmap(display, frame, pixmap);
            XClearWindow(display, frame);
        }
        list<WaMenuItem *>::iterator it = item_list->begin();
        for (; it != item_list->end(); ++it) {
            (*it)->Render();
            (*it)->DrawFg();
        }
    }
}
#endif // XRENDER

/**
 * @fn    Map(int mx, int my)
 * @brief Maps menu
 *
 * Maps menu at specified position if not already mapped. If menu is already 
 * mapped we do nothing. 
 *
 * @param mx X coordinate to map menu at
 * @param my Y coordinate to map menu at
 */
void WaMenu::Map(int mx, int my) {
    if (tasksw && item_list->size() < 2) return;
    if (mapped) return;
    
    x = mx;
    y = my;
    mapped = true;
    has_focus = false;
    XMoveWindow(display, frame, x, y);
    XMapSubwindows(display, frame);
    XMapWindow(display, frame);
    waimea->WaRaiseWindow(frame);

#ifdef XRENDER
    Render();
#endif // XRENDER
    
    XUngrabPointer(display, CurrentTime);
}

/**
 * @fn    ReMap(int mx, int my)
 * @brief Remaps menu
 *
 * Maps menu at specified position if not already mapped. If menu is already 
 * mapped then we move it to the position we want to remap it to
 *
 * @param mx X coordinate to remap menu at
 * @param my Y coordinate to remap menu at
 */
void WaMenu::ReMap(int mx, int my) {
    if (tasksw && item_list->size() < 2) return;
    
    if (mapped) Move(mx - x, my - y);
    x = mx;
    y = my;
    mapped = true;
    has_focus = false;
    XMoveWindow(display, frame, x, y);
    XMapSubwindows(display, frame);
    XMapWindow(display, frame);
    waimea->WaRaiseWindow(frame);

#ifdef XRENDER
    Render();
#endif // XRENDER
    
    XUngrabPointer(display, CurrentTime);
}

/**
 * @fn    Move(int dx, int dy)
 * @brief Moves menu
 *
 * Moves menu and all linked submenus to a specified position.
 *
 * @param dx Move in x coordinate relative to old position
 * @param dy Move in y coordinate relative to old position
 */
void WaMenu::Move(int dx, int dy) {
    list<WaMenuItem *>::iterator it = item_list->begin();
    for (; it != item_list->end(); ++it) {
        if (((*it)->func_mask & MenuSubMask) && (*it)->submenu->root_menu &&
            (*it)->submenu->mapped) {
            (*it)->submenu->Move(dx, dy);
        }
    }
    x += dx;
    y += dy;
    XMoveWindow(display, frame, x, y);

#ifdef XRENDER
    Render();
#endif // XRENDER
    
}

/**
 * @fn    Unmap(bool focus)
 * @brief Unmaps menu
 *
 * Unmaps menu and dehilites root menu item if menu was mapped as a submenu.
 *
 * @param focus True if we should set focus to root item
 */
void WaMenu::Unmap(bool focus) {
    XEvent e;

    XUnmapWindow(display, frame);

    list<WaMenuItem *>::iterator it = item_list->begin();
    for (; it != item_list->end(); ++it) {
        if ((*it)->hilited) {
            if ((*it)->func_mask & MenuSubMask) {
                if (! (*it)->submenu->mapped)
                    (*it)->DeHilite();
            }
            else
                (*it)->DeHilite();
        }
    }
    if (focus) {
        XSync(display, false);
        while (XCheckTypedEvent(display, EnterNotify, &e));
    }
    if (root_item) {
        if (focus)
            root_item->Focus();
        else
            root_item->DeHilite();
    }
    else {
        if (! waimea->wawindow_list->empty())
            waimea->wawindow_list->front()->Focus(false);
    }
    root_item = NULL;
    mapped = false;
}

/**
 * @fn    UnmapSubmenus(bool focus)
 * @brief Unmaps submenus
 *
 * Recursive function for unmapping submenu trees. Unmaps all submenus in the 
 * current subtree that are still linked.
 *
 * @param focus True if we should set focus to root item
 */
void WaMenu::UnmapSubmenus(bool focus) {
    list<WaMenuItem *>::iterator it = item_list->begin();
    for (; it != item_list->end(); ++it) {
        if (((*it)->func_mask & MenuSubMask) && (*it)->submenu->root_menu &&
            (*it)->submenu->mapped) {
            (*it)->submenu->UnmapSubmenus(focus);
            (*it)->submenu->Unmap(focus);
        }
    }
}

/**
 * @fn    UnmapTree(void)
 * @brief Unmaps menu tree
 *
 * Recursive function for unmapping complete menu trees. Unmaps all menus that 
 * are still linked in the current menu tree.
 */
void WaMenu::UnmapTree(void) {
    if (root_menu) {
        root_menu->UnmapTree();
    }
    UnmapSubmenus(false);
    Unmap(false);
}

/**
 * @fn    CreateOutlineWindows(void)
 * @brief Creates outline windows
 *
 * Creates four windows used for displaying an outline when doing
 * non opaque moving of the menu.
 */
void WaMenu::CreateOutlineWindows(void) {
    XSetWindowAttributes attrib_set;
    
    int create_mask = CWOverrideRedirect | CWBackPixel | CWEventMask |
        CWColormap;
    attrib_set.background_pixel = wascreen->wstyle.outline_color.getPixel();
    attrib_set.colormap = wascreen->colormap;
    attrib_set.override_redirect = true;
    attrib_set.event_mask = NoEventMask;
    
    o_west = XCreateWindow(display, wascreen->id, 0, 0, 1, 1, 0,
                           wascreen->screen_number, CopyFromParent,
                           wascreen->visual, create_mask, &attrib_set);
    o_east = XCreateWindow(display, wascreen->id, 0, 0, 1, 1, 0,
                           wascreen->screen_number, CopyFromParent,
                           wascreen->visual, create_mask, &attrib_set);
    o_north = XCreateWindow(display, wascreen->id, 0, 0, 1, 1, 0,
                            wascreen->screen_number, CopyFromParent,
                            wascreen->visual, create_mask, &attrib_set);
    o_south = XCreateWindow(display, wascreen->id, 0, 0, 1, 1, 0,
                            wascreen->screen_number, CopyFromParent,
                            wascreen->visual, create_mask, &attrib_set);
    waimea->always_on_top_list->push_back(o_west);
    waimea->always_on_top_list->push_back(o_east);
    waimea->always_on_top_list->push_back(o_north);
    waimea->always_on_top_list->push_back(o_south);
    o_mapped = false;
}

/**
 * @fn    ToggleOutline(void)
 * @brief Toggles outline windows on/off
 *
 * Recursive function that un-/ maps outline windows.
 */
void WaMenu::ToggleOutline(void) {
    list<WaMenuItem *>::iterator it = item_list->begin();
    for (; it != item_list->end(); ++it) {        
        if (((*it)->func_mask & MenuSubMask) && (*it)->submenu->root_menu &&
            (*it)->submenu->mapped) {
            (*it)->submenu->ToggleOutline();
        }
    }
    if (o_mapped) {
        XUnmapWindow(display, o_west);
        XUnmapWindow(display, o_east);
        XUnmapWindow(display, o_north);
        XUnmapWindow(display, o_south);
        o_mapped = false;
    }
    else {
        XMapWindow(display, o_west);
        XMapWindow(display, o_east);
        XMapWindow(display, o_north);
        XMapWindow(display, o_south);
        waimea->WaRaiseWindow(0);
        o_mapped = true;
    }       
}

/**
 * @fn    DrawOutline(int dx, int dy)
 * @brief Draws menu outline
 *
 * Recursive function for drawing a outline for a menu tree. Draws outline for
 * all menu in the current menu subtree that are still linked. This function
 * is used for none opaque moving.
 *
 * @param dx X position relative to menu position to draw outline
 * @param dy Y position relative to menu position to draw outline
 */
void WaMenu::DrawOutline(int dx, int dy) {
    list<WaMenuItem *>::iterator it = item_list->begin();
    for (; it != item_list->end(); ++it) {        
        if (((*it)->func_mask & MenuSubMask) && (*it)->submenu->root_menu &&
            (*it)->submenu->mapped) {
            (*it)->submenu->DrawOutline(dx, dy);
        }
    }
    int bw = wascreen->mstyle.border_width;
    XResizeWindow(display, o_west, bw, bw * 2 + height);
    XResizeWindow(display, o_east, bw, bw * 2 + height);
    XResizeWindow(display, o_north, width + bw * 2, bw);
    XResizeWindow(display, o_south, width + bw * 2, bw);
    int xx = x + dx;
    int yy = y + dy;
    XMoveWindow(display, o_west, xx, yy);
    XMoveWindow(display, o_east, xx + width + bw, yy);
    XMoveWindow(display, o_north, xx, yy);
    XMoveWindow(display, o_south, xx, yy + bw + height);
}

/**
 * @fn    Raise(void)
 * @brief Raise menu window in display stack
 *
 * Raises the menu frame to top of all non-alwaysontop windows in the display
 * stack.
 */
void WaMenu::Raise(void) {
    waimea->WaRaiseWindow(frame);
    list<WaMenuItem *>::iterator it = item_list->begin();
    for (; it != item_list->end(); ++it)
        (*it)->DrawFg();
}

/**
 * @fn    FocusFirst(void)
 * @brief Focus first selectable item in menu
 *
 * Set input focus to first element of type [item] or [sub] in menu.
 */
void WaMenu::FocusFirst(void) {
    XEvent e;
    
    XSync(display, false);
    list<WaMenuItem *>::iterator it = item_list->begin();
    for (; it != item_list->end(); ++it)
        while (XCheckTypedWindowEvent(display, (*it)->id, EnterNotify, &e));
    it = item_list->begin();
    for (; it != item_list->end() &&
             (*it)->type == MenuTitleType; ++it);
    if (it != item_list->end()) (*it)->Focus();
}

/**
 * @fn    WaMenuItem(char *s) : WindowObject(0, 0)
 * @brief Constructor for WaMenuItem class
 *
 * Creates a new menu item
 *
 * @param s Menu item label
 */
WaMenuItem::WaMenuItem(char *s) : WindowObject(0, 0) {
    label = label1 = s;
    id = (Window) 0;
    func_mask = func_mask1 = func_mask2 = height = width = dy =
        realheight = cb = 0;
    wfunc = wfunc2 = NULL;
    rfunc = rfunc2 = NULL;
    mfunc = mfunc2 = NULL;
    wf = (Window) 0;
    submenu = submenu1 = submenu2 = NULL;
    exec = sub = exec1 = param1 = sub1 = label2 = exec2 = param2 =
        sub2 = cbox = NULL;
    move_resize = false;
    
#ifdef XFT
    xftdraw = (Drawable) 0;
#endif // XFT

#ifdef XRENDER
    pixmap = None;
#endif // XRENDER
    
}

/**
 * @fn    ~WaMenuItem(void)
 * @brief Destructor for WaMenuItem class
 *
 * Deletes xftdrawable if compiled with xft support and destroys menu item
 * window.
 */
WaMenuItem::~WaMenuItem(void) {
    delete [] label1;
    if (label2) delete [] label2;
    if (sub1) delete [] sub1;
    if (sub2) delete [] sub2;
    if (exec1) delete [] exec1;
    if (exec2) delete [] exec2;
    if (param1) delete [] param1;
    if (param2) delete [] param2;

    menu->item_list->remove(this);
        
#ifdef XFT
    if (xftdraw) XftDrawDestroy(xftdraw);
#endif // XFT

#ifdef XRENDER    
    if (pixmap != None) XFreePixmap(menu->display, pixmap);
#endif // XRENDER
    
    if (id) {
       XDestroyWindow(menu->display, id);
       menu->waimea->window_table->erase(id);
    }
}

/**
 * @fn    DrawFg(void)
 * @brief Draws menu item foreground
 *
 * Draws menu item label in menu item foreground. 
 */
void WaMenuItem::DrawFg(void) {
    int x, y, justify;

    XClearWindow(menu->display, id);
    
#ifdef XFT
    cbox_xft_font = menu->wascreen->mstyle.cf_xftfont;
#endif // XFT
    
    cbox_gc = &menu->wascreen->mstyle.cf_text_gc;
    
    cb_y = menu->wascreen->mstyle.cf_y_pos;
    cbox = menu->wascreen->mstyle.checkbox_false;
    if (cb) UpdateCBox();
    
#ifdef XFT    
    XGlyphInfo extents;
    if (type == MenuCBItemType && menu->wascreen->mstyle.wa_f_font.xft) {
        XftTextExtents8(menu->display, menu->wascreen->mstyle.f_xftfont,
                        (unsigned char *) label, strlen(label), &extents);
        width = extents.width + 20;
    }
#endif // XFT
    
    if (type == MenuCBItemType && !menu->wascreen->mstyle.wa_f_font.xft) {
        width = XTextWidth(menu->wascreen->mstyle.f_font, label,
                           strlen(label)) + 20;
    }
    
    if (type == MenuTitleType)
        justify = menu->wascreen->mstyle.t_justify;
    else
        justify = menu->wascreen->mstyle.f_justify;
    if (menu->width <= width) justify = LeftJustify;
    
    switch (justify) {
        case LeftJustify: x = 10; break;
        case CenterJustify:
            if (type == MenuTitleType)
                x = (menu->width / 2) - ((width - 10) / 2);
            else if (type == MenuCBItemType)
                x = ((menu->width - menu->cb_width) / 2) - ((width - 10) / 2);
            else x = ((menu->width - menu->extra_width) / 2) -
                     ((width - 10) / 2);
            break;
        default:
            if (type == MenuTitleType)
                x = menu->width - (width - 10);
            else if (type == MenuCBItemType)
                x = (menu->width - menu->cb_width) - (width - 10);
            else x = (menu->width - menu->extra_width) - (width - 10);
    }

    bool draw_i = false, draw_b = false, draw_cb = false;
#ifdef XFT
    XftFont *font;
    XftColor *xftcolor;
    
    if (type == MenuTitleType && menu->wascreen->mstyle.wa_t_font.xft) {
        font = menu->wascreen->mstyle.t_xftfont;
        xftcolor = menu->wascreen->mstyle.t_xftcolor;
        y = menu->wascreen->mstyle.t_y_pos;
        draw_i = true;
    } else if (menu->wascreen->mstyle.wa_f_font.xft) {
        font = menu->wascreen->mstyle.f_xftfont;
        y = menu->wascreen->mstyle.f_y_pos;
        draw_i = true;
        if (hilited) {
            xftcolor = menu->wascreen->mstyle.fh_xftcolor;
        } else
            xftcolor = menu->wascreen->mstyle.f_xftcolor;
    }
    if (draw_i)
        XftDrawString8(xftdraw, xftcolor, font, x, y, (unsigned char *) label,
                       strlen(label));
    
    if (type == MenuSubType && menu->wascreen->mstyle.wa_b_font.xft) {
        draw_b = true;
        y = menu->wascreen->mstyle.b_y_pos;
        XftDrawString8(xftdraw, xftcolor, menu->wascreen->mstyle.b_xftfont,
                       menu->width - (menu->bullet_width + 5), y,
                       (unsigned char *) menu->wascreen->mstyle.bullet,
                       strlen(menu->wascreen->mstyle.bullet));
    }
    else if (type == MenuCBItemType && cbox_xft_font) {
        draw_cb = true;
        XftDrawString8(xftdraw, xftcolor, cbox_xft_font,
                       menu->width - (cb_width + 5), cb_y,
                       (unsigned char *) cbox, strlen(cbox));
    }
#endif // XFT
    
    GC *gc, *bgc = NULL;

    if (! draw_i) {
        if (type == MenuTitleType) {
            gc = &menu->wascreen->mstyle.t_text_gc;
            y = menu->wascreen->mstyle.t_y_pos;
        } else {
            if (hilited) {
                gc = &menu->wascreen->mstyle.fh_text_gc;
                bgc = &menu->wascreen->mstyle.bh_text_gc;
            } else {
                gc = &menu->wascreen->mstyle.f_text_gc;
                bgc = &menu->wascreen->mstyle.b_text_gc;
            }
            y = menu->wascreen->mstyle.f_y_pos;
        }
        XDrawString(menu->display, (Drawable) id, *gc, x, y, label,
                    strlen(label));
    }
    if (type == MenuSubType) {
        if (! draw_b) {
            y = menu->wascreen->mstyle.b_y_pos;
            XDrawString(menu->display, (Drawable) id, *bgc,
                        menu->width - (menu->bullet_width + 5), y,
                        menu->wascreen->mstyle.bullet,
                        strlen(menu->wascreen->mstyle.bullet));
        }
    }
    else if (type == MenuCBItemType) {
        if (! draw_cb) {
            XDrawString(menu->display, (Drawable) id, *cbox_gc,
                        menu->width - (cb_width + 5), cb_y, cbox,
                        strlen(cbox));
        }
    }
}

#ifdef XRENDER
/**
 * @fn    Render(void)
 * @brief Render transparent background
 *
 * Renders and sets transperancy background.
 */
void WaMenuItem::Render(void) {
    if (type != MenuTitleType && ! hilited) {
        XClearWindow(menu->display, id);
        return;
    }
    int bw = menu->wascreen->mstyle.border_width;
    if (! texture->getOpacity()) return;
    if (((menu->x + menu->width) > 0 && menu->x < menu->wascreen->width) && 
        ((menu->y + dy + height) > 0 && (menu->y + dy) <
         menu->wascreen->height)) {
        if (type == MenuTitleType) {
            pixmap = menu->wascreen->ic->xrender(menu->ptitle, menu->width,
                                                 height, texture,
                                                 menu->wascreen->xrootpmap_id,
                                                 menu->x + bw,
                                                 menu->y + dy + bw,
                                                 pixmap);
            XSetWindowBackgroundPixmap(menu->display, id, pixmap);
        }
        else if (hilited) {
            pixmap = menu->wascreen->ic->xrender(menu->philite, menu->width,
                                                 height, texture,
                                                 menu->wascreen->xrootpmap_id,
                                                 menu->x +bw,
                                                 menu->y + dy + bw,
                                                 pixmap);
            XSetWindowBackgroundPixmap(menu->display, id, pixmap);
        }
    }
}
#endif // XRENDER
    
/**
 * @fn    Hilite(void)
 * @brief Hilite menu item
 *
 * Sets menu items window background pixmap to the one representing a hilited
 * menu item and redraws foreground.
 */
void WaMenuItem::Hilite(void) {
    if (type == MenuTitleType) return;
    
    list<WaMenuItem *>::iterator it = menu->item_list->begin();
    for (; it != menu->item_list->end(); ++it) {        
        if ((*it)->hilited && menu->has_focus)
            if (!(((*it)->func_mask & MenuSubMask) && (*it)->submenu->mapped))
                (*it)->DeHilite();
    }
    hilited = true;
    texture = &menu->wascreen->mstyle.hilite;

#ifdef XRENDER
    if (texture->getOpacity()) Render();
    else {
#endif // XRENDER
    
        if (menu->philite)
            XSetWindowBackgroundPixmap(menu->display, id, menu->philite);
        else
            XSetWindowBackground(menu->display, id, menu->hilite_pixel);
        
#ifdef XRENDER
    }
#endif // XRENDER
    
    DrawFg();
}

/**
 * @fn    DeHilite(void)
 * @brief DeHilite menu item
 *
 * Sets menu items window background pixmap to the one representing a none 
 * hilited menu item and redraws foreground.
 */
void WaMenuItem::DeHilite(void) {
    if (type == MenuTitleType) return;
    hilited = false;
    texture = &menu->wascreen->mstyle.back_frame;
    
    XSetWindowBackgroundPixmap(menu->display, id, ParentRelative);
    
    DrawFg();
}

/**
 * @fn    UnmapMenu(XEvent *, WaAction *, bool focus)
 * @brief Unmaps menu
 *
 * Unmaps the menu holding the menu item.
 *
 * @param bool True if we should focus root item
 */
void WaMenuItem::UnmapMenu(XEvent *, WaAction *, bool focus) {
    if (! in_window) return;
    if (menu->waimea->eh->move_resize != EndMoveResizeType) return;
    menu->Unmap(focus);
}

/**
 * @fn    MapSubmenu(XEvent *, WaAction *)
 * @brief Maps Submenu
 *
 * Maps menu items submenu, if there is one, at a good position close to the
 * menu item. If the submenu is already mapped then we do nothing.
 *
 * @param bool True if we should focus first item in submenu
 */
void WaMenuItem::MapSubmenu(XEvent *, WaAction *, bool focus) {
    int skip;

    if (! in_window) return;
    if ((! (func_mask & MenuSubMask)) || submenu->mapped) return;
    if (menu->waimea->eh->move_resize != EndMoveResizeType) return;
    
    Hilite();
    if (submenu->tasksw) menu->waimea->taskswitch->Build(menu->wascreen);
    submenu->root_menu = menu;
    submenu->root_item = this;
    submenu->wf = menu->wf;
    submenu->rf = menu->rf;
    submenu->mf = menu->mf;
    submenu->ftype = menu->ftype;
    list<WaMenuItem *>::iterator it = submenu->item_list->begin();
    for (skip = 0; it != submenu->item_list->end(); ++it) {        
        if ((*it)->type == MenuTitleType)
            skip += (*it)->realheight;
        else break;
    }
    submenu->Map(menu->x + menu->width + menu->wascreen->mstyle.border_width,
                 menu->y + dy - skip);
    if (focus) submenu->FocusFirst();
}

/**
 * @fn    RemapSubmenu(XEvent *, WaAction * )
 * @brief Remaps Submenu
 *
 * Maps menu items submenu, if there is one, at a good position close to the
 * menu item. If the submenu is already mapped then we just move it to
 * the position we want to remap it to.
 *
 * @param bool True if we should focus first item in submenu
 */
void WaMenuItem::RemapSubmenu(XEvent *, WaAction *, bool focus) {
    int skip;

    if (! in_window) return;
    if (! (func_mask & MenuSubMask)) return;
    if (menu->waimea->eh->move_resize != EndMoveResizeType) return;
    
    Hilite();
    if (submenu->tasksw) menu->waimea->taskswitch->Build(menu->wascreen);
    submenu->root_menu = menu;
    submenu->root_item = this;
    submenu->wf = menu->wf;
    submenu->rf = menu->rf;
    submenu->mf = menu->mf;
    submenu->ftype = menu->ftype;
    list<WaMenuItem *>::iterator it = submenu->item_list->begin();
    for (skip = 0; it != submenu->item_list->end(); ++it) {        
        if ((*it)->type == MenuTitleType)
            skip += (*it)->height + menu->wascreen->mstyle.border_width;
        else break;
    }
    submenu->ReMap(menu->x + menu->width + menu->wascreen->mstyle.border_width,
                   menu->y + dy - skip);
    if (focus) submenu->FocusFirst();
} 

/**
 * @fn    UnLinkMenu(XEvent *, WaAction *)
 * @brief Unlink menu
 *
 * Removes link to menu tree from menu. Menu will not be apart of the menu tree
 * mapping the menu any longer after this. Unmapsubmenu and Unmapmenutree
 * functions applied somewhere in the old menu tree will not unmap this menu. 
 */
void WaMenuItem::UnLinkMenu(XEvent *, WaAction *) {
    if (! in_window) return;
    menu->root_menu = NULL;
}

/**
 * @fn    Exec(XEvent *, WaAction *)
 * @brief Execute program
 *
 * This function executes menu items command line, if there is one.
 */
void WaMenuItem::Exec(XEvent *, WaAction *) {
    if (cb) UpdateCBox();
    if (! in_window) return;
    if (! (func_mask & MenuExecMask)) return;

    waexec(exec, menu->wascreen->displaystring);
}

/**
 * @fn    Func(XEvent *e, WaAction *ac)
 * @brief Call function
 *
 * This function calls the function stored in menu items function pointer, if
 * there is a function pointer and the menu has been linked to an object of
 * same type as the function is a member of.
 *
 * @param e Event causing function call
 * @param ac WaAction object
 */
void WaMenuItem::Func(XEvent *e, WaAction *ac) {
    hash_map<Window, WindowObject *>::iterator it;
    Window func_win;
    char *tmp_param;

    if (! in_window) return;
    if (cb) UpdateCBox();
    if (param) {
        tmp_param = ac->param;
        ac->param = param;
    }
    if (wf) func_win = wf;
    else func_win = menu->wf;
    if ((func_mask & MenuWFuncMask) &&
        ((menu->ftype == MenuWFuncMask) || wf)) {
        if ((it = menu->waimea->window_table->find(func_win)) !=
            menu->waimea->window_table->end()) {
            if (((*it).second)->type == WindowType) {
                (*((WaWindow *) (*it).second).*(wfunc))(e, ac);
            }
        }
    }
    else if ((func_mask & MenuRFuncMask) && (menu->ftype == MenuRFuncMask))
        ((*(menu->rf)).*(rfunc))(e, ac);
    else if ((func_mask & MenuMFuncMask) && (menu->ftype == MenuMFuncMask))
        ((*(menu->mf)).*(mfunc))(e, ac);
    
    if (param) ac->param = tmp_param;
}

/**
 * @fn    Lower(XEvent *, WaAction *)
 * @brief Lowers menu window in display stack
 *
 * Lowers the menu frame to the bottom of the display stack.
 */
void WaMenuItem::Lower(XEvent *, WaAction *) {
    if (! in_window) return;
    menu->waimea->WaLowerWindow(menu->frame);
}

/**
 * @fn    Focus(void)
 * @brief Focus menu
 *
 * Sets input focus to the menu item window.
 */
void WaMenuItem::Focus(void) {
    XSetInputFocus(menu->display, id, RevertToPointerRoot, CurrentTime);
    menu->has_focus = true;
    Hilite();
}

/**
 * @fn    Move(XEvent *e, WaAction *)
 * @brief Moves the menu items menu
 *
 * Non-opaque moving of menu.
 *
 * @param e Event causing function call
 */
void WaMenuItem::Move(XEvent *e, WaAction *) {
    XEvent event;
    int px, py, i;
    list<XEvent *> *maprequest_list;
    bool started = false;
    int nx = menu->x;
    int ny = menu->y;
    Window w;
    unsigned int ui;

    if (menu->waimea->eh->move_resize != EndMoveResizeType) return;
    menu->waimea->eh->move_resize = MoveType;
    move_resize = true;
    
    XQueryPointer(menu->display, menu->wascreen->id, &w, &w, &px,
                  &py, &i, &i, &ui);
    
    maprequest_list = new list<XEvent *>;
    XGrabPointer(menu->display, id, true, ButtonReleaseMask |
                 ButtonPressMask | PointerMotionMask | EnterWindowMask |
                 LeaveWindowMask, GrabModeAsync, GrabModeAsync, None,
                 menu->waimea->move_cursor, CurrentTime);
    XGrabKeyboard(menu->display, id, true, GrabModeAsync, GrabModeAsync,
                  CurrentTime);
    for (;;) {
        menu->waimea->eh->EventLoop(
            menu->waimea->eh->menu_viewport_move_return_mask, &event);
        switch (event.type) {
            case MotionNotify:
                if (! started) {
                    menu->ToggleOutline();
                    started = true;
                }
                nx += event.xmotion.x_root - px;
                ny += event.xmotion.y_root - py;
                px  = event.xmotion.x_root;
                py  = event.xmotion.y_root;
                menu->DrawOutline(nx - menu->x, ny - menu->y);
                break;
            case LeaveNotify:
            case EnterNotify:
                if (menu->wascreen->west->id == event.xcrossing.window ||
                    menu->wascreen->east->id == event.xcrossing.window ||
                    menu->wascreen->north->id == event.xcrossing.window ||
                    menu->wascreen->south->id == event.xcrossing.window) {
                    menu->waimea->eh->HandleEvent(&event);
                } else {
                    nx += event.xcrossing.x_root - px;
                    ny += event.xcrossing.y_root - py;
                    px  = event.xcrossing.x_root;
                    py  = event.xcrossing.y_root;
                    menu->DrawOutline(nx - menu->x, ny - menu->y);
                }
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
                menu->waimea->eh->HandleEvent(&event);
                if (menu->waimea->eh->move_resize != EndMoveResizeType) break;
                if (started) menu->ToggleOutline();
                menu->Move(nx - menu->x, ny - menu->y);
                while (! maprequest_list->empty()) {
                    XPutBackEvent(menu->display, maprequest_list->front());
                    maprequest_list->pop_front();
                }
                delete maprequest_list;
                move_resize = false;
                XUngrabKeyboard(menu->display, CurrentTime);
                XUngrabPointer(menu->display, CurrentTime);
                return;
        }
    }
}

/**
 * @fn    MoveOpaque(XEvent *e, WaAction *)
 * @brief Moves the menu items menu
 *
 * Opaque moving of menu.
 *
 * @param e Event causing function call
 */
void WaMenuItem::MoveOpaque(XEvent *e, WaAction *) {
    XEvent event, *map_ev;
    int px, py, i;
    list<XEvent *> *maprequest_list;
    int nx = menu->x;
    int ny = menu->y;
    Window w;
    unsigned int ui;

    if (menu->waimea->eh->move_resize != EndMoveResizeType) return;
    menu->waimea->eh->move_resize = MoveType;
    move_resize = true;
    
    XQueryPointer(menu->display, menu->wascreen->id, &w, &w, &px,
                  &py, &i, &i, &ui);
    
    maprequest_list = new list<XEvent *>;
    XGrabPointer(menu->display, id, true, ButtonReleaseMask |
                 ButtonPressMask | PointerMotionMask | EnterWindowMask |
                 LeaveWindowMask, GrabModeAsync, GrabModeAsync, None,
                 menu->waimea->move_cursor, CurrentTime);
    XGrabKeyboard(menu->display, id, true, GrabModeAsync, GrabModeAsync,
                  CurrentTime);
    for (;;) {
        menu->waimea->eh->EventLoop(
            menu->waimea->eh->menu_viewport_move_return_mask, &event);
        switch (event.type) {
            case MotionNotify:
                nx += event.xmotion.x_root - px;
                ny += event.xmotion.y_root - py;
                px = event.xmotion.x_root;
                py = event.xmotion.y_root;
                menu->Move(nx - menu->x, ny - menu->y);
                break;
            case LeaveNotify:
            case EnterNotify:
                if (menu->wascreen->west->id == event.xcrossing.window ||
                    menu->wascreen->east->id == event.xcrossing.window ||
                    menu->wascreen->north->id == event.xcrossing.window ||
                    menu->wascreen->south->id == event.xcrossing.window) {
                    menu->waimea->eh->HandleEvent(&event);
                } else {
                    nx += event.xcrossing.x_root - px;
                    ny += event.xcrossing.y_root - py;
                    px = event.xcrossing.x_root;
                    py = event.xcrossing.y_root;
                    menu->Move(nx - menu->x, ny - menu->y);
                }
                break;
            case MapRequest:
                map_ev = new XEvent;
                *map_ev = event;
                maprequest_list->push_front(map_ev); break;
            case ButtonPress:
            case ButtonRelease:
                event.xbutton.window = id;
            case KeyPress:
            case KeyRelease:
                if (event.type == KeyPress || event.type == KeyRelease)
                    event.xkey.window = id;
                menu->waimea->eh->HandleEvent(&event);
                if (menu->waimea->eh->move_resize != EndMoveResizeType) break;
                while (! maprequest_list->empty()) {
                    XPutBackEvent(menu->display, maprequest_list->front());
                    delete maprequest_list->front();
                    maprequest_list->pop_front();
                }
                delete maprequest_list;
                move_resize = false;
                XUngrabKeyboard(menu->display, CurrentTime);
                XUngrabPointer(menu->display, CurrentTime);
                return;
        }
    }
}

/**
 * @fn    EndMoveResize(XEvent *e, WaAction *)
 * @brief Ends move
 *
 * Ends menu moving process.
 */
void WaMenuItem::EndMoveResize(XEvent *, WaAction *) {
    menu->waimea->eh->move_resize = EndMoveResizeType;   
}

/**
 * @fn    TaskSwitcher(XEvent *, WaAction *)
 * @brief Maps task switcher menu
 *
 * Maps task switcher menu at middle of screen and sets input focus to
 * first window in list.
 */
void WaMenuItem::TaskSwitcher(XEvent *, WaAction *) {
    if (menu->waimea->eh->move_resize != EndMoveResizeType) return;
    menu->waimea->taskswitch->Build(menu->wascreen);
    menu->waimea->taskswitch->ReMap(menu->wascreen->width / 2 -
                                    menu->waimea->taskswitch->width / 2,
                                    menu->wascreen->height / 2 -
                                    menu->waimea->taskswitch->height / 2);
    menu->waimea->taskswitch->FocusFirst();
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
void WaMenuItem::PreviousTask(XEvent *e, WaAction *ac) {
    if (menu->waimea->eh->move_resize != EndMoveResizeType) return;
    list<WaWindow *>::iterator it = menu->waimea->wawindow_list->begin();
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
void WaMenuItem::NextTask(XEvent *e, WaAction *ac) {
    if (menu->waimea->eh->move_resize != EndMoveResizeType) return;
    menu->waimea->wawindow_list->back()->Raise(e, ac);
    menu->waimea->wawindow_list->back()->FocusVis(e, ac);
}

/**
 * @fn    NextItem(XEvent *e, WaAction *ac)
 * @brief Hilite and focus next item
 *
 * Hilites and sets input focus to menu item beneath this menu item.
 *
 * @param e X event that have occurred
 * @param ed Event details
 */
void WaMenuItem::NextItem(XEvent *e, WaAction *ac) {
    list<WaMenuItem *>::iterator it = menu->item_list->begin();
    for (; it != menu->item_list->end(); ++it) {
        if (*it == this) {
            for (++it; it != menu->item_list->end() &&
                     (*it)->type == MenuTitleType; ++it);
            if (it == menu->item_list->end()) {
                it = menu->item_list->begin();
                for (; *it != this && (*it)->type == MenuTitleType; ++it);
                if (*it != this) {
                    (*it)->Focus();
                    return;
                }
            } else {
                (*it)->Focus();
                return;
            }
        }
    }
}

/**
 * @fn    PreviousItem(XEvent *e, WaAction *ac)
 * @brief Hilite and focus previous item
 *
 * Hilites and sets input focus to menu item above this menu item.
 *
 * @param e X event that have occurred
 * @param ed Event details
 */
void WaMenuItem::PreviousItem(XEvent *e, WaAction *ac) {
    list<WaMenuItem *>::reverse_iterator it = menu->item_list->rbegin();
    for (; it != menu->item_list->rend(); ++it) {
        if (*it == this) {
            for (++it; it != menu->item_list->rend() &&
                     (*it)->type == MenuTitleType; ++it);
            if (it == menu->item_list->rend()) {
                it = menu->item_list->rbegin();
                for (; *it != this && (*it)->type == MenuTitleType; ++it);
                if (*it != this) {
                    (*it)->Focus();
                    return;
                }
            } else {
                (*it)->Focus();
                return;
            }
        }
    }
}

/**
 * @fn    EvAct(XEvent *e, EventDetail *ed, list<WaAction *> *acts)
 * @brief Calls menu item function
 *
 * Tries to match an occurred X event with the actions in an action list.
 * If we have a match then we execute that action.
 *
 * @param e X event that have occurred
 * @param ed Event details
 * @param acts List with actions to match event with
 */
void WaMenuItem::EvAct(XEvent *e, EventDetail *ed, list<WaAction *> *acts) {
    Window w;
    unsigned int ui;
    int xp, yp, i;
    in_window = true;

    if (e->type == ButtonPress || e->type == ButtonRelease ||
        e->type == DoubleClick) {
        XQueryPointer(menu->display, id, &w, &w, &i, &i, &xp, &yp, &ui);
        if (xp < 0 || yp < 0 || xp > menu->width || yp > height)
            in_window = false;
    }
   
    if (menu->waimea->eh->move_resize != EndMoveResizeType)
        ed->mod |= MoveResizeMask;

    list<WaAction *>::iterator it = acts->begin();
    for (; it != acts->end(); ++it) {
        if (eventmatch(*it, ed)) {
            if ((*it)->delay.tv_sec || (*it)->delay.tv_usec) {
                Interrupt *i = new Interrupt(*it, e);
                i->wm = this;
                menu->waimea->timer->AddInterrupt(i);
            } else {
                if ((*it)->exec)
                    waexec((*it)->exec, menu->wascreen->displaystring);
                else 
                    ((*this).*((*it)->menufunc))(e, *it);
            }
        }
    }
    if (ed->type == EnterNotify) {
        Hilite();
        if (menu->has_focus && type != MenuTitleType) Focus();
    }
    else if (ed->type == LeaveNotify) {
        if (func_mask & MenuSubMask) {
            if (! submenu->mapped) 
                DeHilite();
        } else DeHilite();
    }
}

/**
 * @fn    UpdateCBox(void)
 * @brief Update Checkbox
 *
 * Reads checkbox value and updates pointers so that checkbox is drawn and
 * handled correct.
 */
void WaMenuItem::UpdateCBox(void) {
    hash_map<Window, WindowObject *>::iterator it;
    Window func_win;
    bool true_false = false;
    WaWindow *ww;

    if (cb) {
        if (wf) func_win = wf;
        else func_win = menu->wf;
        if ((func_mask & MenuWFuncMask) &&
            ((menu->ftype == MenuWFuncMask) || wf)) {
            if ((it = menu->waimea->window_table->find(func_win)) !=
                menu->waimea->window_table->end()) {
                if (((*it).second)->type == WindowType) {
                    ww = (WaWindow *) (*it).second;
                    switch (cb) {
                        case MaxCBoxType:
                            true_false = ww->flags.max; break;
                        case ShadeCBoxType:
                            true_false = ww->flags.shaded; break;
                        case StickCBoxType:
                            true_false = ww->flags.sticky; break;
                        case TitleCBoxType:
                            true_false = ww->flags.title; break;
                        case HandleCBoxType:
                            true_false = ww->flags.handle; break;
                        case BorderCBoxType:
                            true_false = ww->flags.border; break;
                        case AllCBoxType:
                            true_false = ww->flags.all; break;
                        case AOTCBoxType:
                            true_false = ww->flags.alwaysontop; break;
                        case AABCBoxType:
                            true_false = ww->flags.alwaysatbottom;
                    }
                    if (true_false) {
                        
#ifdef XFT
                        if (menu->wascreen->mstyle.wa_ct_font.xft)
                            cbox_xft_font = menu->wascreen->mstyle.ct_xftfont;
                        else
                            cbox_xft_font = NULL;
#endif // XFT
                        
                        if (! menu->wascreen->mstyle.wa_ct_font.xft) {
                            if (hilited)
                                cbox_gc = &menu->wascreen->mstyle.cth_text_gc;
                            else
                                cbox_gc = &menu->wascreen->mstyle.ct_text_gc;
                        }
                        cb_y = menu->wascreen->mstyle.ct_y_pos;
                        cbox = menu->wascreen->mstyle.checkbox_true;
                        label = label2;
                        sub = sub2;
                        wfunc = wfunc2;
                        rfunc = rfunc2;
                        mfunc = mfunc2;
                        func_mask = func_mask2;
                        cb_width = cb_width2;
                        param = param2;
                    }
                    else {
                        
#ifdef XFT
                        if (menu->wascreen->mstyle.wa_cf_font.xft)
                            cbox_xft_font = menu->wascreen->mstyle.cf_xftfont;
                        else
                            cbox_xft_font = NULL;
#endif // XFT
                        
                        if (! menu->wascreen->mstyle.wa_cf_font.xft) {
                            if (hilited)
                                cbox_gc = &menu->wascreen->mstyle.cfh_text_gc;
                            else
                                cbox_gc = &menu->wascreen->mstyle.cf_text_gc;
                        }
                        cb_y = menu->wascreen->mstyle.cf_y_pos;
                        cbox = menu->wascreen->mstyle.checkbox_false;
                        label = label1;
                        sub = sub1;
                        wfunc = wfunc1;
                        rfunc = rfunc1;
                        mfunc = mfunc1;
                        func_mask = func_mask1;
                        cb_width = cb_width1;
                        param = param1;
                    }
                }
            }
        }
    }
}


/** 
 * @fn    TaskSwitcher(void) : WaMenu(wastrdup("__windowlist__"))
 * @brief Constructor for TaskSwitcher class
 *
 * Creates a TaskSwitcher object, used for fast switching between windows.
 * This is a WaMenu with some extra functionality.
 * 
 */
TaskSwitcher::TaskSwitcher(void) : WaMenu(wastrdup("__windowlist__")) {
    WaMenuItem *m;
    
    tasksw = true;
    m = new WaMenuItem(wastrdup("Window List"));
    m->type = MenuTitleType;
    AddItem(m);
}

/** 
 * @fn    Build(WaScreen *wascrn) 
 * @brief Builds Task Switcher menu
 *
 * Overloaded Build function to make Task Switcher build a menu from
 * current windows.
 *
 * @param wascrn WaScreen to map menu on.
 */
void TaskSwitcher::Build(WaScreen *wascrn) {
    WaWindow *ww;
    WaMenuItem *m;
    wawindow_list = wascrn->waimea->wawindow_list;
    
    LISTCLEAR2(item_list);
    item_list = new list<WaMenuItem *>;

    m = new WaMenuItem(wastrdup("Window List"));
    m->type = MenuTitleType;
    AddItem(m);

    list<WaWindow *>::iterator it = wawindow_list->begin();
    for (++it; it != wawindow_list->end(); ++it) {
        ww = (WaWindow *) *it;
        m = new WaMenuItem(wastrdup(ww->name));
        m->type = MenuItemType;
        m->wfunc = &WaWindow::RaiseFocus;
        m->func_mask |= MenuWFuncMask;
        m->func_mask1 |= MenuWFuncMask;
        m->wf = ww->id;
        AddItem(m);
    }
    if (! wawindow_list->empty()) {
        ww = (WaWindow *) wawindow_list->front();
        m = new WaMenuItem(wastrdup(ww->name));
        m->type = MenuItemType;
        m->wfunc = &WaWindow::RaiseFocus;
        m->func_mask |= MenuWFuncMask;
        m->func_mask1 |= MenuWFuncMask;
        m->wf = ww->id;
        AddItem(m);
    }
    
    WaMenu::Build(wascrn);
}


/**
 * Wrapper functions.
 */
void WaMenuItem::ViewportMove(XEvent *e, WaAction *wa) {
    menu->wascreen->ViewportMove(e, wa);
}
void WaMenuItem::ViewportRelativeMove(XEvent *e, WaAction *wa) {
       menu->wascreen->ViewportRelativeMove(e, wa);
}
void WaMenuItem::ViewportFixedMove(XEvent *e, WaAction *wa) {
       menu->wascreen->ViewportFixedMove(e, wa);
}
void WaMenuItem::MoveViewportLeft(XEvent *, WaAction *) {
    menu->wascreen->MoveViewport(WestDirection);
}
void WaMenuItem::MoveViewportRight(XEvent *, WaAction *) {
    menu->wascreen->MoveViewport(EastDirection);
}
void WaMenuItem::MoveViewportUp(XEvent *, WaAction *) {
    menu->wascreen->MoveViewport(NorthDirection);
}
void WaMenuItem::MoveViewportDown(XEvent *, WaAction *) {
    menu->wascreen->MoveViewport(SouthDirection);
}
void WaMenuItem::PointerRelativeWarp(XEvent *e, WaAction *ac) {
    menu->wascreen->PointerRelativeWarp(e, ac);
}
void WaMenuItem::PointerFixedWarp(XEvent *e, WaAction *ac) {
       menu->wascreen->PointerFixedWarp(e, ac);
}
void WaMenuItem::MenuMap(XEvent *e, WaAction *ac) {
    menu->wascreen->MenuMap(e, ac, false);
}
void WaMenuItem::MenuMapFocused(XEvent *e, WaAction *ac) {
    menu->wascreen->MenuMap(e, ac, true);
}
void WaMenuItem::MenuRemap(XEvent *e, WaAction *ac) {
    menu->wascreen->MenuRemap(e, ac, false);
}
void WaMenuItem::MenuRemapFocused(XEvent *e, WaAction *ac) {
    menu->wascreen->MenuRemap(e, ac, true);
}
void WaMenuItem::MenuUnmap(XEvent *e, WaAction *wa) {
    menu->wascreen->MenuUnmap(e, wa, false);
}
void WaMenuItem::MenuUnmapFocus(XEvent *e, WaAction *wa) {
    menu->wascreen->MenuUnmap(e, wa, true);
}
void WaMenuItem::Restart(XEvent *e, WaAction *ac) {
    menu->wascreen->Restart(e, ac);
}
void WaMenuItem::Exit(XEvent *e, WaAction *ac) {
    menu->wascreen->Exit(e, ac);
}
