module logger;

import std::time::datetime, std::io;
import color;

fn String get_timestamp() @local
{
    TzDateTime time = datetime::now().to_local();
    String time_format = datetime::tformat(DateTimeFormat.TIMEONLY, time);
    return string::tformat("[%s]", time_format);
}

fn void warn(String fmt, args...)
{
    io::printfn("%s%s [WARN] %s%s", color::WARN, get_timestamp(), string::tformat(fmt, ...args), color::RESET);
}

fn void info(String fmt, args...)
{
    io::printfn("%s [INFO] %s%s", get_timestamp(), string::tformat(fmt, ...args), color::RESET);
}

fn void error(String fmt, args...)
{
    io::printfn("%s%s [ERROR] %s%s", color::ERROR, get_timestamp(), string::tformat(fmt, ...args), color::RESET);
}