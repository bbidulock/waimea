/**
 * @file   Regex.hh
 * @author David Reveman <c99drn@cs.umu.se>
 * @date   4-Oct-2001 22:24:11
 *
 * @brief Definition of Regex class  
 *
 * Function declarations and variable definitions for Regex class.
 *
 * Copyright (C) David Reveman. All rights reserved.
 *
 */

#ifndef __Regex_hh
#define __Regex_hh

extern "C" {
#ifdef    HAVE_STDIO_H
#  include <stdio.h>
#endif // HAVE_STDIO_H

#ifdef    HAVE_SYS_TYPES_H
#  include <sys/types.h>
#endif // HAVE_SYS_TYPES_H

#ifdef    HAVE_REGEX_H
#  include <regex.h>
#endif // HAVE_REGEX_H
}

class Regex {
public:
    Regex(char *);
    ~Regex(void) { if (comp_ok) regfree(&regexp); }

    bool Match(char *);

    regex_t regexp;
    bool comp_ok;
};

#endif // __Regex_hh
