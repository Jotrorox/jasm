#include "error.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "color_utils.h"

static int error_count = 0;
static int fatal_error_count = 0;

void error_init(void)
{
    error_count = 0;
    fatal_error_count = 0;
}

static const char *get_severity_string(ErrorSeverity severity)
{
    switch (severity) {
        case ERROR_SEVERITY_FATAL:
            return "fatal";
        case ERROR_SEVERITY_ERROR:
            return "error";
        case ERROR_SEVERITY_WARNING:
            return "warning";
        case ERROR_SEVERITY_INFO:
            return "info";
        default:
            return "unknown";
    }
}

static const char *get_severity_color(ErrorSeverity severity)
{
    switch (severity) {
        case ERROR_SEVERITY_FATAL:
            return COLOR_RED;
        case ERROR_SEVERITY_ERROR:
            return COLOR_RED;
        case ERROR_SEVERITY_WARNING:
            return COLOR_YELLOW;
        case ERROR_SEVERITY_INFO:
            return COLOR_CYAN;
        default:
            return COLOR_RESET;
    }
}

void error_report(const char *filename,
                  int line_number,
                  int column,
                  const char *line_content,
                  ErrorSeverity severity,
                  const char *format,
                  ...)
{
    va_list args;
    va_start(args, format);

    // Update error counts
    if (severity == ERROR_SEVERITY_FATAL) {
        fatal_error_count++;
    } else if (severity == ERROR_SEVERITY_ERROR) {
        error_count++;
    }

    // Print the error location
    color_printf(COLOR_BOLD, "%s:%d:%d: ", filename, line_number, column);

    // Print the severity
    const char *severity_str = get_severity_string(severity);
    const char *severity_color = get_severity_color(severity);
    color_printf(severity_color, "%s: ", severity_str);

    // Print the message
    vprintf(format, args);
    printf("\n");

    // Print the line content if available
    if (line_content) {
        printf("    %s\n", line_content);

        // Print the caret indicator
        if (column > 0) {
            printf("    ");
            for (int i = 0; i < column - 1; i++) {
                printf(" ");
            }
            printf("^\n");
        }
    }

    va_end(args);
}

void error_report_simple(ErrorSeverity severity, const char *format, ...)
{
    va_list args;
    va_start(args, format);

    // Update error counts
    if (severity == ERROR_SEVERITY_FATAL) {
        fatal_error_count++;
    } else if (severity == ERROR_SEVERITY_ERROR) {
        error_count++;
    }

    // Print the severity
    const char *severity_str = get_severity_string(severity);
    const char *severity_color = get_severity_color(severity);
    color_printf(severity_color, "%s: ", severity_str);

    // Print the message
    vprintf(format, args);
    printf("\n");

    va_end(args);
}

int error_get_count(void)
{
    return error_count;
}

bool error_has_errors(void)
{
    return error_count > 0 || fatal_error_count > 0;
}

bool error_has_fatal_errors(void)
{
    return fatal_error_count > 0;
}