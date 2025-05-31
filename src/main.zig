const std = @import("std");

const cli = @import("./cli/cli.zig");
const Command = @import("./cli/command.zig").Command;
const config = @import("./config/config.zig");
const LicenseCommand = @import("./cli/commands/license.zig").LicenseCommand;
const HelpCommand = @import("./cli/commands/help.zig").HelpCommand;

const CurrentProgramConfig = config.ProgramConfig.init("JASM", "0.2.0", &[_]config.ProgramAuthor{config.ProgramAuthor.init("Johannes (Jotrorox) Müller", "jotrorox@gmail.com")});

pub fn main() !void {
    const commands = [_]Command{
        HelpCommand,
        LicenseCommand,
    };

    const cli_config = cli.CLI{
        .pConfig = CurrentProgramConfig,
        .commands = &commands,
    };
    const cli_data = try cli_config.parse();

    if (cli_data.help or cli_data.args.len == 0) {
        try HelpCommand.run(&[_][]const u8{});
    }

    if (cli_data.verbose > 0) {
        std.debug.print("Parsed Program name: {s}\n", .{cli_data.program_name});
    }
}
