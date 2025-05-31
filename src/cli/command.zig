pub const CLIArgs = struct {
    long: []const u8,
    short: []const u8,
};

pub const Command = struct {
    cli_args: CLIArgs,
    name: []const u8,
    description: []const u8,
    run: *const fn (args: []const []const u8) anyerror!void,
};
