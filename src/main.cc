/** -*- Mode: C++ -*-
 *
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

#include <stdio.h>
#include <getopt.h>
#include <hash_set>

#include "Waimea.hh"

static struct option const long_options[] = {
  {"display", required_argument, NULL, 'd'},
  {"rcfile", required_argument, NULL, 'r'},
  {"stylefile", required_argument, NULL, 's'},
  {"actionfile", required_argument, NULL, 'a'},
  {"menufile", required_argument, NULL, 'm'},
  {"usage", no_argument, NULL, 'u'},
  {"help", no_argument, NULL, 'h'},
  {"version", no_argument, NULL, 'v'},
  {NULL, 0, NULL, 0}
};

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
    int opt;
    struct waoptions options;
    sprintf(program_name, "%s", argv[0]);

    options.menufile = options.actionfile = options.stylefile =
        options.rcfile = options.display = NULL;
    
    while ((opt = getopt_long(argc, argv, "", long_options, NULL)) != -1) {
        switch (opt) {
            case 'd':
                options.display = optarg;
                break;
            case 'r':
                options.rcfile = optarg;
                break;
            case 'a':
                options.actionfile = optarg;
                break;
            case 's':
                options.stylefile = optarg;
                break;
            case 'm':
                options.menufile = optarg;
                break;
            case 'u':
                usage();
                return 0 ;
            case 'h':
                help();
                return 0;
            case 'v':
                cout << PACKAGE << " " << VERSION << endl;
                return 0;
            default:
                usage();
                return 1;
        }
    }
    Waimea *waimea = new Waimea(argv, &options);
    waimea->eh->EventLoop(new hash_set<int>);
    
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
    cout << "Usage: " << program_name << " [--display=DISPLAY]" <<
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
    cout << "Waimea Window Manager - an X11 Window Manager" << endl << endl;
    cout << "   --display=DISPLAY        X display to use" << endl;
    cout << "   --rcfile=CONFIGFILE      Configfile to use" << endl;
    cout << "   --stylefile=STYLEFILE    Stylefile to use" << endl;
    cout << "   --actionfile=ACTIONFILE  Actionfile to use" << endl;
    cout << "   --menufile=MENUFILE      Menufile to use" << endl;
    cout << "   --usage                  Display brief usage message" << endl;
    cout << "   --help                   Show this help message" << endl;
    cout << "   --version                Output version information and exit" <<
        endl << endl;
    cout << "Report bugs to <c99drn@cs.umu.se>." << endl;
}
