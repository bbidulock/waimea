/** -*- Mode: C++ -*-
 *
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
    mapped = has_focus = built = tasksw = False;
    root_menu = NULL;
    root_item = NULL;
    wf = (Window) 0;
    rf = NULL;
    mf = NULL;
    
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
    item->hilited = False;
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
    
    int lasttype = 0;
    it = item_list->begin();
    for (i = 1; it != item_list->end(); ++it, ++i) {
#ifdef XFT
        XGlyphInfo extents;
        XftFont *font;
        
        if ((*it)->type == MenuTitleType)
            font = wascreen->mstyle.t_xftfont;
        else
            font = wascreen->mstyle.f_xftfont;
        
        XftTextExtents8(display, font, (unsigned char *) (*it)->label,
                        strlen((*it)->label), &extents);
        (*it)->width = extents.width + 20;
        if ((*it)->type == MenuSubType) {
            XftTextExtents8(display, wascreen->mstyle.b_xftfont,
                            (unsigned char *) wascreen->mstyle.bullet,
                            strlen(wascreen->mstyle.bullet), &extents);
            bullet_width = extents.width;
        }
        else if ((*it)->type == MenuCBItemType) {
            XftTextExtents8(display, wascreen->mstyle.ct_xftfont,
                            (unsigned char *) wascreen->mstyle.checkbox_true,
                            strlen(wascreen->mstyle.checkbox_true), &extents);
            (*it)->cb_width2 = cb_width = extents.width;
            
            XftTextExtents8(display, wascreen->mstyle.cf_xftfont,
                            (unsigned char *) wascreen->mstyle.checkbox_false,
                            strlen(wascreen->mstyle.checkbox_false), &extents);
            if (extents.width > cb_width) cb_width = extents.width;
            (*it)->cb_width1 = extents.width;
            
            XftTextExtents8(display, font, (unsigned char *) (*it)->label2,
                            strlen((*it)->label2), &extents);
            if ((extents.width + 20) > (*it)->width)
                (*it)->width = extents.width + 20;
        }
#else // ! XFT
        XFontStruct *font;
        int tmp_w;
        if ((*it)->type == MenuTitleType)
            font = wascreen->mstyle.t_font;
        else
            font = wascreen->mstyle.f_font;

        (*it)->width = XTextWidth(font, (*it)->label, strlen((*it)->label)) + 20;
        
        if ((*it)->type == MenuSubType)
            bullet_width = XTextWidth(wascreen->mstyle.b_font,
                                      wascreen->mstyle.bullet,
                                      strlen(wascreen->mstyle.bullet));
        else if ((*it)->type == MenuCBItemType) {
            (*it)->cb_width2 = cb_width = XTextWidth(wascreen->mstyle.ct_font,
                                  wascreen->mstyle.checkbox_true,
                                  strlen(wascreen->mstyle.checkbox_true));
            tmp_w = XTextWidth(wascreen->mstyle.cf_font,
                               wascreen->mstyle.checkbox_false,
                               strlen(wascreen->mstyle.checkbox_false));
            if (tmp_w > cb_width) cb_width = tmp_w;
            (*it)->cb_width1 = tmp_w;
            tmp_w = XTextWidth(font, (*it)->label2, strlen((*it)->label2)) + 20;
            if (tmp_w > (*it)->width) (*it)->width = tmp_w;
        }
#endif // XFT
        extra_width = (bullet_width >= cb_width) ? bullet_width: cb_width;
        
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
    
    texture = &wascreen->mstyle.frame;
    pframe  = ic->renderImage(width, f_height, texture);

    attrib_set.background_pixel = None;
    attrib_set.border_pixel = wascreen->mstyle.border_color.getPixel();
    attrib_set.colormap = wascreen->colormap;
    attrib_set.override_redirect = True;
    attrib_set.event_mask =  ButtonPressMask | ButtonReleaseMask |
        EnterWindowMask | LeaveWindowMask | PointerMotionMask | 
        ExposureMask | KeyPressMask | KeyReleaseMask | FocusChangeMask;
    
    if (! built) {
        frame = XCreateWindow(display, wascreen->id, 0, 0, width, height,
                              wascreen->mstyle.border_width,
                              wascreen->screen_number,
                              CopyFromParent, wascreen->visual,
                              CWOverrideRedirect | CWBackPixel | CWEventMask |
                              CWColormap | CWBorderPixel, &attrib_set);
    } else XResizeWindow(display, frame, width, height);
    
    if (pbackframe)
        XSetWindowBackgroundPixmap(display, frame, pbackframe);
    else
        XSetWindowBackground(display, frame, backframe_pixel);
    XClearWindow(display, frame);

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
        if ((*it)->type == MenuTitleType) {
            if (ptitle)
                XSetWindowBackgroundPixmap(display, (*it)->id,
                                           ptitle);
            else
                XSetWindowBackground(display, (*it)->id,
                                     title_pixel);
        } else
            XSetWindowBackgroundPixmap(display, (*it)->id, pframe);
        
        lasttype = (*it)->type;
    }
    built = True;
}

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
    mapped = True;
    has_focus = False;
    XMoveWindow(display, frame, x, y);
    XMapSubwindows(display, frame);
    XMapWindow(display, frame);
    waimea->WaRaiseWindow(frame);
    list<WaMenuItem *>::iterator it = item_list->begin();
    for (; it != item_list->end(); ++it)
        (*it)->DrawFg();
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
    mapped = True;
    has_focus = False;
    XMoveWindow(display, frame, x, y);
    XMapSubwindows(display, frame);
    XMapWindow(display, frame);
    waimea->WaRaiseWindow(frame);
    list<WaMenuItem *>::iterator it = item_list->begin();
    for (; it != item_list->end(); ++it)
        (*it)->DrawFg();
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
        XSync(display, False);
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
            waimea->wawindow_list->front()->Focus(False);
    }
    root_item = NULL;
    mapped = False;
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
    UnmapSubmenus(False);
    Unmap(False);
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
    attrib_set.override_redirect = True;
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
    o_mapped = False;
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
    while (XCheckTypedEvent(display, EnterNotify, &e));
    
    list<WaMenuItem *>::iterator it = item_list->begin();
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
    
#ifdef XFT
    xftdraw = (Drawable) 0;
#endif // XFT
    
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
    XftDrawDestroy(xftdraw);
#endif // XFT
    
    XDestroyWindow(menu->display, id);
    menu->waimea->window_table->erase(id);
}

/**
 * @fn    DrawFg(void)
 * @brief Draws menu item foreground
 *
 * Draws menu item label in menu item foreground. 
 */
