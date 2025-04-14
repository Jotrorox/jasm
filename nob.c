#define NOB_IMPLEMENTATION
#include "nob.h"

#define VERSION   "0.1"
#define AUTHOR    "Johannes (Jotrorox) Müller"
#define COPYRIGHT "2025 " AUTHOR
#define LICENSE   "MIT"

// Build configuration
typedef struct {
    const char *build_type;
    bool verbose;
    bool quiet;
    const char *output_dir;
    bool clean;
    bool static_build;
    const char *cc;
    bool force_clean;  // New option for forcing directory deletion
} BuildConfig;

// Compiler flags for different build types
static void append_compiler_flags(Nob_Cmd *cmd, const char *build_type)
{
    if (strcmp(build_type, "Debug") == 0) {
        nob_cmd_append(cmd,
                       "-g",
                       "-fsanitize=address",
                       "-fno-omit-frame-pointer",
                       "-fno-strict-overflow",
                       "-fno-lto",
                       "-O1",
                       "-D_FORTIFY_SOURCE=2");
    } else if (strcmp(build_type, "Release") == 0) {
        nob_cmd_append(cmd,
                       "-DNDEBUG",
                       "-O3",
                       "-fomit-frame-pointer",
                       "-finline-functions",
                       "-fstrict-aliasing",
                       "-flto",
                       "-march=native",
                       "-funroll-loops",
                       "-fstack-protector-strong",
                       "-fstack-clash-protection",
                       "-fcf-protection=full",
                       "-D_FORTIFY_SOURCE=2");
    } else if (strcmp(build_type, "Verbose") == 0) {
        nob_cmd_append(cmd, "-DVERBOSE", "-O2", "-Wall", "-Wextra");
    } else {
        nob_cmd_append(cmd, "-Wall", "-Wextra");
    }
}

// Get all source files from src directory
static bool get_source_files(Nob_File_Paths *sources)
{
    Nob_File_Paths temp = {0};
    if (!nob_read_entire_dir("src", &temp))
        return false;

    // Convert relative paths to full paths
    for (size_t i = 0; i < temp.count; ++i) {
        Nob_String_Builder full_path = {0};
        nob_sb_append_cstr(&full_path, "src/");
        nob_sb_append_cstr(&full_path, temp.items[i]);
        nob_sb_append_null(&full_path);
        nob_da_append(sources, nob_temp_strdup(full_path.items));
        nob_sb_free(full_path);
    }

    nob_da_free(temp);
    return true;
}

// Clean build directory
static bool clean_build_dir(const char *output_dir, bool force_clean)
{
    Nob_File_Paths files = {0};
    if (!nob_read_entire_dir(output_dir, &files))
        return false;

    bool success = true;
    for (size_t i = 0; i < files.count; ++i) {
        // Skip . and .. directory entries
        if (strcmp(files.items[i], ".") == 0 || strcmp(files.items[i], "..") == 0) {
            continue;
        }

        Nob_String_Builder path = {0};
        nob_sb_append_cstr(&path, output_dir);
        nob_sb_append_cstr(&path, "/");
        nob_sb_append_cstr(&path, files.items[i]);
        nob_sb_append_null(&path);

        if (!nob_delete_file(path.items)) {
            if (force_clean) {
                nob_sb_free(path);
                nob_da_free(files);
                return false;
            }
            success = false;
        }

        nob_sb_free(path);
    }

    nob_da_free(files);
    return success;
}

// Compile a single source file
static bool compile_source_file(const char *src_file,
                                const char *obj_file,
                                const char *build_type,
                                bool verbose,
                                bool quiet,
                                const char *cc)
{
    Nob_Cmd cmd = {0};
    nob_cmd_append(&cmd, cc, "-c", "-Ilib");
    append_compiler_flags(&cmd, build_type);
    nob_cmd_append(&cmd, "-o", obj_file, src_file);

    if (verbose && !quiet) {
        Nob_String_Builder sb = {0};
        nob_cmd_render(cmd, &sb);
        nob_sb_append_null(&sb);
        nob_log(NOB_INFO, "CMD: %s", sb.items);
        nob_sb_free(sb);
    }

    return nob_cmd_run_sync_and_reset(&cmd);
}

