/** -*- Mode: C++ -*-
 *
 * @file   TaskSwitcher.cc
 * @author David Reveman <c99drn@cs.umu.se>
 * @date   27-Nov-2001 09:15:41
 *
 * @brief Implementation of TaskSwitcher class
 *
 * Task Switcher and functions for controling it.
 *
 * Copyright (C) David Reveman. All rights reserved.
 *
 */

#include "TaskSwitcher.hh"


TaskSwitcher::TaskSwitcher(WaScreen *wascrn) : WaMenu("__taskswitcher__") {
    wascreen = wascrn;
    wawindow_list = wascreen->waimea->wawindow_list;
}

void TaskSwitcher::View(void) {
    WaWindow *ww;
    WaMenuItem *m;

    LISTCLEAR(item_list);
    built = False;
    
    m = new WaMenuItem("Task Switcher (DEVELOPMENT VERSION)");
    m->type = MenuTitleType;
    AddItem(m);

    list<WaWindow *>::iterator it = wawindow_list->begin();
    for (++it; it != wawindow_list->end(); ++it) {
        ww = (WaWindow *) *it;
        m = new WaMenuItem(ww->name);
        m->type = MenuItemType;
        m->wfunc = &WaWindow::Raise;
        m->func_mask |= MenuWFuncMask;
        AddItem(m);
    }
    ww = (WaWindow *) wawindow_list->front();
    m = new WaMenuItem(ww->name);
    m->type = MenuItemType;
    m->wfunc = &WaWindow::Raise;
    m->func_mask |= MenuWFuncMask;
    AddItem(m);

    Build(wascreen);
}
