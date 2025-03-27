#define NOB_IMPLEMENTATION
#include "nob.h"

#define VERSION "0.1"
#define AUTHOR "Johannes (Jotrorox) Müller"
#define COPYRIGHT "2025 " AUTHOR

// Build configuration
typedef struct {
    const char *build_type;
    bool verbose;
    const char *output_dir;
    bool clean;
    bool install;
    const char *prefix;
    bool uninstall;
} BuildConfig;

// Compiler flags for different build types
static void append_compiler_flags(Nob_Cmd *cmd, const char *build_type) {
    if (strcmp(build_type, "Debug") == 0) {
        nob_cmd_append(cmd, "-g", "-fsanitize=address", "-fno-omit-frame-pointer", 
                      "-fno-inline", "-fno-strict-aliasing", "-fno-lto", "-O0");
    } else if (strcmp(build_type, "Release") == 0) {
        nob_cmd_append(cmd, "-DNDEBUG", "-O3", "-fomit-frame-pointer", 
                      "-finline-functions", "-fstrict-aliasing", "-flto", 
                      "-march=native", "-funroll-loops");
    } else if (strcmp(build_type, "Verbose") == 0) {
        nob_cmd_append(cmd, "-DVERBOSE", "-O2", "-Wall", "-Wextra");
    } else {
        nob_cmd_append(cmd, "-Wall", "-Wextra"); // Default flags
    }
}

// Get all source files from src directory
static bool get_source_files(Nob_File_Paths *sources) {
    Nob_File_Paths temp = {0};
    if (!nob_read_entire_dir("src", &temp)) return false;
    
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
static bool clean_build_dir(const char *output_dir) {
    Nob_File_Paths files = {0};
    if (!nob_read_entire_dir(output_dir, &files)) return false;
    
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
            nob_sb_free(path);
            nob_da_free(files);
            return false;
        }
        
        nob_sb_free(path);
    }
    
    nob_da_free(files);
    return true;
}

// Compile a single source file
static bool compile_source_file(const char *src_file, const char *obj_file, const char *build_type, bool verbose) {
    Nob_Cmd cmd = {0};
    nob_cmd_append(&cmd, "cc", "-c", "-Ilib");
    append_compiler_flags(&cmd, build_type);
    nob_cmd_append(&cmd, "-o", obj_file, src_file);
    
    if (verbose) {
        Nob_String_Builder sb = {0};
        nob_cmd_render(cmd, &sb);
        nob_sb_append_null(&sb);
        nob_log(NOB_INFO, "CMD: %s", sb.items);
        nob_sb_free(sb);
    }
    
    return nob_cmd_run_sync_and_reset(&cmd);
}

// Link all object files into the final executable
static bool link_executable(const char *output_dir, Nob_File_Paths *obj_files, const char *build_type, bool verbose) {
    Nob_Cmd cmd = {0};
    nob_cmd_append(&cmd, "cc");
    
    // Add ASAN flags only in Debug mode
    if (strcmp(build_type, "Debug") == 0) {
        nob_cmd_append(&cmd, "-fsanitize=address");
    }
    
    // Add all object files
    for (size_t i = 0; i < obj_files->count; ++i) {
        nob_cmd_append(&cmd, obj_files->items[i]);
    }
    
    // Add output executable
    nob_cmd_append(&cmd, "-o", "jasm");
    
    if (verbose) {
        Nob_String_Builder sb = {0};
        nob_cmd_render(cmd, &sb);
        nob_sb_append_null(&sb);
        nob_log(NOB_INFO, "CMD: %s", sb.items);
        nob_sb_free(sb);
    }
    
    return nob_cmd_run_sync_and_reset(&cmd);
}

// Install the executable
static bool install_executable(const char *prefix) {
    Nob_Cmd cmd = {0};
    nob_cmd_append(&cmd, "install", "-D", "-m", "755", "jasm", prefix);
    
    if (!nob_cmd_run_sync_and_reset(&cmd)) {
        nob_log(NOB_ERROR, "Failed to install jasm");
        return false;
    }
    
    nob_log(NOB_INFO, "Installed jasm to %s", prefix);
    return true;
}

// Uninstall the executable
static bool uninstall_executable(const char *prefix) {
    Nob_Cmd cmd = {0};
    nob_cmd_append(&cmd, "rm", "-f", prefix);
    
    if (!nob_cmd_run_sync_and_reset(&cmd)) {
        nob_log(NOB_ERROR, "Failed to uninstall jasm");
        return false;
    }
    
    nob_log(NOB_INFO, "Uninstalled jasm from %s", prefix);
    return true;
}