// Link all object files into the final executable
static bool link_executable(const char *output_dir,
                            Nob_File_Paths *obj_files,
                            const char *build_type,
                            bool verbose,
                            bool quiet,
                            bool static_build,
                            const char *cc)
{
    Nob_Cmd cmd = {0};
    nob_cmd_append(&cmd, cc);

    // Add ASAN flags only in Debug mode and if not doing static build
    if (strcmp(build_type, "Debug") == 0 && !static_build) {
        nob_cmd_append(&cmd, "-fsanitize=address");
    }

    // Add static linking flags if requested
    if (static_build) {
        nob_cmd_append(&cmd, "-static");
    }

    // Add all object files
    for (size_t i = 0; i < obj_files->count; ++i) {
        nob_cmd_append(&cmd, obj_files->items[i]);
    }

    // Add output executable
    nob_cmd_append(&cmd, "-o", "jasm");

    if (verbose && !quiet) {
        Nob_String_Builder sb = {0};
        nob_cmd_render(cmd, &sb);
        nob_sb_append_null(&sb);
        nob_log(NOB_INFO, "CMD: %s", sb.items);
        nob_sb_free(sb);
    }

    if (!nob_cmd_run_sync_and_reset(&cmd)) {
        return false;
    }

    // Strip the executable in Release mode
    if (strcmp(build_type, "Release") == 0) {
        Nob_Cmd strip_cmd = {0};
        nob_cmd_append(&strip_cmd, "strip", "jasm");
        if (verbose && !quiet) {
            Nob_String_Builder sb = {0};
            nob_cmd_render(strip_cmd, &sb);
            nob_sb_append_null(&sb);
            nob_log(NOB_INFO, "CMD: %s", sb.items);
            nob_sb_free(sb);
        }
        return nob_cmd_run_sync_and_reset(&strip_cmd);
    }

    return true;
}

// Main build function
static bool build_project(const BuildConfig *config)
{
    bool result = true;
    Nob_File_Paths sources = {0};
    Nob_File_Paths obj_files = {0};

    // Create output directory
    if (!nob_mkdir_if_not_exists(config->output_dir)) {
        nob_return_defer(false);
    }

    // Clean build directory if requested
    if (config->clean) {
        if (!clean_build_dir(config->output_dir, config->force_clean)) {
            if (config->force_clean) {
                nob_return_defer(false);
            } else if (!config->quiet) {
                nob_log(NOB_WARNING, "Some files could not be deleted");
            }
        }
        if (!config->quiet) {
            nob_log(NOB_INFO, "Cleaned build directory");
        }
        if (config->clean) {  // If only cleaning was requested
            nob_return_defer(true);
        }
    }

    // Get all source files
    if (!get_source_files(&sources)) {
        nob_return_defer(false);
    }

    // Compile each source file
    for (size_t i = 0; i < sources.count; ++i) {
        const char *src_file = sources.items[i];
        if (!nob_sv_end_with(nob_sv_from_cstr(src_file), ".c"))
            continue;

        // Create object file path
        Nob_String_Builder obj_path = {0};
        nob_sb_append_cstr(&obj_path, config->output_dir);
        nob_sb_append_cstr(&obj_path, "/");
        nob_sb_append_cstr(&obj_path, nob_path_name(src_file));
        nob_sb_append_cstr(&obj_path, ".o");
        nob_sb_append_null(&obj_path);

        if (!compile_source_file(src_file, obj_path.items, config->build_type, config->verbose, config->quiet, config->cc)) {
            nob_sb_free(obj_path);
            nob_return_defer(false);
        }

        nob_da_append(&obj_files, nob_temp_strdup(obj_path.items));
        nob_sb_free(obj_path);
    }

    // Link the executable
    if (!link_executable(config->output_dir, &obj_files, config->build_type, config->verbose, config->quiet, config->static_build, config->cc)) {
        nob_return_defer(false);
    }

defer:
    nob_da_free(sources);
    nob_da_free(obj_files);
    return result;
}

// Print help message
static void print_help(const char *program)
{
    nob_log(NOB_INFO, "JASM Assembler v%s", VERSION);
    nob_log(NOB_INFO, "Copyright (c) %s", COPYRIGHT);
    nob_log(NOB_INFO, "License: %s", LICENSE);
    fprintf(stderr, "\n");
    nob_log(NOB_INFO, "Usage: %s [options]", program);
    fprintf(stderr, "\n");
    nob_log(NOB_INFO, "Build Options:");
    nob_log(NOB_INFO, "  -t, --type <type>    Build type (Debug, Release, Verbose)");
    nob_log(NOB_INFO, "  -o, --output <dir>   Output directory (default: build)");
    nob_log(NOB_INFO, "  -v, --verbose        Enable verbose output");
    nob_log(NOB_INFO, "  -q, --quiet          Disable all output except errors");
    nob_log(NOB_INFO, "  -s, --static         Build static executable (not compatible with Debug mode)");
    nob_log(NOB_INFO, "  -c, --cc <compiler>  C compiler to use (default: cc)");
    fprintf(stderr, "\n");
    nob_log(NOB_INFO, "Other Options:");
    nob_log(NOB_INFO, "  -C, --clean          Clean build directory");
    nob_log(NOB_INFO, "  -F, --force          Force clean (delete non-empty directories)");
    nob_log(NOB_INFO, "  -h, --help           Show this help message");
}

