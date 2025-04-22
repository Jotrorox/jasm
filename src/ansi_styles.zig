pub const reset = "\x1b[0m";

// --- Text Styles ---
pub const bold = "\x1b[1m";
pub const dim = "\x1b[2m";
pub const italic = "\x1b[3m"; // Often not supported
pub const underline = "\x1b[4m";
pub const blink = "\x1b[5m"; // Often not supported or annoying
pub const reverse = "\x1b[7m"; // Swaps foreground and background
pub const hidden = "\x1b[8m"; // Concealed text
pub const strikethrough = "\x1b[9m"; // Often not supported

// --- Foreground Colors (Basic) ---
pub const fg_black = "\x1b[30m";
pub const fg_red = "\x1b[31m";
pub const fg_green = "\x1b[32m";
pub const fg_yellow = "\x1b[33m";
pub const fg_blue = "\x1b[34m";
pub const fg_magenta = "\x1b[35m";
pub const fg_cyan = "\x1b[36m";
pub const fg_white = "\x1b[37m";

// --- Foreground Colors (Bright) ---
pub const fg_bright_black = "\x1b[90m"; // Often gray
pub const fg_bright_red = "\x1b[91m";
pub const fg_bright_green = "\x1b[92m";
pub const fg_bright_yellow = "\x1b[93m";
pub const fg_bright_blue = "\x1b[94m";
pub const fg_bright_magenta = "\x1b[95m";
pub const fg_bright_cyan = "\x1b[96m";
pub const fg_bright_white = "\x1b[97m";

// --- Background Colors (Basic) ---
pub const bg_black = "\x1b[40m";
pub const bg_red = "\x1b[41m";
pub const bg_green = "\x1b[42m";
pub const bg_yellow = "\x1b[43m";
pub const bg_blue = "\x1b[44m";
pub const bg_magenta = "\x1b[45m";
pub const bg_cyan = "\x1b[46m";
pub const bg_white = "\x1b[47m";

// --- Background Colors (Bright) ---
pub const bg_bright_black = "\x1b[100m"; // Often gray
pub const bg_bright_red = "\x1b[101m";
pub const bg_bright_green = "\x1b[102m";
pub const bg_bright_yellow = "\x1b[103m";
pub const bg_bright_blue = "\x1b[104m";
pub const bg_bright_magenta = "\x1b[105m";
pub const bg_bright_cyan = "\x1b[106m";
pub const bg_bright_white = "\x1b[107m";
