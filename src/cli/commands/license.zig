const Command = @import("../command.zig").Command;
const std = @import("std");

fn run(_: []const []const u8) anyerror!void {
    const stdout = std.io.getStdOut().writer();
    try stdout.print("JASM License Information:\n", .{});
    try stdout.print("License: MIT Copyright 2025 Johannes (jotrorox) Müller\n", .{});
    try stdout.print("For more information, visit: https://github.com/Jotrorox/jasm\n", .{});
}

pub const LicenseCommand = Command{
    .cli_args = .{
        .long = "license",
        .short = "l",
    },
    .name = "license",
    .description = "Prints the license of the program.",
    .run = run,
};
