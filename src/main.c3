module jasm;

import std::io, std::collections;
import logger;
import argparser;
import lexer;
import config;

const config::Config CONFIG = {
    .version = "0.2.0",
    .name = "jasm",
    .author = "Johannes (jotrorox) Müller",
    .year = 2025,
    .license = "MIT"
};

fn void handle_cmd_version() {
	io::printfn("%s %s - Copyright %d %s", CONFIG.version, CONFIG.name, CONFIG.year, CONFIG.author);
}

fn int main(String[] args)
{
    logger::Logger logger = {.to_file = false};

	Option[] options = {
		{
			.short_name = 'v',
			.long_name = "version",
			.description = "Displays the version",
			.has_arg = false,
			.required = false
		}
	};
	ArgParser parser = {args, options, CONFIG};
	parser.parse();
	if (parser.has_arg("v")) {
		handle_cmd_version(); 
		return 0;
	}

	if (args.len <= 1) {
		logger.error("Please input a jasm file or use the command line options!");
		logger.error("Usage: %s <your jasm file>", args[0]);
		return 1;
	}

	String filename = args[1];

	if (!file::is_file(filename)) {
		logger.error("Please input a valid file path!");
		logger.error("Usage: %s <your jasm file>", args[0]);
		return 1;
	}

	File file = file::open(filename, "r")!!;
	defer (void)file.close();

	List{ String } lines;
	while (try line = io::treadline(&file)) {
		lines.push(line);
	}

    lexer::Lexer lexer = {.lines = lines, .logger = {.to_file = false}};
    List { lexer::Expression } expressions = lexer.lex()!!;

    foreach (item : expressions) {
        logger.info("Expression at Line %d: %s", item.line, item.type);
    }

	return 0;
}
