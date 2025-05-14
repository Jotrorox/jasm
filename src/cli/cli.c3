module cli;

import jasm;
import utils;
import std::io, std::os;

fn void handle_cmd_version() {
    io::printfn("%s %s - Copyright %d %s", jasm::CONFIG.version, jasm::CONFIG.name, jasm::CONFIG.year, jasm::CONFIG.author);
}

fn String[] cli(String[] args, utils::Logger logger) {
    Option[] options = {
        {
        .short_name = 'v',
        .long_name = "version",
        .description = "Displays the version",
        .has_arg = false,
        .required = false
        }
    };
    ArgParser parser = {args, options, jasm::CONFIG};
    parser.parse();
    if (parser.has_arg("v")) {
        handle_cmd_version();
        os::exit(0);
    }

    if (args.len <= 1) {
        logger.error("Please input a jasm file or use the command line options!");
        logger.error("Usage: %s <your jasm file>", args[0]);
        os::exit(1);
    }

    return args;
}