void WaMenuItem::DrawFg(void) {
    int x, y, justify;

#ifdef XFT
    cbox_xft_font = menu->wascreen->mstyle.cf_xftfont;
#else // ! XFT
    cbox_gc = &menu->wascreen->mstyle.cf_text_gc;
#endif // XFT
    
    cb_y = menu->wascreen->mstyle.cf_y_pos;
    cbox = menu->wascreen->mstyle.checkbox_false;
    if (cb) UpdateCBox();
    
#ifdef XFT    
    XGlyphInfo extents;
    if (type == MenuCBItemType) {
        XftTextExtents8(menu->display, menu->wascreen->mstyle.f_xftfont,
                        (unsigned char *) label, strlen(label), &extents);
        width = extents.width + 20;
    }
#else // ! XFT
    if (type == MenuCBItemType) {
        width = XTextWidth( menu->wascreen->mstyle.f_font, label,
                            strlen(label)) + 20;
    }
#endif // XFT
    
    if (type == MenuTitleType)
        justify = menu->wascreen->mstyle.t_justify;
    else
        justify = menu->wascreen->mstyle.f_justify;
    if (menu->width <= width) justify = LeftJustify;
    
    switch (justify) {
        case LeftJustify: x = 10; break;
        case CenterJustify:
            if (type == MenuTitleType)
                x = ((menu->width - menu->bullet_width) / 2) - ((width - 20) / 2);
            else if (type == MenuCBItemType)
                x = ((menu->width - menu->cb_width) / 2) - ((width - 20) / 2);
            else x = ((menu->width - menu->extra_width) / 2) - ((width - 20) / 2);
            break;
        default:
            if (type == MenuTitleType)
                x = (menu->width - menu->bullet_width) - (width - 10);
            else if (type == MenuCBItemType)
                x = (menu->width - menu->cb_width) - (width - 10);
            else x = (menu->width - menu->extra_width) - (width - 10);
    }

#ifdef XFT
    XftFont *font;
    XftColor *xftcolor;
    
    if (type == MenuTitleType) {
            font = menu->wascreen->mstyle.t_xftfont;
            xftcolor = &menu->wascreen->mstyle.t_xftcolor;
            y = menu->wascreen->mstyle.t_y_pos;
    } else {
        font = menu->wascreen->mstyle.f_xftfont;
        if (hilited) xftcolor = &menu->wascreen->mstyle.fh_xftcolor;
        else
            xftcolor = &menu->wascreen->mstyle.f_xftcolor;
        y = menu->wascreen->mstyle.f_y_pos;
    }
    XClearWindow(menu->display, id);
    XftDrawString8(xftdraw, xftcolor, font, x, y, (unsigned char *) label,
                   strlen(label));
    if (type == MenuSubType) {
        y = menu->wascreen->mstyle.b_y_pos;
        XftDrawString8(xftdraw, xftcolor, menu->wascreen->mstyle.b_xftfont,
                       menu->width - (menu->bullet_width + 5), y,
                       (unsigned char *) menu->wascreen->mstyle.bullet,
                       strlen(menu->wascreen->mstyle.bullet));
    }
    else if (type == MenuCBItemType) {
        XftDrawString8(xftdraw, xftcolor, cbox_xft_font,
                       menu->width - (cb_width + 5), cb_y,
                       (unsigned char *) cbox, strlen(cbox));
    }
#else // ! XFT
    GC *gc, *bgc = NULL;
    
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
    XClearWindow(menu->display, id);
    XDrawString(menu->display, (Drawable) id, *gc, x, y, label,
                strlen(label));
    if (type == MenuSubType) {
        y = menu->wascreen->mstyle.b_y_pos;
        XDrawString(menu->display, (Drawable) id, *bgc,
                    menu->width - (menu->bullet_width + 5), y,
                    menu->wascreen->mstyle.bullet,
                    strlen(menu->wascreen->mstyle.bullet));
    }
    else if (type == MenuCBItemType) {
        XDrawString(menu->display, (Drawable) id, *cbox_gc,
                    menu->width - (cb_width + 5), cb_y, cbox,
                    strlen(cbox));
    }
#endif // XFT
    
}

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
    hilited = True;
    if (menu->philite)
        XSetWindowBackgroundPixmap(menu->display, id, menu->philite);
    else
        XSetWindowBackground(menu->display, id, menu->hilite_pixel);
    
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
    hilited = False;
    XSetWindowBackgroundPixmap(menu->display, id, menu->pframe);
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
    
    if ((! (func_mask & MenuSubMask)) || submenu->mapped) return;
    
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

    if (! (func_mask & MenuSubMask)) return;
    
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
    menu->has_focus = True;
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
    bool started = False;
    int nx = menu->x;
    int ny = menu->y;
    Window w;
    unsigned int ui;
    
    XQueryPointer(menu->display, menu->wascreen->id, &w, &w, &px,
                  &py, &i, &i, &ui);
    
    if (e->type != ButtonPress) {
            nx = px;
            ny = py;
            menu->DrawOutline(nx - menu->x, ny - menu->y);
            menu->ToggleOutline();
            started = True;
    }
    maprequest_list = new list<XEvent *>;
    XGrabPointer(menu->display, id, True, ButtonReleaseMask |
                 ButtonPressMask | PointerMotionMask | EnterWindowMask |
                 LeaveWindowMask, GrabModeAsync, GrabModeAsync, None,
                 menu->waimea->move_cursor, CurrentTime);
    for (;;) {
        menu->waimea->eh->EventLoop(
            menu->waimea->eh->menu_viewport_move_return_mask, &event);
        switch (event.type) {
            case MotionNotify:
                if (! started) {
                    menu->ToggleOutline();
                    started = True;
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
                    menu->waimea->eh->ed.type = event.type;
                    menu->waimea->eh->ed.mod = event.xcrossing.state;
                    menu->waimea->eh->ed.detail = 0;
                    menu->waimea->eh->EvAct(&event, event.xcrossing.window);
                }
                break;
            case MapRequest:
                maprequest_list->push_front(&event); break;
            case ButtonPress:
            case ButtonRelease:
                if (e->type == event.type) break;
                if (started) menu->ToggleOutline();
                menu->Move(nx - menu->x, ny - menu->y);
                XUngrabPointer(menu->display, CurrentTime);
                while (! maprequest_list->empty()) {
                    XPutBackEvent(menu->display, maprequest_list->front());
                    maprequest_list->pop_front();
                }
                delete maprequest_list;
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
    
    XQueryPointer(menu->display, menu->wascreen->id, &w, &w, &px,
                  &py, &i, &i, &ui);
    
    if (e->type != ButtonPress) {
            nx = px;
            ny = py;
            menu->Move(nx - menu->x, ny - menu->y);
    }
    maprequest_list = new list<XEvent *>;
    XGrabPointer(menu->display, id, True, ButtonReleaseMask |
                 ButtonPressMask | PointerMotionMask | EnterWindowMask |
                 LeaveWindowMask, GrabModeAsync, GrabModeAsync, None,
                 menu->waimea->move_cursor, CurrentTime);
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
                    menu->waimea->eh->ed.type = event.type;
                    menu->waimea->eh->ed.mod = event.xcrossing.state;
                    menu->waimea->eh->ed.detail = 0;
                    menu->waimea->eh->EvAct(&event, event.xcrossing.window);
                }
                break;
            case MapRequest:
                map_ev = new XEvent;
                *map_ev = event;
                maprequest_list->push_front(map_ev); break;
            case ButtonPress:
            case ButtonRelease:
                if (e->type == event.type) break;
                XUngrabPointer(menu->display, CurrentTime);
                while (! maprequest_list->empty()) {
                    XPutBackEvent(menu->display, maprequest_list->front());
                    delete maprequest_list->front();
                    maprequest_list->pop_front();
                }
                delete maprequest_list;
                return;
        }
    }
}

