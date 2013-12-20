/*****************************************************************************
 *   Copyright 2013 - 2013 Yichao Yu <yyc1992@gmail.com>                     *
 *                                                                           *
 *   This program is free software; you can redistribute it and/or modify    *
 *   it under the terms of the GNU Lesser General Public License as          *
 *   published by the Free Software Foundation; either version 2.1 of the    *
 *   License, or (at your option) version 3, or any later version accepted   *
 *   by the membership of KDE e.V. (or its successor approved by the         *
 *   membership of KDE e.V.), which shall act as a proxy defined in          *
 *   Section 6 of version 3 of the license.                                  *
 *                                                                           *
 *   This program is distributed in the hope that it will be useful,         *
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of          *
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU       *
 *   Lesser General Public License for more details.                         *
 *                                                                           *
 *   You should have received a copy of the GNU Lesser General Public        *
 *   License along with this library. If not,                                *
 *   see <http://www.gnu.org/licenses/>.                                     *
 *****************************************************************************/

#ifndef _QTC_UTILS_MACROS_H_
#define _QTC_UTILS_MACROS_H_

/**
 * Macros for detecting empty arguments, used to implement function overloading
 * and default arguments in c.
 * The idea of this implementation is borrowed from the following URL
 * https://gustedt.wordpress.com/2010/06/08/detect-empty-macro-arguments/
 * and is modified in order to fit with our usage.
 **/
#define __QTC_USE_3(_1, _2, _3, ...) _3
// Test if args has one comma
#define __QTC_HAS_COMMA1(ret_true, ret_false, args...)  \
    __QTC_USE_3(args, ret_true, ret_false)
// Convert parentheses to comma, used to check if the next character is "("
#define __QTC_CONVERT_PAREN(...) ,
// Check if arg starts with "("
#define __QTC_IS_PAREN_(ret_true, ret_false, arg)                       \
    __QTC_HAS_COMMA1(ret_true, ret_false, __QTC_CONVERT_PAREN arg)
// Extra layer just to make sure more evaluation (if any) is done than the
// seperator path.
#define __QTC_IS_PAREN(ret_true, ret_false, arg)        \
    __QTC_IS_PAREN_(ret_true, ret_false, arg)
// Check if arg is not empty and does not start with "("
// Will not work if arg has comma or is the name of a function like macro
#define __QTC_IS_SEP(ret_true, ret_false, arg)                          \
    __QTC_HAS_COMMA1(ret_false, ret_true, __QTC_CONVERT_PAREN arg ())
#define __QTC_IS_EMPTY_PAREN_TRUE(ret_true, ret_false, arg) ret_false
#define __QTC_IS_EMPTY_PAREN_FALSE(ret_true, ret_false, arg)    \
    __QTC_IS_SEP(ret_false, ret_true, arg)
/**
 * Test if @arg is empty, evaluate to @ret_true if is empty,
 * evaluate to @ret_false otherwise. NOTE, this may not work if arg is a macro
 * that can be evaluated to a comma separate list without parentheses or is
 * the name of a function like macro.
 **/
#define QTC_SWITCH(arg, ret_true, ret_false)                            \
    __QTC_IS_PAREN(__QTC_IS_EMPTY_PAREN_TRUE, __QTC_IS_EMPTY_PAREN_FALSE, arg) \
    (ret_false, ret_true, arg)
/**
 * Evaluate to @def if @v is empty and to @v otherwise. See #QTC_SWITCH() for
 * restrictions.
 **/
#define QTC_DEFAULT(v, def) QTC_SWITCH(v, v, def)
/**
 * Evaluate to _##@f if @v is empty and to @f otherwise. See #QTC_SWITCH() for
 * restrictions.
 **/
#define QTC_SWITCH_(v, f) QTC_SWITCH(v, f, _##f)
#define QTC_EXPORT __attribute__((visibility("default")))

// For public c headers
#ifdef __cplusplus
#  define QTC_BEGIN_DECLS extern "C" {
#  define QTC_END_DECLS }
#else
#  define QTC_BEGIN_DECLS
#  define QTC_END_DECLS
#endif

// For small functions
#define QTC_ALWAYS_INLINE __attribute__((always_inline))

// Suppress unused parameter warning.
#define QTC_UNUSED(x) ((void)(x))

#endif