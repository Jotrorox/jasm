const Command = @import("../command.zig").Command;
const std = @import("std");

fn run(_: []const []const u8) anyerror!void {
    const stdout = std.io.getStdOut().writer();
    try stdout.print("JASM - A JavaScript-like Assembly Language\n", .{});
    try stdout.print("Version: 0.2.0\n\n", .{});
    try stdout.print("Usage: jasm [OPTIONS]\n\n", .{});
    try stdout.print("Options:\n", .{});
    try stdout.print("  -h, --help      Show this help message\n", .{});
    try stdout.print("  -l, --license   Show license information\n", .{});
    try stdout.print("  -v, --verbose   Increase verbosity\n", .{});
    try stdout.print("  -q, --quiet     Decrease verbosity\n\n", .{});
    try stdout.print("For more information, visit: https://github.com/Jotrorox/jasm\n", .{});
}

pub const HelpCommand = Command{
    .cli_args = .{
        .long = "help",
        .short = "h",
    },
    .name = "help",
    .description = "Shows help information about the program.",
    .run = run,
};
