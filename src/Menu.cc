/**
 * @file   Menu.cc
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

#ifdef    HAVE_CONFIG_H
#  include "../config.h"
#endif // HAVE_CONFIG_H

#include "Menu.hh"

extern "C" {
#ifdef    HAVE_STDIO_H
#  include <stdio.h>
#endif // HAVE_STDIO_H

#ifdef    STDC_HEADERS
#  include <string.h>
#endif // STDC_HEADERS
}

#include <iostream>
using std::cerr;
using std::cout;
using std::endl;

/**
 * @fn    WaMenu(char *n) : WindowObject((Window) 0, MenuType)
 * @brief Constructor for WaMenu class
 *
 * Creates a new menu with no items
 *
 * @param n Name of menu
 */
WaMenu::WaMenu(char *n) : WindowObject((Window) 0, MenuType) {
    char *__m_wastrdup_tmp;
    
    name = __m_wastrdup(n);
    
    height = 0;
    width = 0;
    mapped = has_focus = built = tasksw = dynamic = dynamic_root =
        ignore = db = false;
    root_menu = NULL;
    root_item = NULL;
    wf = (Window) 0;
    rf = NULL;
    mf = NULL;

#ifdef RENDER
    pixmap = None;
    render_if_opacity = false;
#endif // RENDER
    
}

/**
 * @fn    ~WaMenu(void)
 * @brief Destructor for WaMenu class
 *
 * Deletes all menu items in the menu and removes the frame.
 */
WaMenu::~WaMenu(void) {
    LISTDELITEMS(item_list);
    if (built) {
        if (wascreen->config.menu_stacking == AlwaysOnTop)
            wascreen->wamenu_list_stacking_aot.remove(this);
        else if (wascreen->config.menu_stacking == AlwaysAtBottom)
            wascreen->wamenu_list_stacking_aab.remove(this);
        else
            wascreen->wa_list_stacking.remove(this);
        XDestroyWindow(display, frame);

#ifdef RENDER
        if (pixmap) {
            XSync(display, false);
            XFreePixmap(wascreen->pdisplay, pixmap);
        }
#endif // RENDER

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
    item_list.push_back(item);
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
    }
    bullet_width = cb_width = 0;

    f_height = wascreen->mstyle.item_height;
    t_height = wascreen->mstyle.title_height;
    
    list<WaMenu *>::iterator menu_it;
    list<WaMenuItem *>::iterator it = item_list.begin();
    for (; it != item_list.end(); ++it) {
        if ((*it)->func_mask & MenuSubMask) {
            for (i = 0; (*it)->sub[i] != '\0' && (*it)->sub[i] != '!'; i++);
            if ((*it)->sub[i] == '!') {
                (*it)->sdyn = (*it)->sdyn1 = true;
            } else {
                menu_it = wascreen->wamenu_list.begin();
                for (; menu_it != wascreen->wamenu_list.end(); ++menu_it) {
                    if (! strcmp((*menu_it)->name, (*it)->sub)) {
                        (*it)->submenu = *menu_it;
                        break;
                    }
                }
                if (menu_it == wascreen->wamenu_list.end()) {
                    WARNING << "no menu named \"" << (*it)->sub << "\"" <<
                        endl;
                    delete *it;
                    it = item_list.begin();
                }
            }
        }
        if ((*it)->func_mask2 & MenuSubMask) {
            for (i = 0; (*it)->sub2[i] != '\0' && (*it)->sub2[i] != '!'; i++);
            if ((*it)->sub2[i] == '!') {
                (*it)->sdyn2 = true;
            } else {
                menu_it = wascreen->wamenu_list.begin();
                for (; menu_it != wascreen->wamenu_list.end(); ++menu_it) {
                    if (! strcmp((*menu_it)->name, (*it)->sub2)) {
                        (*it)->submenu2 = *menu_it;
                        break;
                    }
                }
                if (menu_it == wascreen->wamenu_list.end()) {
                    WARNING << "no menu named \"" << (*it)->sub2 << "\"" <<
                        endl;
                    delete *it;
                    it = item_list.begin();
                }
            }
        }
    }

    for (it = item_list.begin(); it != item_list.end(); ++it) {
        if ((*it)->type == MenuSubType) {
            bullet_width =
                wascreen->mstyle.wa_b_font.Width(display,
                                                 wascreen->mstyle.bullet,
                                                 strlen(wascreen->
                                                        mstyle.bullet));
        }
        else if ((*it)->type == MenuCBItemType) {
            (*it)->cb_width2 = cb_width =
                wascreen->
                mstyle.wa_ct_font.Width(display,
                                        wascreen->mstyle.checkbox_true,
                                        strlen(
                                            wascreen->mstyle.checkbox_true));
            
            (*it)->cb_width1 =
                wascreen->
                mstyle.wa_cf_font.Width(display,
                                        wascreen->mstyle.checkbox_false,
                                        strlen(
                                            wascreen->mstyle.checkbox_false));
            
            if ((*it)->cb_width1 > cb_width) cb_width = (*it)->cb_width1;
        }
    }
    extra_width = (bullet_width >= cb_width) ? bullet_width: cb_width;
    
    int lasttype = 0;
    it = item_list.begin();
    for (i = 1; it != item_list.end(); ++it, ++i) {
        WaFont *wafont;
        if ((*it)->type == MenuTitleType) wafont = &wascreen->mstyle.wa_t_font;
        else wafont = &wascreen->mstyle.wa_f_font;

        char *l = (*it)->e_label? (*it)->e_label: (*it)->label;
        (*it)->width = wafont->Width(display, l, strlen(l)) + 20;
            
        if ((*it)->type == MenuCBItemType) {
            l = (*it)->e_label2? (*it)->e_label2: (*it)->label2;
            int cb_width = wafont->Width(display, l, strlen(l)) + 20;
            if ((cb_width + 20) > (*it)->width)
                (*it)->width = cb_width + 20;
        }
        
        if (((*it)->width + extra_width) > width)
            width = (*it)->width + extra_width;

        height += f_height;
        (*it)->height = (*it)->realheight = f_height;
        
        if ((*it)->type == MenuTitleType) {
            height -= f_height;
            height += t_height;
            (*it)->height = (*it)->realheight = t_height;
            height += wascreen->mstyle.border_width * 2;
            (*it)->realheight = t_height + wascreen->mstyle.border_width * 2;
            if ((lasttype == MenuTitleType) || (i == 1)) {
                height -= wascreen->mstyle.border_width;
                (*it)->realheight -= wascreen->mstyle.border_width;
            }
            if (i == item_list.size()) {
                height -= wascreen->mstyle.border_width;
                (*it)->realheight -= wascreen->mstyle.border_width;
            }
        }
        lasttype = (*it)->type;
    }
    if (width > (wascreen->width / 2)) width = wascreen->width / 2;
    
    WaTexture *texture = &wascreen->mstyle.back_frame;
    if (texture->getTexture() == (WaImage_Flat | WaImage_Solid)) {
        pbackframe = None;
        backframe_pixel = texture->getColor()->getPixel();
        if (wascreen->config.db) db = true;
    } else { 
        pbackframe = ic->renderImage(width, height, texture);
        if (wascreen->config.db && pbackframe != ParentRelative)
            db = true;
    }
    
#ifdef RENDER
    pixmap = XCreatePixmap(wascreen->pdisplay, wascreen->id, width,
                           height, wascreen->screen_depth);
#endif // RENDER
    
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
                              wascreen->screen_depth,
                              CopyFromParent, wascreen->visual,
                              CWOverrideRedirect | CWBackPixmap |
                              CWEventMask | CWColormap | CWBorderPixel,
                              &attrib_set);
    } else XResizeWindow(display, frame, width, height);

    attrib_set.event_mask = ButtonPressMask | ButtonReleaseMask |
        EnterWindowMask | LeaveWindowMask | KeyPressMask |
        KeyReleaseMask | ExposureMask | FocusChangeMask;

    int y, x, bw;
    it = item_list.begin();
    for (y = 0, lasttype = 0; it != item_list.end(); ++it) {
        x = bw = 0;
        if ((*it)->type == MenuTitleType) {
            bw = wascreen->mstyle.border_width;
            x = -bw;
            if ((y == 0) || (lasttype == MenuTitleType))
                y -= bw;
        }
        (*it)->id = XCreateWindow(display, frame, x, y, width,
                                  (*it)->height, bw,
                                  wascreen->screen_depth, CopyFromParent,
                                  wascreen->visual, CWOverrideRedirect | 
                                  CWBackPixel | CWEventMask | CWColormap,
                                  &attrib_set);
        waimea->window_table.insert(make_pair((*it)->id, (*it)));
        (*it)->dy = y;
        y += (*it)->height + bw * 2;
#ifdef XFT
        (*it)->xftdraw = XftDrawCreate(display, (Drawable) (*it)->id,
                                       wascreen->visual,
                                       wascreen->colormap);
#endif // XFT

#ifdef RENDER
        (*it)->pixmap = XCreatePixmap(wascreen->pdisplay, wascreen->id, width,
                                      (*it)->height, wascreen->screen_depth);
#endif // RENDER
        
        if ((*it)->type == MenuTitleType) {
            (*it)->actionlist = &wascreen->config.mtacts;
            (*it)->texture = &wascreen->mstyle.title;
        } else {
            switch ((*it)->type) {
                case MenuItemType:
                    (*it)->actionlist = &wascreen->config.miacts; break;
                case MenuCBItemType:
                    (*it)->actionlist = &wascreen->config.mcbacts; break;
                case MenuSubType:
                    (*it)->actionlist = &wascreen->config.msacts; break;
            }
            (*it)->texture = &wascreen->mstyle.back_frame;
        }
        lasttype = (*it)->type;
    }
    built = true;
}

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
        
