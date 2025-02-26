/**
 * color_utils.c - Implementation of terminal color utilities for jasm
 */

#include "color_utils.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

/* Global flag for enabling/disabling colors */
bool use_colors = true;

/* Utility buffer for colorized strings */
#define MAX_COLORIZED_STR_LEN 1024
static char color_buffer[MAX_COLORIZED_STR_LEN];

/**
 * Initialize color utilities.
 * Detects if colors should be enabled based on terminal capabilities.
 */
void color_init(void)
{
    /* Disable colors if output is not a terminal */
    if (!isatty(STDOUT_FILENO)) {
        use_colors = false;
        return;
    }

    /* Check for NO_COLOR environment variable */
    if (getenv("NO_COLOR") != NULL) {
        use_colors = false;
        return;
    }

    /* Check for TERM=dumb */
    const char *term = getenv("TERM");
    if (term != NULL && strcmp(term, "dumb") == 0) {
        use_colors = false;
        return;
    }

    /* By default, enable colors */
    use_colors = true;
}

/**
 * Print a colorized message to stdout.
 */
void color_printf(const char *color, const char *format, ...)
{
    va_list args;
    va_start(args, format);

    if (use_colors) {
        printf("%s", color);
    }

    vprintf(format, args);

    if (use_colors) {
        printf("%s", COLOR_RESET);
    }

    va_end(args);
}

/**
 * Print a colorized error message to stderr.
 */
void color_error(const char *format, ...)
{
    va_list args;
    va_start(args, format);

    if (use_colors) {
        fprintf(stderr, "%s%sERROR:%s ", COLOR_BOLD, COLOR_RED, COLOR_RESET);
    } else {
        fprintf(stderr, "ERROR: ");
    }

    vfprintf(stderr, format, args);
    fprintf(stderr, "\n");

    va_end(args);
}

/**
 * Print a colorized warning message to stderr.
 */
void color_warning(const char *format, ...)
{
    va_list args;
    va_start(args, format);

    if (use_colors) {
        fprintf(stderr, "%s%sWARNING:%s ", COLOR_BOLD, COLOR_YELLOW, COLOR_RESET);
    } else {
        fprintf(stderr, "WARNING: ");
    }

    vfprintf(stderr, format, args);
    fprintf(stderr, "\n");

    va_end(args);
}

/**
 * Print a colorized success message to stdout.
 */
void color_success(const char *format, ...)
{
    va_list args;
    va_start(args, format);

    if (use_colors) {
        printf("%s%sSUCCESS:%s ", COLOR_BOLD, COLOR_GREEN, COLOR_RESET);
    } else {
        printf("SUCCESS: ");
    }

    vprintf(format, args);
    printf("\n");

    va_end(args);
}

/**
 * Print a colorized info message to stdout.
 */
void color_info(const char *format, ...)
{
    va_list args;
    va_start(args, format);

    if (use_colors) {
        printf("%s", COLOR_CYAN);
    }

    vprintf(format, args);

    if (use_colors) {
        printf("%s", COLOR_RESET);
    }

    printf("\n");

    va_end(args);
}

/**
 * Print a section header with a decorative style.
 */
void color_section(const char *title)
{
    if (use_colors) {
        printf("\n%s%s=== %s ===%s\n\n", COLOR_BOLD, COLOR_BRIGHT_BLUE, title, COLOR_RESET);
    } else {
        printf("\n=== %s ===\n\n", title);
    }
}

/**
 * Colorize a string with given color and return the result.
 */
const char *color_string(const char *color, const char *str)
{
    if (!use_colors || !str) {
        return str;
    }

    snprintf(color_buffer, MAX_COLORIZED_STR_LEN, "%s%s%s", color, str, COLOR_RESET);

    return color_buffer;
}