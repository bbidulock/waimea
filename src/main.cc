/**
 * @file   main.cc
 * @author David Reveman <c99drn@cs.umu.se>
 * @date   01-May-2001 17:58:24
 *
 * @brief Main body implementation
 *
 * Starts Waimea window manager.
 *
 * Copyright (C) David Reveman.  All rights reserved.
 */

#ifdef    HAVE_CONFIG_H
#  include "../config.h"
#endif // HAVE_CONFIG_H

#include "Waimea.hh"

#ifdef    HAVE_STDIO_H
#  include <stdio.h>
#endif // HAVE_STDIO_H

#ifdef    HAVE_IOSTREAM
#  include <iostream>
#endif // HAVE_IOSTREAM

using std::cerr;
using std::cout;
using std::endl;

void usage(void);
void help(void);

char program_name[128];

/**
 * @fn    main(int argc, char **argv)
 * @brief Main function for Waimea window manager
 *
 * Parses command line options. Creates a new waimea object.
 * Enters the eventloop.
 *
 * @param argc Number of command line arguments
 * @param argv Vector with command line arguments
 * @return     0 on successful execution else >0
 */
int main(int argc, char **argv) {
    struct waoptions options;
    sprintf(program_name, "%s", argv[0]);
    int i;
    XEvent e;
    char *__m_wastrdup_tmp;

    options.menufile = options.actionfile = options.stylefile =
        options.rcfile = options.display = NULL;

    for (i = 1; i < argc; i++) {
        if (! strcmp(argv[i], "--display")) {
            if (i + 1 < argc) options.display = argv[i++ + 1];
            else { cerr << program_name << ": option `" <<
                       argv[i] << "' requires an argument" << endl; return 1; }
        } else if (! strncmp(argv[i], "--display=", 10) &&
                   strlen(argv[i]) >= 11) { options.display = argv[i] + 10;
        } else if (! strcmp(argv[i], "--rcfile")) {
            if (i + 1 < argc) options.rcfile = __m_wastrdup(argv[i++ + 1]);
            else { cerr << program_name << ": option `" <<
                       argv[i] << "' requires an argument" << endl; return 1; }
        } else if (! strncmp(argv[i], "--rcfile=", 9) &&
                   strlen(argv[i]) >= 10) {
            options.rcfile = __m_wastrdup(argv[i] + 9);
        } else if (! strcmp(argv[i], "--stylefile")) {
            if (i + 1 < argc) options.stylefile = __m_wastrdup(argv[i++ + 1]);
            else { cerr << program_name << ": option `" <<
                       argv[i] << "' requires an argument" << endl; return 1; }
        } else if (! strncmp(argv[i], "--stylefile=", 12) &&
                   strlen(argv[i]) >= 13) {
            options.stylefile = __m_wastrdup(argv[i] + 12);
        } else if (! strcmp(argv[i], "--actionfile")) {
            if (i + 1 < argc) options.actionfile = __m_wastrdup(argv[i++ + 1]);
            else { cerr << program_name << ": option `" <<
                       argv[i] << "' requires an argument" << endl; return 1; }
        } else if (! strncmp(argv[i], "--actionfile=", 13) &&
                   strlen(argv[i]) >= 14) {
            options.actionfile = __m_wastrdup(argv[i] + 13);
        } else if (! strcmp(argv[i], "--menufile")) {
            if (i + 1 < argc) options.menufile = __m_wastrdup(argv[i++ + 1]);
            else { cerr << program_name << ": option `" <<
                       argv[i] << "' requires an argument" << endl; return 1; }
        } else if (! strncmp(argv[i], "--menufile=", 11) &&
                   strlen(argv[i]) >= 12) {
            options.menufile = __m_wastrdup(argv[i] + 11);
        } else if (! strcmp(argv[i], "--usage")) {
            usage(); return 0;
        } else if (! strcmp(argv[i], "--help")) {
            help(); return 0;
        } else if (! strcmp(argv[i], "--version")) {
            cout << PACKAGE << " " << VERSION << endl; return 0;
        } else {
            cerr << program_name << ": unrecognized option `" <<
                argv[i] << "'" << endl; usage(); return 1;
        }
    }
    
    Waimea *waimea = new Waimea(argv, &options);
    waimea->eh->EventLoop(waimea->eh->empty_return_mask, &e);
    
    return 1;
}

/** 
 * @fn    usage(void)
 * @brief Display brief usage message
 *
 * Prints brief usage message on standard out.
 * 
 */
void usage(void) {
    cout << "Usage: " << program_name << " [--display=DISPLAYNAME]" <<
        " [--rcfile=CONFIGFILE]" << endl << "\t[--stylefile=STYLEFILE]" <<
        " [--actionfile=ACTIONFILE]" << " [--menufile=MENUFILE]" << endl <<
        "\t[--usage]" << " [--help]" << " [--version]" << endl;
}

/** 
 * @fn    help(void)
 * @brief Shows help message
 *
 * Prints full command line help message to standard out.
 * 
 */
void help(void) {
    cout << "Usage: " << program_name << " [OPTION...]" << endl;
    cout << "Waimea - an X11 window manager designed for maximum efficiency" <<
        endl << endl;
    cout << "   --display=DISPLAYNAME    X server to contact" << endl;
    cout << "   --rcfile=CONFIGFILE      Config-file to use" << endl;
    cout << "   --stylefile=STYLEFILE    Style-file to use" << endl;
    cout << "   --actionfile=ACTIONFILE  Action-file to use" << endl;
    cout << "   --menufile=MENUFILE      Menu-file to use" << endl;
    cout << "   --usage                  Display brief usage message" << endl;
    cout << "   --help                   Show this help message" << endl;
    cout << "   --version                Output version information and exit"
         << endl << endl;
    cout << "Report bugs to <c99drn@cs.umu.se>." << endl;
}
