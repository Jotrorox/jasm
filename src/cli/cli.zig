const config = @import("config.zig");
const parser = @import("parser.zig");
const command = @import("command.zig");
const programConfig = @import("../config/config.zig");
const std = @import("std");

pub const CLI = struct {
    pConfig: programConfig.ProgramConfig,
    commands: []const command.Command,

    pub fn parse(cli: CLI) anyerror!config.CliConfig {
        var cli_alloc = std.heap.GeneralPurposeAllocator(.{}){};
        defer {
            const check = cli_alloc.deinit();
            if (check == .leak) @panic("Memory leak detected");
        }

        var p = try parser.parseProcess(cli_alloc.allocator(), .{});
        defer p.deinit();

        var c = config.CliConfig.initDefault(p.nextValue() orelse @panic("No Programname provided"));

        while (p.next()) |token| {
            switch (token) {
                .flag => |flag| {
                    if (flag.isLong("help") or flag.isShort("h")) {
                        c.help = true;
                        return c;
                    } else if (flag.isLong("verbose") or flag.isShort("v")) {
                        c.verbose += 1;
                    } else if (flag.isLong("quiet") or flag.isShort("q")) {
                        c.verbose -= 1;
                    }

                    for (cli.commands) |cmd| {
                        if (flag.isLong(cmd.cli_args.long) or flag.isShort(cmd.cli_args.short)) {
                            c.args = &[_][]const u8{cmd.name};
                            try cmd.run(c.args);
                            return c;
                        }
                    }
                },
                .arg => |arg| {
                    c.args = &[_][]const u8{arg};
                },
                .unexpected_value => @panic("Unexpected value in command line arguments"),
            }
        }

        return c;
    }
};
