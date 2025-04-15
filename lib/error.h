#ifndef ERROR_H
#define ERROR_H

#include <stdarg.h>
#include <stdbool.h>

typedef enum {
    ERROR_SEVERITY_FATAL,    // Must stop compilation
    ERROR_SEVERITY_ERROR,    // Will fail but can continue
    ERROR_SEVERITY_WARNING,  // Might succeed
    ERROR_SEVERITY_INFO      // Informational
} ErrorSeverity;

typedef struct {
    const char *filename;
    int line_number;
    int column;
    const char *line_content;
    ErrorSeverity severity;
    const char *message;
} ErrorContext;

// Initialize error handling
void error_init(void);

// Report an error with context
void error_report(const char *filename,
                  int line_number,
                  int column,
                  const char *line_content,
                  ErrorSeverity severity,
                  const char *format,
                  ...);

// Report an error without context
void error_report_simple(ErrorSeverity severity, const char *format, ...);

// Get the current error count
int error_get_count(void);

// Check if there were any errors
bool error_has_errors(void);

// Check if there were any fatal errors
bool error_has_fatal_errors(void);

#endif  // ERROR_H