int main(int argc, char **argv)
{
    NOB_GO_REBUILD_URSELF(argc, argv);

    // Default configuration
    BuildConfig config = {.build_type = "Debug",
                          .verbose = false,
                          .quiet = false,
                          .output_dir = "build",
                          .clean = false,
                          .static_build = false,
                          .cc = "cc",
                          .force_clean = false};

    // Parse command line arguments
    for (int i = 1; i < argc; ++i) {
        const char *arg = argv[i];
        if (strcmp(arg, "--type") == 0 || strcmp(arg, "-t") == 0) {
            if (i + 1 < argc) {
                config.build_type = argv[++i];
            } else {
                nob_log(NOB_ERROR, "Missing argument for %s", arg);
                return 1;
            }
        } else if (strcmp(arg, "--output") == 0 || strcmp(arg, "-o") == 0) {
            if (i + 1 < argc) {
                config.output_dir = argv[++i];
            } else {
                nob_log(NOB_ERROR, "Missing argument for %s", arg);
                return 1;
            }
        } else if (strcmp(arg, "--verbose") == 0 || strcmp(arg, "-v") == 0) {
            config.verbose = true;
        } else if (strcmp(arg, "--quiet") == 0 || strcmp(arg, "-q") == 0) {
            config.quiet = true;
            nob_minimal_log_level = NOB_ERROR;  // Only show errors in quiet mode
        } else if (strcmp(arg, "--clean") == 0 || strcmp(arg, "-C") == 0) {
            config.clean = true;
        } else if (strcmp(arg, "--force") == 0 || strcmp(arg, "-F") == 0) {
            config.force_clean = true;
        } else if (strcmp(arg, "--static") == 0 || strcmp(arg, "-s") == 0) {
            config.static_build = true;
        } else if (strcmp(arg, "--cc") == 0 || strcmp(arg, "-c") == 0) {
            if (i + 1 < argc) {
                config.cc = argv[++i];
            } else {
                nob_log(NOB_ERROR, "Missing argument for %s", arg);
                return 1;
            }
        } else if (strcmp(arg, "--help") == 0 || strcmp(arg, "-h") == 0) {
            print_help(argv[0]);
            return 0;
        } else {
            nob_log(NOB_ERROR, "Unknown option: %s", arg);
            nob_log(NOB_ERROR, "Use --help for usage information");
            return 1;
        }
    }

    // Check for incompatible options
    if (config.static_build && strcmp(config.build_type, "Debug") == 0) {
        nob_log(NOB_ERROR, "Static builds are not compatible with Debug mode (ASAN)");
        return 1;
    }

    // Print build configuration
    if (!config.quiet) {
        nob_minimal_log_level = NOB_INFO;  // Show all messages in non-quiet mode
        nob_log(NOB_INFO, "Building JASM v%s", VERSION);
        nob_log(NOB_INFO, "Copyright (c) %s", COPYRIGHT);
        nob_log(NOB_INFO, "Build type: %s", config.build_type);
        nob_log(NOB_INFO, "Output directory: %s", config.output_dir);
        nob_log(NOB_INFO, "Using compiler: %s", config.cc);
        if (config.static_build) {
            nob_log(NOB_INFO, "Building static executable");
        }
    }

    // Clean build directory if requested
    if (config.clean) {
        if (!clean_build_dir(config.output_dir, config.force_clean)) {
            if (config.force_clean) {
                nob_log(NOB_ERROR, "Failed to clean build directory");
                return 1;
            } else if (!config.quiet) {
                nob_log(NOB_WARNING, "Some files could not be deleted");
            }
        }
        if (!config.quiet) {
            nob_log(NOB_INFO, "Cleaned build directory");
        }
        if (config.clean) {  // If only cleaning was requested
            return 0;
        }
    }

    // Build the project
    if (!build_project(&config)) {
        nob_log(NOB_ERROR, "Build failed");
        return 1;
    }

    if (!config.quiet) {
        nob_log(NOB_INFO, "Build completed successfully");
    }
    return 0;
}