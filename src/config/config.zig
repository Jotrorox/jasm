pub const ProgramAuthor = struct {
    name: []const u8,
    email: []const u8,

    pub fn init(name: []const u8, email: []const u8) ProgramAuthor {
        return ProgramAuthor{
            .name = name,
            .email = email,
        };
    }
};

pub const ProgramConfig = struct {
    name: []const u8,
    version: []const u8,
    author: []const ProgramAuthor,

    pub fn init(name: []const u8, version: []const u8, author: []const ProgramAuthor) ProgramConfig {
        return ProgramConfig{
            .name = name,
            .version = version,
            .author = author,
        };
    }
};
