module logger;

import std::time::datetime, std::io;
import ansi;

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
    io::printfn("%s%s [INFO] %s%s", ansi::BRIGHT_GREEN, get_timestamp(), string::tformat(fmt, ...args), ansi::RESET);
}

fn void Logger.debug(&self, String fmt, args...)
{
    io::printfn("%s%s [DEBUG] %s%s", ansi::DIM, get_timestamp(), string::tformat(fmt, ...args), ansi::RESET);
}

fn void Logger.warn(&self, String fmt, args...)
{
    io::printfn("%s%s%s [WARN] %s%s", ansi::BRIGHT_YELLOW, ansi::BOLD, get_timestamp(), string::tformat(fmt, ...args), ansi::RESET);
}

fn void Logger.error(&self, String fmt, args...)
{
    io::printfn("%s%s%s [ERROR] %s%s", ansi::BOLD, ansi::BRIGHT_RED, get_timestamp(), string::tformat(fmt, ...args), ansi::RESET);
}