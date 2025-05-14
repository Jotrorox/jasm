module utils;

import std::time::datetime, std::io;

struct Logger {
    // TODO: Implement file logging
    bool to_file;
}

fn String get_timestamp() @local
{
    TzDateTime time = datetime::now().to_local();
    String time_format = datetime::tformat(DateTimeFormat.TIMEONLY, time);
    return string::tformat("[%s]", time_format);
}

fn void Logger.info(&self, String fmt, args...)
{
    io::printfn("%s%s [INFO] %s%s", utils::BRIGHT_GREEN, get_timestamp(), string::tformat(fmt, ...args), utils::RESET);
}

fn void Logger.debug(&self, String fmt, args...)
{
    io::printfn("%s%s [DEBUG] %s%s", utils::DIM, get_timestamp(), string::tformat(fmt, ...args), utils::RESET);
}

fn void Logger.warn(&self, String fmt, args...)
{
    io::printfn("%s%s%s [WARN] %s%s", utils::BRIGHT_YELLOW, utils::BOLD, get_timestamp(), string::tformat(fmt, ...args), utils::RESET);
}

fn void Logger.error(&self, String fmt, args...)
{
    io::printfn("%s%s%s [ERROR] %s%s", utils::BOLD, utils::BRIGHT_RED, get_timestamp(), string::tformat(fmt, ...args), utils::RESET);
}
