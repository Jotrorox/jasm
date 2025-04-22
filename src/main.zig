const std = @import("std");
const cli_parser = @import("cli_parser.zig");
const ansi = @import("ansi_styles.zig");

const name = "jasm";
const version = "0.2.0";
const copyright = "Copyright (c) 2025 Johannes (jotrorox) Müller";
const author = "Johannes (jotrorox) Müller";
const license = @embedFile("../LICENSE");

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
        std.debug.print("Run '{s}{s}{s} --help' for usage information.\n", .{
            style_prog, name, reset,
        });
        std.process.exit(1);
    }

    if (cli_args.positional.items.len > 1) {
        std.debug.print("{s}Error:{s} {s}Too many input files provided. Expected 1.{s}\n", .{
            style_error, reset, style_err_msg, reset,
        });
        std.process.exit(1);
    }

    const input_filename = cli_args.positional.items[0];

    // --- Read file and count lines ---
    const file = std.fs.cwd().openFile(input_filename, .{}) catch |err| {
        std.debug.print("{s}Error:{s} {s}Could not open file '{s}{s}{s}': {any}{s}\n", .{
            style_error, // 1st {s}
            reset, // 2nd {s}
            style_err_msg, // 3rd {s} (For "Could not open...")
            style_file, // 4th {s} (Style for filename)
            input_filename, // 5th {s} (The filename itself)
            reset ++ style_err_msg, // 6th {s} (Reset style, then error style for ':')
            err, // {any} (The actual error value)
            reset, // 7th {s} (Final reset)
        });
        std.process.exit(1);
    };
    defer file.close();

    var line_count: usize = 0;
    var buf_reader = std.io.bufferedReader(file.reader());
    const reader = buf_reader.reader();
    const buffer_size = 4096; // Adjust buffer size as needed
    var buffer: [buffer_size]u8 = undefined;

    while (try reader.readUntilDelimiterOrEof(&buffer, '\n')) |line| {
        _ = line; // We only care about the count, not the content
        line_count += 1;
    }

    // Print the line count as the output just for testing
    std.debug.print("{d}\n", .{line_count});
}
