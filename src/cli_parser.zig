const std = @import("std");

/// Represents a single parsed command-line option.
pub const Option = struct {
    /// The name of the option (e.g., "name", "v"). Does not include leading dashes.
    name: []const u8,
    /// The value associated with the option. Empty string if no value is provided.
    value: []const u8,
};

/// Parses command-line arguments into options and positional arguments.
pub const CliArgs = struct {
    /// A list of parsed options.
    options: std.ArrayList(Option),
    /// A list of positional arguments in the order they appear.
    positional: std.ArrayList([]const u8),

    /// Initializes a new CliArgs struct.
    ///
    /// Requires an allocator for dynamic memory allocation.
    pub fn init(allocator: std.mem.Allocator) CliArgs {
        return CliArgs{
            .options = std.ArrayList(Option).init(allocator),
            .positional = std.ArrayList([]const u8).init(allocator),
        };
    }

    /// Deinitializes the CliArgs struct, freeing all allocated memory.
    pub fn deinit(self: *CliArgs) void {
        // Free memory for option names and values
        for (self.options.items) |option| {
            self.options.allocator.free(option.name);
            self.options.allocator.free(option.value);
        }
        self.options.deinit();

        // Free memory for positional arguments
        for (self.positional.items) |arg| {
            self.positional.allocator.free(arg);
        }
        self.positional.deinit();
    }

    /// Parses command-line arguments from an ArgIterator.
    ///
    /// Skips the first argument (program name).
    /// Handles both short (`-o`) and long (`--option`) options,
    /// including options with values (`-ovalue`, `--option=value`).
    /// Stores option names *without* leading dashes.
    pub fn parse(self: *CliArgs, args_iterator: *std.process.ArgIterator) !void {
        // Get the first argument (program name) and discard it for parsing purposes
        _ = args_iterator.next().?;

        while (args_iterator.next()) |arg| {
            if (arg.len > 1 and arg[0] == '-') {
                // It's an option
                if (arg[1] == '-') {
                    // Long option (--option or --option=value)
                    var parts = std.mem.splitScalar(u8, arg, '=');
                    const raw_option_name_slice = parts.next().?;
                    const option_value_slice = parts.next() orelse "";

                    // Strip leading '--' for storing the name
                    const option_name_slice = if (raw_option_name_slice.len > 2 and raw_option_name_slice[0] == '-' and raw_option_name_slice[1] == '-') raw_option_name_slice[2..] else raw_option_name_slice;

                    const name_copy = try self.options.allocator.dupe(u8, option_name_slice);
                    const value_copy = try self.options.allocator.dupe(u8, option_value_slice);

                    try self.options.append(Option{ .name = name_copy, .value = value_copy });

                } else {
                    // Short option (-o or -ovalue)
                    // We still need to handle potential chained short options like -abc,
                    // but for this simple parser, we'll just take the first character after '-'
                    // and the rest as value if present immediately after.
                    const option_name_slice = arg[1..2]; // Take the first character after '-'
                    const option_value_slice = if (arg.len > 2) arg[2..] else "";

                    const name_copy = try self.options.allocator.dupe(u8, option_name_slice);
                    const value_copy = try self.options.allocator.dupe(u8, option_value_slice);

                    try self.options.append(Option{ .name = name_copy, .value = value_copy });
                }
            } else {
                // It's a positional argument
                const arg_copy = try self.positional.addOne();
                arg_copy.* = try self.positional.allocator.dupe(u8, arg);
            }
        }
    }

    /// Retrieves the value of an option by its name.
    ///
    /// Performs a linear search through the parsed options.
    /// Returns a slice of the option's value if found, otherwise returns null.
    /// Searches for option names *without* leading dashes.
    pub fn getOption(self: *CliArgs, name: []const u8) ?[]const u8 {
        for (self.options.items) |option| {
            if (std.mem.eql(u8, option.name, name)) {
                return option.value;
            }
        }
        return null;
    }
};