#ifdef RENDER
        if (render_if_opacity && ! texture->getOpacity()) return;
        if (texture->getOpacity()) {
            pixmap = wascreen->ic->xrender(pbackframe, width, height, texture,
                                           wascreen->xrootpmap_id,
                                           x + wascreen->mstyle.border_width,
                                           y + wascreen->mstyle.border_width,
                                           pixmap);
            if (db) {
                Pixmap p_tmp;
                p_tmp = XCreatePixmap(display, wascreen->id, width, height,
                                      wascreen->screen_depth);
                GC gc = DefaultGC(display, wascreen->screen_number);
                XCopyArea(display, pixmap, p_tmp, gc, 0, 0, width,
                          height, 0, 0);
                list<WaMenuItem *>::iterator it = item_list.begin();
                for (; it != item_list.end(); ++it) {
                    (*it)->Draw(p_tmp, true, (*it)->dy);
                }
                XSetWindowBackgroundPixmap(display, frame, p_tmp);
                XClearWindow(display, frame);
                XFreePixmap(display, p_tmp);
            } else {
                XSetWindowBackgroundPixmap(display, frame, pixmap);
                XClearWindow(display, frame);
            }
        } else {
#endif // RENDER

            Pixmap p_tmp;
            if (db) {
                p_tmp = XCreatePixmap(display, wascreen->id, width,
                                      height, wascreen->screen_depth);
                if (pbackframe) {
                    GC gc = DefaultGC(display, wascreen->screen_number);
                    XCopyArea(display, pbackframe, p_tmp, gc, 0, 0, width,
                              height, 0, 0);
                } else {
                    XGCValues values;
                    values.foreground = backframe_pixel;
                    GC gc = XCreateGC(display, wascreen->id, GCForeground,
                                      &values);
                    XFillRectangle(display, p_tmp, gc, 0, 0, width, height);
                    XFreeGC(display, gc);
                }
                list<WaMenuItem *>::iterator it = item_list.begin();
                for (; it != item_list.end(); ++it) {
                    (*it)->Draw(p_tmp, true, (*it)->dy);
                }
                XSetWindowBackgroundPixmap(display, frame, p_tmp);
                XFreePixmap(display, p_tmp);
            }
            if (!db) {
                if (pbackframe)
                    XSetWindowBackgroundPixmap(display, frame, pbackframe);   
                else
                    XSetWindowBackground(display, frame, backframe_pixel);
            }
            XClearWindow(display, frame);
            
#ifdef RENDER
        }
