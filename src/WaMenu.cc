/** -*- Mode: C++ -*-
 *
 * @file   WaMenu.cc
 * @author David Reveman <c99drn@cs.umu.se>
 * @date   02-Aug-2001 22:40:20
 *
 * @brief Implementation of WaMenu and WaMenuItem classes  
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
    width  = 0;
    mapped = False;
    root_menu = NULL;
    root_item = NULL;
    wf = (Window) 0;
    rf = NULL;
    mf = NULL;
    built = False;
    
    item_list = new list<WaMenuItem *>;
}

/**
 * @fn    ~WaMenu(void)
 * @brief Destructor for WaMenu class
 *
 * Deletes all menu items in the menu and removes the frame.
 */
WaMenu::~WaMenu(void) {
    LISTCLEAR(item_list);
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
    free(name);
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
 * @fn    RemoveItem(WaMenuItem *item)
 * @brief Removes item from menu
 *
 * Removes item from menu by removing a WaMenuItem object from the menus item 
 * list.
 * 
 * @param item Item to remove
 */
void WaMenu::RemoveItem(WaMenuItem *item) {
    item_list->remove(item);
    delete item;
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
    
    if (! built) {
        wascreen = screen;
        waimea = wascreen->waimea;
        display = wascreen->display;
        ic = wascreen->ic;
    }
    bullet_width = 0;

    CreateOutlineWindows();

    f_height = wascreen->mstyle.item_height;
    t_height = wascreen->mstyle.title_height;
    
    list<WaMenuItem *>::iterator it = item_list->begin();
    for (; it != item_list->end(); ++it) {
        if ((*it)->func_mask & MenuSubMask) {
            list<WaMenu *>::iterator menu_it = waimea->wamenu_list->begin();
            for (; menu_it != waimea->wamenu_list->end(); ++menu_it) {
                if (! strcmp((*menu_it)->name, (*it)->sub)) {
                    (*it)->submenu = *menu_it;
                    break;
                }
            }
            if (menu_it == waimea->wamenu_list->end()) {
                WARNING << "no menu named \"" << (*it)->sub << "\"" << endl;
                RemoveItem(*it);
                it = item_list->begin();
            }
        }
    }
    int b, lasttype = 0;
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
        (*it)->width = (extents.width + 20);
        if ((*it)->type == MenuSubType) {
            XftTextExtents8(display, wascreen->mstyle.b_xftfont,
                            (unsigned char *) wascreen->mstyle.bullet,
                            strlen(wascreen->mstyle.bullet), &extents);
            bullet_width = extents.width;
        }
#else // ! XFT
	XFontStruct *font;
        if ((*it)->type == MenuTitleType)
            font = wascreen->mstyle.t_font;
        else
            font = wascreen->mstyle.f_font;

        (*it)->width = XTextWidth(font, (*it)->label, strlen((*it)->label)) +
            20;
        if ((*it)->type == MenuSubType)
            bullet_width = XTextWidth(wascreen->mstyle.b_font,
                                      wascreen->mstyle.bullet,
                                      strlen(wascreen->mstyle.bullet));
#endif // XFT
        b = ((*it)->type == MenuSubType)? bullet_width: 0;
        if (((*it)->width + b) > width) width = (*it)->width + b;
        
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
   
    if (! built) {
        attrib_set.background_pixel = None;
        attrib_set.border_pixel = wascreen->mstyle.border_color.getPixel();
        attrib_set.colormap = wascreen->colormap;
        attrib_set.override_redirect = True;
        attrib_set.event_mask =  ButtonPressMask | ButtonReleaseMask |
            EnterWindowMask | LeaveWindowMask | PointerMotionMask | 
            ExposureMask;

        frame = XCreateWindow(display, wascreen->id, 0, 0, width, height,
                              wascreen->wstyle.border_width,
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
            bw = wascreen->wstyle.border_width;
            x = -bw;
            if ((y == 0) || (lasttype == MenuTitleType))
                y -= bw;
        }
        if (! built) {
            (*it)->id = XCreateWindow(display, frame, x, y, width,
                                      (*it)->height, bw,
                                      wascreen->screen_number, CopyFromParent,
                                      wascreen->visual, CWOverrideRedirect | 
                                      CWBackPixel | CWEventMask | CWColormap,
                                      &attrib_set);
            waimea->window_table->insert(make_pair((*it)->id, (*it)));
        }
        else XMoveResizeWindow(display, (*it)->id, x, y, (*it)->width,
                               (*it)->height);
        (*it)->dy = y;
        y += (*it)->height + bw * 2;
#ifdef XFT
        if (! built) 
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
    if (mapped) return;
    x = mx;
    y = my;
    mapped = True;
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
    if (mapped) Move(mx - x, my - y);
    x = mx;
    y = my;
    mapped = True;
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
 * @fn    Unmap(void)
 * @brief Unmaps menu
 *
 * Unmaps menu and dehilites root menu item if menu was mapped as a submenu.
 */
void WaMenu::Unmap(void) {
    if (root_item)
        root_item->DeHilite();
    XUnmapWindow(display, frame);
    XUnmapSubwindows(display, frame);
    mapped = False;
}

/**
 * @fn    UnmapSubmenus(void)
 * @brief Unmaps submenus
 *
 * Recursive function for unmapping submenu trees. Unmaps all submenus in the 
 * current subtree that are still linked.
 */
void WaMenu::UnmapSubmenus(void) {
    list<WaMenuItem *>::iterator it = item_list->begin();
    for (; it != item_list->end(); ++it) {
        if (((*it)->func_mask & MenuSubMask) && (*it)->submenu->root_menu &&
            (*it)->submenu->mapped) {
            (*it)->submenu->UnmapSubmenus();
            (*it)->submenu->Unmap();
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
    UnmapSubmenus();
    Unmap();
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
    attrib_set.background_pixel = wascreen->wstyle.border_color.getPixel();
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
    int bw = wascreen->wstyle.border_width;
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
 * @fn    WaMenuItem(char *s) : WindowObject(0, 0)
 * @brief Constructor for WaMenuItem class
 *
 * Creates a new menu item
 *
 * @param s Menu item label
 */
WaMenuItem::WaMenuItem(char *s) : WindowObject(0, 0) {
    label = s;
    id = (Window) 0;
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
#ifdef XFT
    if (xftdraw) XftDrawDestroy(xftdraw);
#endif // XFT
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
    
    if (type == MenuTitleType)
        justify = menu->wascreen->mstyle.t_justify;
    else
        justify = menu->wascreen->mstyle.f_justify;
    
    switch (justify) {
        case LeftJustify: x = 10; break;
        case CenterJustify:
            if (type == MenuTitleType)
                x = ((menu->width) / 2) - ((width - 20) / 2);
            else x = ((menu->width - menu->bullet_width) / 2) -
                     ((width - 20) / 2);
            break;
        default:
            if (type == MenuTitleType) x = (menu->width) - (width - 10);
            else x = (menu->width - menu->bullet_width) - (width - 10);
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
#else // ! XFT
    XFontStruct *font;
    GC *gc, *bgc = NULL;
    
    if (type == MenuTitleType) {
        font = menu->wascreen->mstyle.t_font;
        gc = &menu->wascreen->mstyle.t_text_gc;
        y = menu->wascreen->mstyle.t_y_pos;
    } else {
        font = menu->wascreen->mstyle.f_font;
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
 * @fn    UnmapMenu(XEvent *e)
 * @brief Unmaps menu
 *
 * Unmaps the menu holding the menu item.
 * 
 * @param e Event causing function call
 */
void WaMenuItem::UnmapMenu(XEvent *e) {
    menu->Unmap();
}

/**
 * @fn    MapSubmenu(XEvent *e)
 * @brief Maps Submenu
 *
 * Maps menu items submenu, if there is one, at a good position close to the
 * menu item. If the submenu is already mapped then we do nothing.
 * 
 * @param e Event causing function call
 */
void WaMenuItem::MapSubmenu(XEvent *e) {
    int skip;
    if ((! (func_mask & MenuSubMask)) || submenu->mapped) return;

    Hilite();
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
    submenu->Map(menu->x + menu->width + menu->wascreen->wstyle.border_width,
                 menu->y + dy - skip);
}

/**
 * @fn    ReMapSubmenu(XEvent *e)
 * @brief Remaps Submenu
 *
 * Maps menu items submenu, if there is one, at a good position close to the
 * menu item. If the submenu is already mapped then we just move it to
 * the position we want to remap it to. 
 * 
 * @param e Event causing function call
 */
void WaMenuItem::ReMapSubmenu(XEvent *e) {
    int skip;
    if (! (func_mask & MenuSubMask)) return;

    Hilite();
    submenu->root_menu = menu;
    submenu->root_item = this;
    submenu->wf = menu->wf;
    submenu->rf = menu->rf;
    submenu->mf = menu->mf;
    submenu->ftype = menu->ftype;
    list<WaMenuItem *>::iterator it = submenu->item_list->begin();
    for (skip = 0; it != submenu->item_list->end(); ++it) {        
        if ((*it)->type == MenuTitleType)
            skip += (*it)->height + menu->wascreen->wstyle.border_width;
        else break;
    }
    submenu->ReMap(menu->x + menu->width + menu->wascreen->wstyle.border_width,
                 menu->y + dy - skip);
} 

/**
 * @fn    UnLinkMenu(XEvent *e)
 * @brief Unlink menu
 *
 * Removes link to menu tree from menu. Menu will not be apart of the menu tree
 * mapping the menu any longer after this. Unmapsubmenu and Unmapmenutree
 * functions applied somewhere in the old menu tree will not unmap this menu. 
 *
 * @param e Event causing function call
 */
void WaMenuItem::UnLinkMenu(XEvent *e) {
    menu->root_menu = NULL;
}

/**
 * @fn    Exec(XEvent *e)
 * @brief Execute program
 *
 * This function executes menu items command line, if there is one.
 *
 * @param e Event causing function call
 */
void WaMenuItem::Exec(XEvent *e) {
    if (! (func_mask & MenuExecMask)) return;

    waexec(exec, menu->wascreen->displaystring);
}

/**
 * @fn    Func(XEvent *e)
 * @brief Call function
 *
 * This function calls the function stored in menu items function pointer, if
 * there is a function pointer and the menu has been linked to an object of
 * same type as the function is a member of.
 *
 * @param e Event causing function call
 */
void WaMenuItem::Func(XEvent *e) {
    hash_map<int, WindowObject *>::iterator it;
    
    if ((func_mask & MenuWFuncMask) && (menu->ftype == MenuWFuncMask)) {
        if ((it = menu->waimea->window_table->find(menu->wf)) !=
            menu->waimea->window_table->end()) {
            if (((*it).second)->type == WindowType) {
                (*((WaWindow *) (*it).second).*(wfunc))(e);
            }
        }
    }
    else if ((func_mask & MenuRFuncMask) && (menu->ftype == MenuRFuncMask))
        ((*(menu->rf)).*(rfunc))(e);
    else if ((func_mask & MenuMFuncMask) && (menu->ftype == MenuMFuncMask))
        ((*(menu->mf)).*(mfunc))(e);
}

/**
 * @fn    Raise(XEvent *e)
 * @brief Lowers menu window in display stack
 *
 * Lowers the menu frame to the bottom of the display stack.
 *
 * @param e Event causing function call
 */
void WaMenuItem::Lower(XEvent *e) {
    XLowerWindow(menu->display, menu->frame);
}

/**
 * @fn    Move(XEvent *e)
 * @brief Moves the menu items menu
 *
 * Non-opaque moving of menu.
 *
 * @param e Event causing function call
 */
void WaMenuItem::Move(XEvent *e) {
    XEvent *event;
    int px, py;
    list<XEvent *> *maprequest_list;
    bool started = False;
    int nx = menu->x;
    int ny = menu->y;
    
    switch (e->type) {
        case ButtonRelease:
            nx = e->xbutton.x_root;
            ny = e->xbutton.y_root;
            menu->DrawOutline(nx - menu->x, ny - menu->y);
            menu->ToggleOutline();
            started = True;
        case ButtonPress:
            px = e->xbutton.x_root;
            py = e->xbutton.y_root; break;
        default: return;
    }
    maprequest_list = new list<XEvent *>;
    XGrabPointer(menu->display, e->xany.window, True, ButtonReleaseMask |
                 ButtonPressMask | PointerMotionMask | EnterWindowMask |
                 LeaveWindowMask, GrabModeAsync, GrabModeAsync, None, None,
                 CurrentTime);
    for (;;) {
        event = menu->waimea->eh->EventLoop(
            menu->waimea->eh->menu_move_return_mask);
        switch (event->type) {
            case MotionNotify:
                if (! started) {
                    menu->ToggleOutline();
                    started = True;
                }
                nx += event->xmotion.x_root - px;
                ny += event->xmotion.y_root - py;
                px  = event->xmotion.x_root;
                py  = event->xmotion.y_root;
                menu->DrawOutline(nx - menu->x, ny - menu->y);
                break;
            case LeaveNotify:
            case EnterNotify:
                if (menu->wascreen->west->id == event->xcrossing.window ||
                    menu->wascreen->east->id == event->xcrossing.window ||
                    menu->wascreen->north->id == event->xcrossing.window ||
                    menu->wascreen->south->id == event->xcrossing.window) {
                    menu->waimea->eh->ed.type = event->type;
                    menu->waimea->eh->ed.mod = event->xcrossing.state;
                    menu->waimea->eh->ed.detail = 0;
                    menu->waimea->eh->EvAct(event, event->xcrossing.window);
                }
                break;
            case MapRequest:
                maprequest_list->push_front(event); break;
            case ButtonPress:
            case ButtonRelease:
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
 * @fn    MoveOpaque(XEvent *e)
 * @brief Moves the menu items menu
 *
 * Opaque moving of menu.
 *
 * @param e Event causing function call
 */
void WaMenuItem::MoveOpaque(XEvent *e) {
    XEvent *event;
    int px, py;
    list<XEvent *> *maprequest_list;
    int nx = menu->x;
    int ny = menu->y;

    switch (e->type) {
        case ButtonRelease:
            nx = e->xbutton.x_root;
            ny = e->xbutton.y_root;
            menu->Move(nx - menu->x, ny - menu->y);
        case ButtonPress:
            px = e->xbutton.x_root;
            py = e->xbutton.y_root; break;
        default: return;
    }
    maprequest_list = new list<XEvent *>;
    XGrabPointer(menu->display, e->xany.window, True, ButtonReleaseMask |
                 ButtonPressMask | PointerMotionMask | EnterWindowMask |
                 LeaveWindowMask, GrabModeAsync, GrabModeAsync, None, None,
                 CurrentTime);
    for (;;) {
        event = menu->waimea->eh->EventLoop(
            menu->waimea->eh->menu_move_return_mask);
        switch (event->type) {
            case MotionNotify:
                nx += event->xmotion.x_root - px;
                ny += event->xmotion.y_root - py;
                px = event->xmotion.x_root;
                py = event->xmotion.y_root;
                menu->Move(nx - menu->x, ny - menu->y);
                break;
            case LeaveNotify:
            case EnterNotify:
                if (menu->wascreen->west->id == event->xcrossing.window ||
                    menu->wascreen->east->id == event->xcrossing.window ||
                    menu->wascreen->north->id == event->xcrossing.window ||
                    menu->wascreen->south->id == event->xcrossing.window) {
                    menu->waimea->eh->ed.type = event->type;
                    menu->waimea->eh->ed.mod = event->xcrossing.state;
                    menu->waimea->eh->ed.detail = 0;
                    menu->waimea->eh->EvAct(event, event->xcrossing.window);
                }
                break;
            case MapRequest:
                maprequest_list->push_front(event); break;
            case ButtonPress:
            case ButtonRelease:
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
            if ((*it)->menu)
                map_menu = (*it)->menu;
            ((*this).*((*it)->menufunc))(e);
        }
    }
    if (ed->type == EnterNotify) {
        Hilite();
    }
    else if (ed->type == LeaveNotify)
        DeHilite();
}
