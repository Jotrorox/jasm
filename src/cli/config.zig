pub const CliConfig = struct {
    program_name: []const u8,
    verbose: i8, // -1 = quiet, 0 = normal, 1 = verbose, 2 = debug
    help: bool,
    args: []const []const u8,

    pub fn init(program_name: []const u8, verbose: i8, help: bool, args: []const []const u8) CliConfig {
        return CliConfig{
            .program_name = program_name,
            .verbose = verbose,
            .help = help,
            .args = args,
        };
    }

    pub fn initDefault(program_name: []const u8) CliConfig {
        return CliConfig{
            .program_name = program_name,
            .verbose = 0,
            .help = false,
            .args = &[_][]const u8{},
        };
    }
};