#endif // RENDER
        
        list<WaMenuItem *>::iterator it = item_list.begin();
        for (; it != item_list.end(); ++it) {
            (*it)->Render();
        }
    }
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
    if (tasksw && item_list.size() < 2) return;
    if (mapped) return;

    if (wascreen->config.menu_stacking == AlwaysAtBottom) {
        wascreen->wamenu_list_stacking_aab.remove(this);
        wascreen->wamenu_list_stacking_aab.push_back(this);
        wascreen->WaLowerWindow(frame);
    } else if (wascreen->config.menu_stacking == AlwaysOnTop) {
        wascreen->wamenu_list_stacking_aot.remove(this);
        wascreen->wamenu_list_stacking_aot.push_front(this);
        wascreen->WaRaiseWindow(0);
    } else {
        wascreen->wa_list_stacking.remove(this);
        wascreen->wa_list_stacking.push_front(this);
        wascreen->WaRaiseWindow(frame);
    }
    x = mx;
    y = my;
    mapped = true;
    has_focus = false;
    XMoveWindow(display, frame, x, y);
    Render();
    XMapSubwindows(display, frame);
    XMapWindow(display, frame);
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
    if (tasksw && item_list.size() < 2) return;
    
    if (mapped) Move(mx - x, my - y);
    else {
        if (wascreen->config.menu_stacking == AlwaysAtBottom) {
            wascreen->wamenu_list_stacking_aab.remove(this);
            wascreen->wamenu_list_stacking_aab.push_back(this);
            wascreen->WaLowerWindow(frame);
        } else if (wascreen->config.menu_stacking == AlwaysOnTop) {
            wascreen->wamenu_list_stacking_aot.remove(this);
            wascreen->wamenu_list_stacking_aot.push_front(this);
            wascreen->WaRaiseWindow(0);
        } else {
            wascreen->wa_list_stacking.remove(this);
            wascreen->wa_list_stacking.push_front(this);
            wascreen->WaRaiseWindow(frame);
        }
    }
    x = mx;
    y = my;
    mapped = true;
    has_focus = false;
    XMoveWindow(display, frame, x, y);
    Render();
    XMapSubwindows(display, frame);
    XMapWindow(display, frame);
    XUngrabPointer(display, CurrentTime);
}

/**
 * @fn    Move(int dx, int dy, bool render)
 * @brief Moves menu
 *
 * Moves menu and all linked submenus to a specified position.
 *
 * @param dx Move in x coordinate relative to old position
 * @param dy Move in y coordinate relative to old position
 * @param render True if menu menu should be redrawn
 */
