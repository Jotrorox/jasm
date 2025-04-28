module logger;

import std::time::datetime, std::io;

const String COLOR_ERROR = "\u001B[31m";
const String COLOR_RESET = "\u001B[0m";
const String COLOR_WARN  = "\u001B[33m";

fn String get_timestamp() @local
{
    TzDateTime time = datetime::now().to_local();
    String time_format = datetime::tformat(DateTimeFormat.TIMEONLY, time);

    return string::tformat("[%s]", time_format);
}

fn void warn(String fmt, args...)
{
    io::printfn("%s%s [WARN] %s%s", COLOR_WARN, get_timestamp(), string::tformat(fmt, ...args), COLOR_RESET);
}

fn void info(String fmt, args...)
{
    io::printfn("%s [INFO] %s%s", get_timestamp(), string::tformat(fmt, ...args), COLOR_RESET);
}

fn void error(String fmt, args...)
{
    io::printfn("%s%s [ERROR] %s%s", COLOR_ERROR, get_timestamp(), string::tformat(fmt, ...args), COLOR_RESET);
}