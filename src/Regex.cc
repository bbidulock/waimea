/**
 * @file   Regex.cc
 * @author David Reveman <david@waimea.org>
 * @date   4-Oct-2001 22:24:11
 *
 * @brief Implementation of Regex class  
 *
 * C++ wrapper for libc's regular expressions.
 *
 * Copyright (C) David Reveman. All rights reserved.
 *
 */

#ifdef    HAVE_CONFIG_H
#  include "../config.h"
#endif // HAVE_CONFIG_H

#include <iostream>
using std::cerr;
using std::cout;
using std::endl;

#include "Regex.hh"
#include "Waimea.hh"

/**
 * @fn    Regex(char *_pattern)
 * @brief Constructor for Regex class
 *
 * Compiles a regular expression that later can be used for string matching.
 *
 * @param _pattern Regular expression string
 */
Regex::Regex(char *_pattern) {
    int status = 0;
    char *err_msg = NULL;
    char *__m_wastrdup_tmp;

    if (_pattern == NULL) {
        comp_ok = false;
        return;
    }
    
    comp_ok = true;

    char *pattern = __m_wastrdup(_pattern);
    
    for (int i = 0; pattern[i] != '\0'; i++) {
        int n;
        if (pattern[i] == '\\' && pattern[i + 1] == '/') {
            for (n = 1; pattern[i + n] != '\0'; n++)
                pattern[i + n - 1] = pattern[i + n];
            pattern[i + n - 1] = '\0';
        }
    }
    if ((status = regcomp(&regexp, pattern, REG_EXTENDED | REG_NOSUB)) != 0) {
        if (status == REG_ESPACE)
            WARNING << "memory allocation error" << endl;
        else {
            int err_msg_sz = regerror(status, &regexp, NULL, (size_t) 0);
            if ((err_msg = (char *) malloc(err_msg_sz)) != NULL) {
                regerror(status, &regexp, err_msg, err_msg_sz );
                WARNING << err_msg << " = " << pattern << endl;
                free(err_msg);
            } else {
                WARNING << "invalid regular expression = " << pattern << endl;
            }
        }
        comp_ok = false;
    }

    delete [] pattern;
}

/**
 * @fn    Match(char *str)
 * @brief String matcher
 *
 * Matches string with compiled regular expression pattern. 
 *
 * @param str String to match with
 *
 * @return True if string matched, otherwise false
 */
bool Regex::Match(char *str) {
    int status = 0;
    char *err_msg = NULL;

    if (! comp_ok) return false;
    
    status = regexec(&regexp, str, (size_t) 0, NULL, 0);

    if (status == REG_NOMATCH)
        return false;
    else if (status != 0) {
        if (status == REG_ESPACE)
            WARNING << "memory allocation error" << endl;
        else {
            int err_msg_sz = regerror(status, &regexp, NULL, (size_t) 0);
            if ((err_msg = (char *) malloc(err_msg_sz)) != NULL) {
                regerror(status, &regexp, err_msg, err_msg_sz );
                WARNING << err_msg << endl;
                free(err_msg);
            } else {
                WARNING << "regexec error " << endl;
            }
        }
        return false;
    }
    return true;
}
