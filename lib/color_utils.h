/**
 * color_utils.h - Terminal color utilities for jasm
 * 
 * Provides ANSI color code definitions and helper functions
 * for colorizing terminal output.
 */

#ifndef COLOR_UTILS_H
#define COLOR_UTILS_H

#include <stdio.h>
#include <stdarg.h>
#include <stdbool.h>

/* ANSI Color Codes */
#define COLOR_RESET     "\033[0m"
#define COLOR_BOLD      "\033[1m"
#define COLOR_DIM       "\033[2m"
#define COLOR_ITALIC    "\033[3m"
#define COLOR_UNDERLINE "\033[4m"

/* Foreground Colors */
#define COLOR_BLACK     "\033[30m"
#define COLOR_RED       "\033[31m"
#define COLOR_GREEN     "\033[32m"
#define COLOR_YELLOW    "\033[33m"
#define COLOR_BLUE      "\033[34m"
#define COLOR_MAGENTA   "\033[35m"
#define COLOR_CYAN      "\033[36m"
#define COLOR_WHITE     "\033[37m"

/* Bright Foreground Colors */
#define COLOR_BRIGHT_BLACK   "\033[90m"
#define COLOR_BRIGHT_RED     "\033[91m"
#define COLOR_BRIGHT_GREEN   "\033[92m"
#define COLOR_BRIGHT_YELLOW  "\033[93m"
#define COLOR_BRIGHT_BLUE    "\033[94m"
#define COLOR_BRIGHT_MAGENTA "\033[95m"
#define COLOR_BRIGHT_CYAN    "\033[96m"
#define COLOR_BRIGHT_WHITE   "\033[97m"

/* Background Colors */
#define COLOR_BG_BLACK   "\033[40m"
#define COLOR_BG_RED     "\033[41m"
#define COLOR_BG_GREEN   "\033[42m"
#define COLOR_BG_YELLOW  "\033[43m"
#define COLOR_BG_BLUE    "\033[44m"
#define COLOR_BG_MAGENTA "\033[45m"
#define COLOR_BG_CYAN    "\033[46m"
#define COLOR_BG_WHITE   "\033[47m"

/**
 * Global flag to enable/disable colorized output.
 * Set to false to disable colors (e.g., when output is redirected to a file).
 */
extern bool use_colors;

/**
 * Initialize color utilities.
 * Detects if colors should be enabled based on terminal capabilities.
 */
void color_init(void);

/**
 * Print a colorized message to stdout.
 * 
 * @param color ANSI color code to use
 * @param format printf-style format string
 * @param ... arguments for format string
 */
void color_printf(const char* color, const char* format, ...);

/**
 * Print a colorized error message to stderr.
 * 
 * @param format printf-style format string
 * @param ... arguments for format string
 */
void color_error(const char* format, ...);

/**
 * Print a colorized warning message to stderr.
 * 
 * @param format printf-style format string
 * @param ... arguments for format string
 */
void color_warning(const char* format, ...);

/**
 * Print a colorized success message to stdout.
 * 
 * @param format printf-style format string
 * @param ... arguments for format string
 */
void color_success(const char* format, ...);

/**
 * Print a colorized info message to stdout.
 * 
 * @param format printf-style format string
 * @param ... arguments for format string
 */
void color_info(const char* format, ...);

/**
 * Print a section header with a decorative style.
 * 
 * @param title The title of the section
 */
void color_section(const char* title);

/**
 * Colorize a string with given color and return the result.
 * Note: The returned string is statically allocated and will be
 * overwritten on subsequent calls.
 * 
 * @param color ANSI color code to use
 * @param str   The string to colorize
 * @return      Pointer to colorized string
 */
const char* color_string(const char* color, const char* str);

#endif /* COLOR_UTILS_H */ 