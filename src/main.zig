const std = @import("std");
const cli_parser = @import("cli_parser.zig");

pub fn main() !void {
    var gpa = std.heap.GeneralPurposeAllocator(.{}){};
    const allocator = gpa.allocator();
    defer _ = gpa.deinit();

    var cli_args = cli_parser.CliArgs.init(allocator);
    defer cli_args.deinit();

    var args_iterator = try std.process.argsWithAllocator(allocator);
    defer args_iterator.deinit();

    try cli_args.parse(&args_iterator);

    std.debug.print("Options:\n", .{});
    for (cli_args.options.items) |option| {
        // Print with correct dashes based on option name length (simple heuristic)
        const dash = if (option.name.len == 1) "-" else "--";
        std.debug.print("  {s}{s}: {s}\n", .{ dash, option.name, option.value });
    }

    std.debug.print("Positional arguments:\n", .{});
    for (cli_args.positional.items) |arg| {
        std.debug.print("  {s}\n", .{arg});
    }

    // Example of using getOption (now searching for names without leading dashes)
    if (cli_args.getOption("name")) |name| {
        std.debug.print("Found name option with value: {s}\n", .{name});
    } else {
        std.debug.print("Name option not found.\n", .{});
    }

    // Checking for a flag like "-v"
    if (cli_args.getOption("v")) |verbose_value| {
         // In our simple parser, a flag will have an empty string value if just the flag is present.
         // If a value is provided immediately after (like -vasd), it will have that value.
         // A more robust parser would handle these differently.
         std.debug.print("Verbose option found with value: '{s}'\n", .{verbose_value});
    } else {
        std.debug.print("Verbose option not found.\n", .{});
    }
}