// Main build function
static bool build_project(const BuildConfig *config) {
    bool result = true;
    Nob_File_Paths sources = {0};
    Nob_File_Paths obj_files = {0};
    
    // Create output directory
    if (!nob_mkdir_if_not_exists(config->output_dir)) {
        nob_return_defer(false);
    }
    
    // Clean build directory if requested
    if (config->clean) {
        if (!clean_build_dir(config->output_dir)) {
            nob_return_defer(false);
        }
        nob_log(NOB_INFO, "Cleaned build directory");
        if (config->clean) { // If only cleaning was requested
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
        if (!nob_sv_end_with(nob_sv_from_cstr(src_file), ".c")) continue;
        
        // Create object file path
        Nob_String_Builder obj_path = {0};
        nob_sb_append_cstr(&obj_path, config->output_dir);
        nob_sb_append_cstr(&obj_path, "/");
        nob_sb_append_cstr(&obj_path, nob_path_name(src_file));
        nob_sb_append_cstr(&obj_path, ".o");
        nob_sb_append_null(&obj_path);
        
        if (!compile_source_file(src_file, obj_path.items, config->build_type, config->verbose)) {
            nob_sb_free(obj_path);
            nob_return_defer(false);
        }
        
        nob_da_append(&obj_files, nob_temp_strdup(obj_path.items));
        nob_sb_free(obj_path);
    }
    
    // Link the executable
    if (!link_executable(config->output_dir, &obj_files, config->build_type, config->verbose)) {
        nob_return_defer(false);
    }
    
    // Install if requested
    if (config->install) {
        if (!install_executable(config->prefix)) {
            nob_return_defer(false);
        }
    }
    
    // Uninstall if requested
    if (config->uninstall) {
        if (!uninstall_executable(config->prefix)) {
            nob_return_defer(false);
        }
    }
    
defer:
    nob_da_free(sources);
    nob_da_free(obj_files);
    return result;
}

// Print help message
static void print_help(const char *program) {
    nob_log(NOB_INFO, "JASM Assembler v%s", VERSION);
    nob_log(NOB_INFO, "Copyright (c) %s", COPYRIGHT);
    fprintf(stderr, "\n");
    nob_log(NOB_INFO, "Usage: %s [options]", program);
    fprintf(stderr, "\n");
    nob_log(NOB_INFO, "Build Options:");
    nob_log(NOB_INFO, "  --type <type>    Build type (Debug, Release, Verbose)");
    nob_log(NOB_INFO, "  --output <dir>   Output directory (default: build)");
    nob_log(NOB_INFO, "  --verbose        Enable verbose output");
    fprintf(stderr, "\n");
    nob_log(NOB_INFO, "Installation Options:");
    nob_log(NOB_INFO, "  --prefix <path>  Installation prefix (default: /usr/local/bin/jasm)");
    nob_log(NOB_INFO, "  --install        Install the executable");
    nob_log(NOB_INFO, "  --uninstall      Uninstall the executable");
    fprintf(stderr, "\n");
    nob_log(NOB_INFO, "Other Options:");
    nob_log(NOB_INFO, "  --clean          Clean build directory");
    nob_log(NOB_INFO, "  --help           Show this help message");
}

int main(int argc, char **argv) {
    NOB_GO_REBUILD_URSELF(argc, argv);
    
    // Default configuration
    BuildConfig config = {
        .build_type = "Debug",
        .verbose = false,
        .output_dir = "build",
        .clean = false,
        .install = false,
        .prefix = "/usr/local/bin/jasm",
        .uninstall = false
    };
    
    // Parse command line arguments
    for (int i = 1; i < argc; ++i) {
        if (strcmp(argv[i], "--type") == 0 && i + 1 < argc) {
            config.build_type = argv[++i];
        } else if (strcmp(argv[i], "--output") == 0 && i + 1 < argc) {
            config.output_dir = argv[++i];
        } else if (strcmp(argv[i], "--verbose") == 0) {
            config.verbose = true;
        } else if (strcmp(argv[i], "--clean") == 0) {
            config.clean = true;
        } else if (strcmp(argv[i], "--install") == 0) {
            config.install = true;
        } else if (strcmp(argv[i], "--uninstall") == 0) {
            config.uninstall = true;
        } else if (strcmp(argv[i], "--prefix") == 0 && i + 1 < argc) {
            config.prefix = argv[++i];
        } else if (strcmp(argv[i], "--help") == 0) {
            print_help(argv[0]);
            return 0;
        } else {
            nob_log(NOB_ERROR, "Unknown option: %s", argv[i]);
            nob_log(NOB_ERROR, "Use --help for usage information");
            return 1;
        }
    }
    
    // Print build configuration
    nob_log(NOB_INFO, "Building JASM v%s", VERSION);
    nob_log(NOB_INFO, "Copyright (c) %s", COPYRIGHT);
    nob_log(NOB_INFO, "Build type: %s", config.build_type);
    nob_log(NOB_INFO, "Output directory: %s", config.output_dir);
    
    // Build the project
    if (!build_project(&config)) {
        nob_log(NOB_ERROR, "Build failed");
        return 1;
    }
    
    nob_log(NOB_INFO, "Build completed successfully");
    return 0;
} 