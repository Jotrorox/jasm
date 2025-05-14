module cli;

import std::io;
import std::os;
import utils;

const int EXIT_SUCCESS = 0;
const int EXIT_FAILURE = 1;

struct Option
{
    char   short_name;
    String long_name;
    String description;
    bool   has_arg;
    bool   required;
    String value;
}

struct ArgParser
{
    String[] args;
    Option[] options;
    utils::Config config;
}

fn String ArgParser.get_arg(&self, String name)
{
    for (int i = 0; i < self.options.len; i++)
    {
        if (name == self.options[i].long_name || (name.len == 1 && name[0] == self.options[i].short_name))
        {
            return self.options[i].value;
        }
    }

    io::eprintfn("%sArgument '%s' is not defined%s", utils::RED, name, utils::RESET);
    os::exit(EXIT_FAILURE);
    return "";
}

fn bool ArgParser.has_arg(&self, String name)
{
    for (int i = 0; i < self.options.len; i++)
    {
        if (name == self.options[i].long_name || (name.len == 1 && name[0] == self.options[i].short_name))
        {
            return self.options[i].value != "";
        }
    }
    return false;
}

fn void ArgParser.parse(&self)
{
    for (int i = 1; i < self.args.len; i++)
    {
        String arg = self.args[i];

        if (arg == "--help" || arg == "-h")
        {
            self.print_usage();
            os::exit(EXIT_SUCCESS);
        }

        for (int j = 0; j < self.options.len; j++)
        {
            bool is_long = arg.starts_with("--") && arg[2..] == self.options[j].long_name;
            bool is_short = arg.starts_with("-") && !arg.starts_with("--") &&  arg[1] == self.options[j].short_name && arg.len == 2;

            if (is_long || is_short)
            {
                if (self.options[j].has_arg)
                {
                    if (i + 1 >= self.args.len)
                    {
                        io::eprintfn("%sOption '%s' requires an argument%s", utils::RED, arg, utils::RESET);
                        os::exit(EXIT_FAILURE);
                    }
                    self.options[j].value = self.args[i + 1];
                    i++;
                } else
                {
                    self.options[j].value = arg;
                }
                break;
            }
        }
    }

    for (int i = 0; i < self.options.len; i++)
    {
        if (self.options[i].required && self.options[i].value == "")
        {
            io::eprintfn("%sOption '--%s' is required%s", utils::RED, self.options[i].long_name, utils::RESET);
            os::exit(EXIT_FAILURE);
        }
    }
}

fn void ArgParser.print_usage(&self)
{
    io::printfn("%s%s v%s%s", utils::BOLD, self.config.name, self.config.version, utils::RESET);
    io::printfn("Copyright (c) %d %s", self.config.year, self.config.author);
    io::printn();

    io::printfn("%sUsage: %s [OPTIONS]%s", utils::BOLD, self.args[0], utils::RESET);
    io::printn();
    io::printfn("%sOptions:%s", utils::BOLD, utils::RESET);

    int max_long_name_len = 0;
    for (int i = 0; i < self.options.len; i++)
    {
        int len = self.options[i].long_name.len;
        if (len > max_long_name_len)
        {
            max_long_name_len = len;
        }
    }

    Option help_option = { .short_name = 'h', .long_name = "help", .description = "Displays this message",  .has_arg = false };

    if (help_option.short_name)
    {
        io::printfn("  %s-%c%s, %s--%-10s%s  %s", utils::GREEN, help_option.short_name, utils::RESET, utils::GREEN, help_option.long_name, utils::RESET, help_option.description);
    } else
    {
        io::printfn("      %s--%-10s%s  %s", utils::GREEN, help_option.long_name, utils::RESET, help_option.description);
    }

    for (int i = 0; i < self.options.len; i++)
    {
        if (self.options[i].short_name)
        {
            io::printfn("  %s-%c%s, %s--%-10s%s  %s", utils::GREEN, self.options[i].short_name, utils::RESET, utils::GREEN, self.options[i].long_name, utils::RESET, self.options[i].description);
        } else
        {
            io::printfn("      %s--%-10s%s  %s", utils::GREEN, self.options[i].long_name, utils::RESET, self.options[i].description);
    }
    }
}
