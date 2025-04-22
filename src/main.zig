const std = @import("std");
const cli_parser = @import("cli_parser.zig");
const ansi = @import("ansi_styles.zig");

const name = "jasm";
const version = "0.2.0";
const copyright = "Copyright (c) 2025 Johannes (jotrorox) Müller";

pub fn main() !void {
    // Combine styles used frequently for brevity
    const style_bold = ansi.bold;
    const style_error = ansi.bold ++ ansi.fg_red; // Bold Red for Error:
    const style_err_msg = ansi.fg_red; // Red for error message text
    const style_prog = ansi.bold ++ ansi.fg_cyan; // Bold Cyan for program name
    const style_flag = ansi.fg_green; // Green for flags
    const style_version = ansi.fg_green; // Green for version number
    const style_file = ansi.fg_yellow; // Yellow for filename
    const style_dim = ansi.dim; // Dim for copyright
    const reset = ansi.reset; // Reset code

    var gpa = std.heap.GeneralPurposeAllocator(.{}){};
    const allocator = gpa.allocator();
    defer _ = gpa.deinit();

    var cli_args = cli_parser.CliArgs.init(allocator);
    defer cli_args.deinit();

    var args_iterator = try std.process.argsWithAllocator(allocator);
    defer args_iterator.deinit();

    try cli_args.parse(&args_iterator);

    if (cli_args.getOption("help")) |_| {
        std.debug.print("{s}Usage:{s} {s}{s}{s} [options] [file]\n", .{
            style_bold, reset, style_prog, name, reset,
        });
        std.debug.print("\n{s}Options:{s}\n", .{ style_bold, reset });
        std.debug.print("  {s}-h{s}, {s}--help{s}      Display this help message\n", .{
            style_flag, reset, style_flag, reset,
        });
        std.debug.print("  {s}-V{s}, {s}--version{s}   Display version information\n", .{
            style_flag, reset, style_flag, reset,
        });
        return;
    }

    if (cli_args.getOption("version")) |_| {
        std.debug.print("{s}{s}{s} {s}{s}{s}\n", .{
            style_prog, name, reset, style_version, version, reset,
        });
        std.debug.print("{s}{s}{s}\n", .{ style_dim, copyright, reset });
        return;
    }

    if (cli_args.positional.items.len == 0) {
        std.debug.print("{s}Error:{s} {s}No input file provided.{s}\n", .{
            style_error, reset, style_err_msg, reset,
        });
        std.debug.print("Run '{s} --help' for usage information.\n", .{name});
        std.process.exit(1);
    }

    // --- Placeholder for successful execution ---
    const input_file = cli_args.positional.items[0];
    std.debug.print("Processing file: {s}{s}{s}\n", .{
        style_file, input_file, reset,
    });
    std.debug.print("Processing complete.\n", .{});
}