void WaMenu::Move(int dx, int dy, bool render) {
    list<WaMenuItem *>::iterator it = item_list.begin();
    for (; it != item_list.end(); ++it) {
        if (((*it)->func_mask & MenuSubMask) && (*it)->submenu &&
            (*it)->submenu->root_menu &&
            (*it)->submenu->mapped) {
            if (! (*it)->submenu->ignore)
                (*it)->submenu->Move(dx, dy, render);
        }
    }
    x += dx;
    y += dy;
    XMoveWindow(display, frame, x, y);

#ifdef RENDER
    if (render) {
        render_if_opacity = true;
        Render();
        render_if_opacity = false;
    }
#endif // RENDER
    
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

    if (wascreen->config.menu_stacking == AlwaysOnTop)
        wascreen->wamenu_list_stacking_aot.remove(this);
    else if (wascreen->config.menu_stacking == AlwaysAtBottom)
        wascreen->wamenu_list_stacking_aab.remove(this);
    
    root_menu = NULL;
    
    if (dynamic) UnmapSubmenus(false);

    list<WaMenuItem *>::iterator it = item_list.begin();
    for (; it != item_list.end(); ++it) {
        if ((*it)->hilited) {
            if ((*it)->func_mask & MenuSubMask) {
                if ((!(*it)->submenu) || (!((*it)->submenu->root_menu &&
                                            (*it)->submenu->mapped)))
                    (*it)->DeHilite();
            } else 
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
        if (! wascreen->wawindow_list.empty())
            wascreen->wawindow_list.front()->Focus(false);
    }
    if (dynamic_root) {
        wascreen->wamenu_list.remove(this);
        if (root_item) root_item->submenu = NULL;
        it = item_list.begin();
        for (; it != item_list.end(); ++it) {
            if ((*it)->submenu) {
                (*it)->submenu->root_item = NULL;
                if ((*it)->submenu->dynamic) {
                    (*it)->submenu->dynamic_root = true;
                    (*it)->submenu->Unmap(false);
                }
            }
        }
        if (root_item) {
            if (focus)
                root_item->Focus();
            else
                root_item->DeHilite();
        }
        delete this;
    }
    else {
        root_item = NULL;
        mapped = false;
    }
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
    ignore = true;
    list<WaMenuItem *>::iterator it = item_list.begin();
    for (; it != item_list.end(); ++it) {
        if ((*it)->func_mask & MenuSubMask) {
            if (! (*it)->submenu) (*it)->DeHilite();
            else if ((*it)->submenu->mapped && (*it)->submenu->root_menu) {
                if (! (*it)->submenu->ignore) {
                    (*it)->submenu->UnmapSubmenus(focus);
                    (*it)->submenu->Unmap(focus);
                }
            }
        }
    }
    ignore = false;
}

/**
 * @fn    UnmapTree(void)
 * @brief Unmaps menu tree
 *
 * Recursive function for unmapping complete menu trees. Unmaps all menus that 
 * are still linked in the current menu tree.
 */
void WaMenu::UnmapTree(void) {
    WaMenu *tmp = NULL;
    if (root_menu) {
        tmp = root_menu;
        root_menu = NULL;
    }
    UnmapSubmenus(false);
    Unmap(false);
    if (tmp) tmp->UnmapTree();
}

/**
 * @fn    CreateOutline(void)
 * @brief Creates outline
 *
 * Creates four windows used for displaying an outline when doing
 * non opaque moving and resizing of the menu.
 */
void WaMenu::CreateOutline(void) {
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
    wascreen->always_on_top_list.push_back(o_west);
    wascreen->always_on_top_list.push_back(o_east);
    wascreen->always_on_top_list.push_back(o_north);
    wascreen->always_on_top_list.push_back(o_south);
    XMapWindow(display, o_west);
    XMapWindow(display, o_east);
    XMapWindow(display, o_north);
    XMapWindow(display, o_south);
    
    list<WaMenuItem *>::iterator it = item_list.begin();
    for (; it != item_list.end(); ++it) {        
        if (((*it)->func_mask & MenuSubMask) && (*it)->submenu &&
            (*it)->submenu->root_menu && (*it)->submenu->mapped) {
            (*it)->submenu->CreateOutline();
        }
    }
    wascreen->WaRaiseWindow(0);
}

/**
 * @fn    DestroyOutline(void)
 * @brief Destroys window outline
 *
 * Destorys the four outline windows.
 */
void WaMenu::DestroyOutline(void) {
    list<WaMenuItem *>::iterator it = item_list.begin();
    for (; it != item_list.end(); ++it) {        
        if (((*it)->func_mask & MenuSubMask) && (*it)->submenu &&
            (*it)->submenu->root_menu && (*it)->submenu->mapped) {
            (*it)->submenu->DestroyOutline();
        }
    }
    wascreen->always_on_top_list.remove(o_west);
    wascreen->always_on_top_list.remove(o_east);
    wascreen->always_on_top_list.remove(o_north);
    wascreen->always_on_top_list.remove(o_south);
    XDestroyWindow(display, o_west);
    XDestroyWindow(display, o_east);
    XDestroyWindow(display, o_north);
    XDestroyWindow(display, o_south);
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
    list<WaMenuItem *>::iterator it = item_list.begin();
    for (; it != item_list.end(); ++it) {        
        if (((*it)->func_mask & MenuSubMask) && (*it)->submenu &&
            (*it)->submenu->root_menu && (*it)->submenu->mapped) {
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
    if (wascreen->config.menu_stacking == AlwaysOnTop) {
        wascreen->wamenu_list_stacking_aot.remove(this);
        wascreen->wamenu_list_stacking_aot.push_front(this);
    } else if (wascreen->config.menu_stacking == AlwaysAtBottom) {
        wascreen->wamenu_list_stacking_aab.remove(this);
        wascreen->wamenu_list_stacking_aab.push_back(this);
    } else {
        wascreen->wa_list_stacking.remove(this);
        wascreen->wa_list_stacking.push_front(this);
    }     
    wascreen->WaRaiseWindow(frame);
    list<WaMenuItem *>::iterator it = item_list.begin();
    for (; it != item_list.end(); ++it) {
        if (! (*it)->db) (*it)->Draw();
    }
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
    list<WaMenuItem *>::iterator it = item_list.begin();
    for (; it != item_list.end(); ++it)
        while (XCheckTypedWindowEvent(display, (*it)->id, EnterNotify, &e));
    it = item_list.begin();
    for (; it != item_list.end() &&
             (*it)->type == MenuTitleType; ++it);
    if (it != item_list.end()) (*it)->Focus();
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
    char *__m_wastrdup_tmp;
    
    label = label1 = __m_wastrdup(s);
    id = (Window) 0;
    func_mask = func_mask1 = func_mask2 = height = width = dy =
        realheight = cb = 0;
    wfunc = wfunc2 = NULL;
    rfunc = rfunc2 = NULL;
    mfunc = mfunc2 = NULL;
    wf = (Window) 0;
    submenu = submenu1 = submenu2 = NULL;
    exec = param = sub = exec1 = param1 = sub1 = label2 = exec2 = param2 =
        sub2 = cbox = NULL;
    move_resize = sdyn = sdyn1 = sdyn2 = db = false;
    e_label = e_label1 = e_label2 = NULL;
    e_sub = e_sub1 = e_sub2 = NULL;
    
#ifdef XFT
    xftdraw = (Drawable) 0;
#endif // XFT

#ifdef RENDER
    pixmap = None;
#endif // RENDER
    
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
    if (e_label1) delete [] e_label1;
    if (e_label2) delete [] e_label2;
    if (e_sub1) delete [] e_sub1;
    if (e_sub2) delete [] e_sub2;

    menu->item_list.remove(this);
        
#ifdef XFT
    if (xftdraw) XftDrawDestroy(xftdraw);
#endif // XFT

#ifdef RENDER    
    if (pixmap != None) XFreePixmap(menu->wascreen->pdisplay, pixmap);
#endif // RENDER
    
    if (id) {
        menu->waimea->window_table.erase(id);
        XDestroyWindow(menu->display, id);
    }
}

/**
 * @fn    Render(void)
 * @brief Render transparent background
 *
 * Renders and sets transperancy background.
 */
void WaMenuItem::Render(void) {
    if (type != MenuTitleType && ! hilited) {
        XSetWindowBackgroundPixmap(menu->display, id, ParentRelative);
        XClearWindow(menu->display, id);
        db = true;
        if (! menu->db) {
            db = false;
            Draw();
        }
        return;
    }
    int bw = menu->wascreen->mstyle.border_width;
    
    if (((menu->x + menu->width) > 0 && menu->x < menu->wascreen->width) && 
        ((menu->y + dy + height) > 0 && (menu->y + dy) <
         menu->wascreen->height)) {
        if (type == MenuTitleType) {
#ifdef RENDER
            if (menu->render_if_opacity && ! texture->getOpacity()) return;
            if (texture->getOpacity()) {
                pixmap = menu->wascreen->ic->xrender(menu->ptitle, menu->width,
                                                     height, texture,
                                                     menu->wascreen->
                                                     xrootpmap_id,
                                                     menu->x + bw,
                                                     menu->y + dy + bw,
                                                     pixmap);
                if (menu->wascreen->config.db) {
                    db = true;
                    Draw(pixmap);
                    return;
                }
                else {
                    XSetWindowBackgroundPixmap(menu->display, id, pixmap);
                    XClearWindow(menu->display, id);
                    db = false;
                    Draw();
                }
            }
#endif // RENDER
            if (menu->wascreen->config.db) {
                db = true;
                if (menu->ptitle) Draw(menu->ptitle);
                else Draw((Drawable) 2);
                return;
            } else {
                if (menu->ptitle)
                    XSetWindowBackgroundPixmap(menu->display, id,
                                               menu->ptitle);
                else
                    XSetWindowBackground(menu->display, id, menu->title_pixel);
                XClearWindow(menu->display, id);
                db = false;
                Draw();
                return;
            }
        }
        else if (hilited) {
#ifdef RENDER
            if (menu->render_if_opacity && ! texture->getOpacity()) return;
            if (texture->getOpacity()) {
                pixmap = menu->wascreen->ic->xrender(menu->philite,
                                                     menu->width,
                                                     height, texture,
                                                     menu->wascreen->
                                                     xrootpmap_id,
                                                     menu->x +bw,
                                                     menu->y + dy + bw,
                                                     pixmap);
                if (menu->wascreen->config.db) {
                    db = true;
                    Draw(pixmap);
                    return;
                }
                else {
                    XSetWindowBackgroundPixmap(menu->display, id, pixmap);
                    XClearWindow(menu->display, id);
                    db = false;
                    Draw();
                    return;
                }
            }
#endif // RENDER
            
            if (menu->wascreen->config.db) {
                db = true;
                if (menu->philite) Draw(menu->philite);
                else Draw((Drawable) 2);
                return;
            } else {
                if (menu->philite) {
                    XSetWindowBackgroundPixmap(menu->display, id,
                                               menu->philite);
                }
                else {
                    XSetWindowBackground(menu->display, id,
                                         menu->hilite_pixel);
                }
                XClearWindow(menu->display, id);
                db = false;
                Draw();
                return;
            }
        }
    }
}

/**
 * @fn    Draw(Drawable drawable, bool frame, int y)
 * @brief Draws menu item foreground
 *
 * Draws menu item text.
 *
 * @param drawable Drawable to draw on
 */
void WaMenuItem::Draw(Drawable drawable, bool frame, int y) {
    int x = 0, justify;
    char *l;
    int org_y = y;

#ifdef RENDER
    if (menu->render_if_opacity && ! texture->getOpacity()) return;
#endif // RENDER
        
    WaFont *wafont = (hilited && !frame)? &menu->wascreen->mstyle.wa_fh_font:
        &menu->wascreen->mstyle.wa_f_font;
    if (type == MenuTitleType)
        wafont = &menu->wascreen->mstyle.wa_t_font;
    
    WaFont *wafont_b = (hilited && !frame)? &menu->wascreen->mstyle.wa_bh_font:
        &menu->wascreen->mstyle.wa_b_font;

    if (drawable == ParentRelative) {
        XSetWindowBackgroundPixmap(menu->display, id, drawable);
        XClearWindow(menu->display, id);
        return;
    }
    if (! drawable) XClearWindow(menu->display, id);

    Pixmap p_tmp;
    if (drawable && !frame) {
        p_tmp = XCreatePixmap(menu->display, menu->wascreen->id,
                              menu->width, height,
                              menu->wascreen->screen_depth);
        if (drawable == (Drawable) 2) {
            XGCValues values;
            values.foreground = texture->getColor()->getPixel();
            GC gc = XCreateGC(menu->display, menu->wascreen->id,
                              GCForeground, &values);
            XFillRectangle(menu->display, p_tmp, gc, 0, 0, menu->width,
                           height);
            XFreeGC(menu->display, gc);
        } else {
            GC gc = DefaultGC(menu->display, menu->wascreen->screen_number);
            XCopyArea(menu->display, drawable, p_tmp, gc, 0, 0, menu->width,
                      height, 0, 0);
        }
    }
    if (frame) p_tmp = drawable;
    
    if (cb) UpdateCBox();

    if (e_label) l = e_label;
    else l = label;
    
    width = wafont_b->Width(menu->display, l, strlen(l)) + 20;    
    
    if (type == MenuTitleType)
        justify = menu->wascreen->mstyle.t_justify;
    else
        justify = menu->wascreen->mstyle.f_justify;
    if (menu->width <= width) justify = LeftJustify;
    
    switch (justify) {
        case LeftJustify: x += 10; break;
        case CenterJustify:
            if (type == MenuTitleType)
                x += (menu->width / 2) - ((width - 10) / 2);
            else if (type == MenuCBItemType)
                x += ((menu->width - menu->cb_width) / 2) - ((width - 10) / 2);
            else x += ((menu->width - menu->extra_width) / 2) -
                     ((width - 10) / 2);
            break;
        default:
            if (type == MenuTitleType)
                x += menu->width - (width - 10);
            else if (type == MenuCBItemType)
                x += (menu->width - menu->cb_width) - (width - 10);
            else x += (menu->width - menu->extra_width) - (width - 10);
    }

    if (type == MenuTitleType) y += menu->wascreen->mstyle.t_y_pos;
    else y += menu->wascreen->mstyle.f_y_pos;

#ifdef    XFT
    if (drawable) XftDrawChange(xftdraw, p_tmp);
    else XftDrawChange(xftdraw, id);
#endif // XFT

    wafont->Draw(menu->display, (drawable)? p_tmp: id,
                 
#ifdef    XFT
                 xftdraw,
#endif // XFT
                 
                 x, y, l, strlen(l));

    if (type == MenuSubType) {
        y = org_y + menu->wascreen->mstyle.b_y_pos;
        wafont_b->Draw(menu->display, (drawable)? p_tmp: id,

#ifdef    XFT
                       xftdraw,
#endif // XFT

                       menu->width - (menu->bullet_width + 5), y,
                       menu->wascreen->mstyle.bullet,
                       strlen(menu->wascreen->mstyle.bullet));
    }
    else if (type == MenuCBItemType) {
        if (frame) {
            if (wafont_cb == &menu->wascreen->mstyle.wa_cth_font)
                wafont_cb = &menu->wascreen->mstyle.wa_ct_font;
            else if (wafont_cb == &menu->wascreen->mstyle.wa_cfh_font)
                wafont_cb = &menu->wascreen->mstyle.wa_cf_font;
        }
        wafont_cb->Draw(menu->display, (drawable)? p_tmp: id,

#ifdef    XFT
                        xftdraw,
#endif // XFT

                        menu->width - (cb_width + 5), org_y + cb_y, cbox,
                        strlen(cbox));
    }
    
    if (drawable && !frame) {
        XSetWindowBackgroundPixmap(menu->display, id, p_tmp);
        XClearWindow(menu->display, id);
        XFreePixmap(menu->display, p_tmp);
    }
}

/**
 * @fn    Hilite(void)
 * @brief Hilite menu item
 *
 * Sets menu item look to hilited look.
 */
void WaMenuItem::Hilite(void) {
    if (type == MenuTitleType || hilited) return;
    
    list<WaMenuItem *>::iterator it = menu->item_list.begin();
    for (; it != menu->item_list.end(); ++it) {        
        if ((*it)->hilited && menu->has_focus)
            if (!(((*it)->func_mask & MenuSubMask) && (*it)->submenu && 
                  (*it)->submenu->mapped))
                (*it)->DeHilite();
    }
    hilited = true;
    texture = &menu->wascreen->mstyle.hilite;
    Render();
}

/**
 * @fn    DeHilite(void)
 * @brief DeHilite menu item
 *
 * Sets menu item look to dehilited look.
 */
void WaMenuItem::DeHilite(void) {
    if (type == MenuTitleType || !hilited) return;
    hilited = false;
    texture = &menu->wascreen->mstyle.back_frame;
    
    Render();
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
 * @fn    MapSubmenu(XEvent *, WaAction *, bool focus, bool only)
 * @brief Maps Submenu
 *
 * Maps menu items submenu, if there is one, at a good position close to the
 * menu item. If the submenu is already mapped then we do nothing.
 *
 * @param focus True if we should focus first item in submenu
 * @param only True if we should unmap all other submenus before mapping this
 */
void WaMenuItem::MapSubmenu(XEvent *, WaAction *, bool focus, bool only) {
    int skip, x, y, diff;

    if (! in_window) return;
    if (menu->waimea->eh->move_resize != EndMoveResizeType) return;
    if (! (func_mask & MenuSubMask)) return;
    
    Hilite();
    if (only) {
        list<WaMenuItem *>::iterator it = menu->item_list.begin();
        for (; it != menu->item_list.end(); ++it) {        
            if ((*it)->hilited && *it != this) {
                if (((*it)->func_mask & MenuSubMask) && (*it)->submenu &&
                    (*it)->submenu->mapped && (*it)->submenu->root_menu) {
                    (*it)->submenu->Unmap(false);
                    (*it)->DeHilite();
                }
            }
        }
    }
    if (sdyn && (! submenu)) {
        XSync(menu->display, false);
        if (! (submenu = menu->wascreen->GetMenuNamed(e_sub? e_sub : sub)))
            return;
    }
    if (submenu->mapped) return;
    
    if (submenu->tasksw) menu->wascreen->window_menu->Build(menu->wascreen);
    submenu->root_menu = menu;
    submenu->root_item = this;
    submenu->wf = menu->wf;
    submenu->rf = menu->rf;
    submenu->mf = menu->mf;
    submenu->ftype = menu->ftype;
    list<WaMenuItem *>::iterator it = submenu->item_list.begin();
    for (skip = 0; it != submenu->item_list.end(); ++it) {        
        if ((*it)->type == MenuTitleType)
            skip += (*it)->realheight;
        else break;
    }

    int workx, worky, workw, workh;
    menu->wascreen->GetWorkareaSize(&workx, &worky, &workw, &workh);
    
    x = menu->x + menu->width + menu->wascreen->mstyle.border_width;
    y = menu->y + dy - skip;
    diff = (y + submenu->height + menu->wascreen->mstyle.border_width * 2) -
        (workh + worky);
    if (diff > 0) y -= diff;
    if (y < 0) y = 0;
    if ((x + submenu->width + menu->wascreen->mstyle.border_width * 2) >
        (unsigned int) (workw + workx))
        x = menu->x - submenu->width - menu->wascreen->mstyle.border_width;

    submenu->Map(x, y);
    if (focus) submenu->FocusFirst();
}

/**
 * @fn    RemapSubmenu(XEvent *, WaAction *, bool focus)
 * @brief Remaps Submenu
 *
 * Maps menu items submenu, if there is one, at a good position close to the
 * menu item. If the submenu is already mapped then we just move it to
 * the position we want to remap it to.
 *
 * @param focus True if we should focus first item in submenu
 */
void WaMenuItem::RemapSubmenu(XEvent *, WaAction *, bool focus) {
    int skip, x, y, diff;

    if (! in_window) return;
    if (! (func_mask & MenuSubMask)) return;
    if (submenu && (submenu == menu)) return;
    if (menu->waimea->eh->move_resize != EndMoveResizeType) return;     
    
    Hilite();
    if (sdyn) {
        XSync(menu->display, false);
        if (submenu) {
            hilited = false;
            submenu->Unmap(submenu->has_focus);
            hilited = true;
        }
        if (! (submenu = menu->wascreen->GetMenuNamed(e_sub? e_sub : sub)))
            return;
    }
    
    if (submenu->tasksw) menu->wascreen->window_menu->Build(menu->wascreen);
    submenu->root_menu = menu;
    submenu->root_item = this;
    submenu->wf = menu->wf;
    submenu->rf = menu->rf;
    submenu->mf = menu->mf;
    submenu->ftype = menu->ftype;
    list<WaMenuItem *>::iterator it = submenu->item_list.begin();
    for (skip = 0; it != submenu->item_list.end(); ++it) {
        if ((*it)->type == MenuTitleType)
            skip += (*it)->height + menu->wascreen->mstyle.border_width;
        else break;
    }

    int workx, worky, workw, workh;
    menu->wascreen->GetWorkareaSize(&workx, &worky, &workw, &workh);
    
    x = menu->x + menu->width + menu->wascreen->mstyle.border_width;
    y = menu->y + dy - skip;
    diff = (y + submenu->height + menu->wascreen->mstyle.border_width * 2) -
        (workh + worky);
    if (diff > 0) y -= diff;
    if (y < 0) y = 0;
    if ((x + submenu->width + menu->wascreen->mstyle.border_width * 2) >
        (unsigned int) (workw + workx))
        x = menu->x - submenu->width - menu->wascreen->mstyle.border_width;
    menu->ignore = true;
    submenu->ReMap(x, y);
    menu->ignore = false;
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
    map<Window, WindowObject *>::iterator it;
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
        if ((it = menu->waimea->window_table.find(func_win)) !=
            menu->waimea->window_table.end()) {
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
    if (menu->wascreen->config.menu_stacking == AlwaysOnTop) {
        menu->wascreen->wamenu_list_stacking_aot.remove(menu);
        menu->wascreen->wamenu_list_stacking_aot.push_back(menu);
    } else if (menu->wascreen->config.menu_stacking == AlwaysAtBottom) {
        menu->wascreen->wamenu_list_stacking_aab.remove(menu);
        menu->wascreen->wamenu_list_stacking_aab.push_front(menu);
    } else {
        menu->wascreen->wa_list_stacking.remove(menu);
        menu->wascreen->wa_list_stacking.push_back(menu);
    }     
    menu->wascreen->WaLowerWindow(menu->frame);
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
    if (XGrabPointer(menu->display, id, true, ButtonReleaseMask |
                     ButtonPressMask | PointerMotionMask | EnterWindowMask |
                     LeaveWindowMask, GrabModeAsync, GrabModeAsync,
                     menu->wascreen->id, menu->waimea->move_cursor,
                     CurrentTime) != GrabSuccess) {
        move_resize = false;
        menu->waimea->eh->move_resize = EndMoveResizeType;
        return;
    }
    if (XGrabKeyboard(menu->display, id, true, GrabModeAsync, GrabModeAsync,
                      CurrentTime) != GrabSuccess) {
        move_resize = false;
        menu->waimea->eh->move_resize = EndMoveResizeType;
        return;
    }
    for (;;) {
        menu->waimea->eh->EventLoop(
            menu->waimea->eh->menu_viewport_move_return_mask, &event);
        switch (event.type) {
            case MotionNotify:
                while (XCheckTypedWindowEvent(menu->display,
                                              event.xmotion.window,
                                              MotionNotify, &event));
                if (! started) {
                    menu->CreateOutline();
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
                } else if (event.type == EnterNotify &&
                           event.xany.window != id) {
                    int cx, cy;
                    XQueryPointer(menu->display, menu->wascreen->id,
                                  &w, &w, &cx,
                                  &cy, &i, &i, &ui);
                    nx += cx - px;
                    ny += cy - py;
                    px = cx;
                    py = cy;
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
                if (started) menu->DestroyOutline();
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
    if (XGrabPointer(menu->display, id, true, ButtonReleaseMask |
                     ButtonPressMask | PointerMotionMask | EnterWindowMask |
                     LeaveWindowMask, GrabModeAsync, GrabModeAsync,
                     menu->wascreen->id, menu->waimea->move_cursor,
                     CurrentTime) != GrabSuccess) {
        move_resize = false;
        menu->waimea->eh->move_resize = EndMoveResizeType;
        return;
    }
    if (XGrabKeyboard(menu->display, id, true, GrabModeAsync, GrabModeAsync,
                      CurrentTime) != GrabSuccess) {
        move_resize = false;
        menu->waimea->eh->move_resize = EndMoveResizeType;
        return;
    }
    for (;;) {
        menu->waimea->eh->EventLoop(
            menu->waimea->eh->menu_viewport_move_return_mask, &event);
        switch (event.type) {
            case MotionNotify:
                while (XCheckTypedWindowEvent(menu->display,
                                              event.xmotion.window,
                                              MotionNotify, &event));
                nx += event.xmotion.x_root - px;
                ny += event.xmotion.y_root - py;
                px = event.xmotion.x_root;
                py = event.xmotion.y_root;
                menu->Move(nx - menu->x, ny - menu->y
                         
#ifdef RENDER
                           , !menu->wascreen->config.lazy_trans
#endif // RENDER
                           
                           );
                break;
            case LeaveNotify:
            case EnterNotify:
                if (menu->wascreen->west->id == event.xcrossing.window ||
                    menu->wascreen->east->id == event.xcrossing.window ||
                    menu->wascreen->north->id == event.xcrossing.window ||
                    menu->wascreen->south->id == event.xcrossing.window) {
                    menu->waimea->eh->HandleEvent(&event);
                } else if (event.type == EnterNotify &&
                           event.xany.window != id) {
                    int cx, cy;
                    XQueryPointer(menu->display, menu->wascreen->id,
                                  &w, &w, &cx,
                                  &cy, &i, &i, &ui);
                    nx += cx - px;
                    ny += cy - py;
                    px = cx;
                    py = cy;
                    menu->Move(nx - menu->x, ny - menu->y
                         
#ifdef RENDER
                               , !menu->wascreen->config.lazy_trans
#endif // RENDER
                                   
                               );
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
                menu->Move(0, 0, true);
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

    int workx, worky, workw, workh;
    menu->wascreen->GetWorkareaSize(&workx, &worky, &workw, &workh);

    menu->wascreen->window_menu->Build(menu->wascreen);
    menu->wascreen->window_menu->ReMap(workx +
                                       (workw / 2 -
                                        menu->wascreen->window_menu->width /
                                        2), worky +
                                       (workh / 2 -
                                        menu->wascreen->window_menu->height /
                                        2));
    menu->wascreen->window_menu->FocusFirst();
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
    list<WaWindow *>::iterator it = menu->wascreen->wawindow_list.begin();
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
    menu->wascreen->wawindow_list.back()->Raise(e, ac);
    menu->wascreen->wawindow_list.back()->FocusVis(e, ac);
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
    list<WaMenuItem *>::iterator it = menu->item_list.begin();
    for (; it != menu->item_list.end(); ++it) {
        if (*it == this) {
            for (++it; it != menu->item_list.end() &&
                     (*it)->type == MenuTitleType; ++it);
            if (it == menu->item_list.end()) {
                it = menu->item_list.begin();
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
    list<WaMenuItem *>::reverse_iterator it = menu->item_list.rbegin();
    for (; it != menu->item_list.rend(); ++it) {
        if (*it == this) {
            for (++it; it != menu->item_list.rend() &&
                     (*it)->type == MenuTitleType; ++it);
            if (it == menu->item_list.rend()) {
                it = menu->item_list.rbegin();
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

    if (ed->type == EnterNotify) {
        if (XCheckTypedWindowEvent(menu->display, e->xany.window, 
                                   LeaveNotify, e)) {
            XPutBackEvent(menu->display, e);
            return;
        }
        Hilite();
        if (menu->has_focus && type != MenuTitleType) Focus();
        XSync(menu->display, false);
    }
   
    if (menu->waimea->eh->move_resize != EndMoveResizeType)
        ed->mod |= MoveResizeMask;

    list<WaAction *>::iterator it = acts->begin();
    for (; it != acts->end(); ++it) {
        if (eventmatch(*it, ed)) {
            if ((*it)->delay.tv_sec || (*it)->delay.tv_usec) {
                Interrupt *i = new Interrupt(*it, e, id);
                menu->waimea->timer->AddInterrupt(i);
            } else {
                if ((*it)->exec)
                    waexec((*it)->exec, menu->wascreen->displaystring);
                else 
                    ((*this).*((*it)->menufunc))(e, *it);
            }
        }
    }
    if (ed->type == LeaveNotify) {
        if (func_mask & MenuSubMask) {
            if ((! submenu) || (! submenu->mapped)) 
                DeHilite();
        } else 
            DeHilite();
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
    map<Window, WindowObject *>::iterator it;
    Window func_win;
    bool true_false = false;
    WaWindow *ww;

    if (cb) {
        if (wf) func_win = wf;
        else func_win = menu->wf;
        if ((func_mask & MenuWFuncMask) &&
            ((menu->ftype == MenuWFuncMask) || wf)) {
            if ((it = menu->waimea->window_table.find(func_win)) !=
                menu->waimea->window_table.end()) {
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
                        if (hilited)
                            wafont_cb = &menu->wascreen->mstyle.wa_cth_font;
                        else
                            wafont_cb = &menu->wascreen->mstyle.wa_ct_font;
                        
                        cb_y = menu->wascreen->mstyle.ct_y_pos;
                        if (cbox != menu->wascreen->mstyle.checkbox_true)
                            menu->cb_db_upd = true;
                        cbox = menu->wascreen->mstyle.checkbox_true;
                        label = label2;
                        sub = sub2;
                        wfunc = wfunc2;
                        rfunc = rfunc2;
                        mfunc = mfunc2;
                        func_mask = func_mask2;
                        cb_width = cb_width2;
                        param = param2;
                        sdyn = sdyn2;
                        e_label = e_label2;
                        e_sub = e_sub2;
                    }
                    else {
                        if (hilited)
                            wafont_cb = &menu->wascreen->mstyle.wa_cfh_font;
                        else
                            wafont_cb = &menu->wascreen->mstyle.wa_cf_font;
                        
                        cb_y = menu->wascreen->mstyle.cf_y_pos;
                        if (cbox != menu->wascreen->mstyle.checkbox_false)
                            menu->cb_db_upd = true;
                        cbox = menu->wascreen->mstyle.checkbox_false;
                        label = label1;
                        sub = sub1;
                        wfunc = wfunc1;
                        rfunc = rfunc1;
                        mfunc = mfunc1;
                        func_mask = func_mask1;
                        cb_width = cb_width1;
                        param = param1;
                        sdyn = sdyn1;
                        e_label = e_label1;
                        e_sub = e_sub1;
                    }
                }
            }
        }
    }
}

/**
 * @fn    ExpandAll(WaWindow *w)
 * @brief Window info expansion for WaMenuItem
 *
 * Expands label and sub strings for WaMenuItem.
 *
 * @param w WaWindow to get expansion info from
 *
 * @return 1 if item label was expanded otherwise 0
 */
int WaMenuItem::ExpandAll(WaWindow *w) {
    if (e_label1) delete [] e_label1;
    e_label1 = expand(label1, w);
    if (e_label2) delete [] e_label2;
    e_label2 = expand(label2, w);
    if (e_sub1) delete [] e_sub1;
    e_sub1 = expand(sub1, w);
    if (e_sub2) delete [] e_sub2;
    e_sub2 = expand(sub2, w);
    
    e_label = e_label1;
    e_sub = e_sub1;

    if (e_label1 || e_label2) return 1;
    else return 0;
}

/** 
 * @fn    WindowMenu(void) : WaMenu("__windowlist__")
 * @brief Constructor for WindowMenu class
 *
 * Creates a WindowMenu object, used for fast switching between windows.
 * This is a WaMenu with some extra functionality.
 * 
 */
WindowMenu::WindowMenu(void) : WaMenu("__windowlist__") {
    WaMenuItem *m;
    
    tasksw = true;
    m = new WaMenuItem("Window List");
    m->type = MenuTitleType;
    AddItem(m);
}

/** 
 * @fn    Build(WaScreen *wascreen) 
 * @brief Builds WindowMenu
 *
 * Overloaded Build function to make WindowMenu build a menu from
 * current windows.
 *
 * @param wascreen WaScreen to map menu on.
 */
void WindowMenu::Build(WaScreen *wascreen) {
    WaWindow *ww;
    WaMenuItem *m;
    wawindow_list = &wascreen->wawindow_list;
    
    LISTCLEAR(item_list);

    list<WaWindow *>::iterator it = wawindow_list->begin();
    for (; it != wawindow_list->end() &&
             ((WaWindow *) *it)->flags.tasklist != true; ++it);
    
    if (it == wawindow_list->end()) return;

    m = new WaMenuItem("Window List");
    m->type = MenuTitleType;
    AddItem(m);
    
    for (++it; it != wawindow_list->end(); ++it) {
        ww = (WaWindow *) *it;
        if (ww->flags.tasklist) {
            m = new WaMenuItem(ww->name);
            m->type = MenuItemType;
            m->wfunc = &WaWindow::RaiseFocus;
            m->func_mask |= MenuWFuncMask;
            m->func_mask1 |= MenuWFuncMask;
            m->wf = ww->id;
            AddItem(m);
        }
    }
    it = wawindow_list->begin();
    for (; it != wawindow_list->end() &&
             ((WaWindow *) *it)->flags.tasklist != true; ++it);
    
    ww = (WaWindow *) *it;
    m = new WaMenuItem(ww->name);
    m->type = MenuItemType;
    m->wfunc = &WaWindow::RaiseFocus;
    m->func_mask |= MenuWFuncMask;
    m->func_mask1 |= MenuWFuncMask;
    m->wf = ww->id;
    AddItem(m);
    
    WaMenu::Build(wascreen);
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
void WaMenuItem::GoToDesktop(XEvent *, WaAction *ac) {
    if (ac->param) menu->wascreen->GoToDesktop((unsigned int) atoi(ac->param));
}
void WaMenuItem::NextDesktop(XEvent *, WaAction *) {
    menu->wascreen->NextDesktop(NULL, NULL);
}
void WaMenuItem::PreviousDesktop(XEvent *, WaAction *) {
    menu->wascreen->PreviousDesktop(NULL, NULL);
}
void WaMenuItem::Restart(XEvent *e, WaAction *ac) {
    menu->wascreen->Restart(e, ac);
}
void WaMenuItem::Exit(XEvent *e, WaAction *ac) {
    menu->wascreen->Exit(e, ac);
}