/**
 * @fn    TaskSwitcher(XEvent *, WaAction *)
 * @brief Maps task switcher menu
 *
 * Maps task switcher menu at middle of screen and sets input focus to
 * first window in list.
 */
void WaMenuItem::TaskSwitcher(XEvent *, WaAction *) {
    menu->waimea->taskswitch->Build(menu->wascreen);
    menu->waimea->taskswitch->Map(menu->wascreen->width / 2 -
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
                    (*it)->Hilite();
                    (*it)->Focus(e, ac);
                    return;
                }
            } else {
                (*it)->Hilite();
                (*it)->Focus(e, ac);
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
                    (*it)->Hilite();
                    (*it)->Focus(e, ac);
                    return;
                }
            } else {
                (*it)->Hilite();
                (*it)->Focus(e, ac);
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
    list<WaAction *>::iterator it = acts->begin();
    for (; it != acts->end(); ++it) {
        if (eventmatch(*it, ed)) {
            if ((*it)->exec)
                waexec((*it)->exec, menu->wascreen->displaystring);
            else
                ((*this).*((*it)->menufunc))(e, *it);
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
    bool true_false = False;
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
                        cbox_xft_font = menu->wascreen->mstyle.ct_xftfont;
#else // ! XFT
                        if (hilited) cbox_gc = &menu->wascreen->mstyle.cth_text_gc;
                        else cbox_gc = &menu->wascreen->mstyle.ct_text_gc;
#endif // XFT
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
                        cbox_xft_font = menu->wascreen->mstyle.cf_xftfont;
#else // ! XFT
                        if (hilited) cbox_gc = &menu->wascreen->mstyle.cfh_text_gc;
                        else cbox_gc = &menu->wascreen->mstyle.cf_text_gc;
#endif // XFT
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
    
    tasksw = True;
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
 * Viewport functions.
 */
void WaMenuItem::ViewportMove(XEvent *e, WaAction *wa) {
    menu->wascreen->ViewportMove(e, wa);
}
void WaMenuItem::MoveViewportLeft(XEvent *, WaAction *) {
    menu->wascreen->MoveViewport(WestDirection, True);
}
void WaMenuItem::MoveViewportRight(XEvent *, WaAction *) {
    menu->wascreen->MoveViewport(EastDirection, True);
}
void WaMenuItem::MoveViewportUp(XEvent *, WaAction *) {
    menu->wascreen->MoveViewport(NorthDirection, True);
}
void WaMenuItem::MoveViewportDown(XEvent *, WaAction *) {
    menu->wascreen->MoveViewport(SouthDirection, True);
}
void WaMenuItem::ScrollViewportLeft(XEvent *, WaAction *ac) {
    menu->wascreen->ScrollViewport(WestDirection, True, ac);
}
void WaMenuItem::ScrollViewportRight(XEvent *, WaAction *ac) {
    menu->wascreen->ScrollViewport(EastDirection, True, ac);
}
void WaMenuItem::ScrollViewportUp(XEvent *, WaAction *ac) {
    menu->wascreen->ScrollViewport(NorthDirection, True, ac);
}
void WaMenuItem::ScrollViewportDown(XEvent *, WaAction *ac) {
    menu->wascreen->ScrollViewport(SouthDirection, True, ac);
}
void WaMenuItem::MoveViewportLeftNoWarp(XEvent *, WaAction *) {
    menu->wascreen->MoveViewport(WestDirection, False);
}
void WaMenuItem::MoveViewportRightNoWarp(XEvent *, WaAction *) {
    menu->wascreen->MoveViewport(EastDirection, False);
}
void WaMenuItem::MoveViewportUpNoWarp(XEvent *, WaAction *) {
    menu->wascreen->MoveViewport(NorthDirection, False);
}
void WaMenuItem::MoveViewportDownNoWarp(XEvent *, WaAction *) {
    menu->wascreen->MoveViewport(SouthDirection, False);
}
void WaMenuItem::ScrollViewportLeftNoWarp(XEvent *, WaAction *ac) {
    menu->wascreen->ScrollViewport(WestDirection, False, ac);
}
void WaMenuItem::ScrollViewportRightNoWarp(XEvent *, WaAction *ac) {
    menu->wascreen->ScrollViewport(EastDirection, False, ac);
}
void WaMenuItem::ScrollViewportUpNoWarp(XEvent *, WaAction *ac) {
    menu->wascreen->ScrollViewport(NorthDirection, False, ac);
}
void WaMenuItem::ScrollViewportDownNoWarp(XEvent *, WaAction *ac) {
    menu->wascreen->ScrollViewport(SouthDirection, False, ac);
}
