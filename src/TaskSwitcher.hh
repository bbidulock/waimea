/** -*- Mode: C++ -*-
 *
 * @file   TaskSwitcher.hh
 * @author David Reveman <c99drn@cs.umu.se>
 * @date   27-Nov-2001 09:15:41
 *
 * @brief Definition of TaskSwitcher class
 *
 * Function declarations and variable definitions for TaskSwitcher class.
 *
 * Copyright (C) David Reveman. All rights reserved.
 *
 */

#ifndef __TaskSwitcher_hh
#define __TaskSwitcher_hh

class TaskSwitcher;

#include "Waimea.hh"
#include "WaMenu.hh"

class TaskSwitcher : public WaMenu {
public:
    TaskSwitcher(WaScreen *);

    void View(void);
    
private:
    WaScreen *wascreen;
    list<WaWindow *> *wawindow_list;
};

#endif // __TaskSwitcher_hh
