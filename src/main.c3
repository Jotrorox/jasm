module jasm;

import std::io, std::collections;
import cli;
import lexer;
import utils;

const utils::Config CONFIG = {
    .version = "0.2.0",
    .name = "jasm",
    .author = "Johannes (jotrorox) Müller",
    .year = 2025,
    .license = "MIT"
};

fn int main(String[] args)
{
    utils::Logger logger = {.to_file = false};

  args = cli::cli(args, logger);
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
