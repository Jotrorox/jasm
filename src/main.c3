module jasm;

import std::io, std::collections;
import logger;
import argparser;

fn void handle_cmd_version() {
	io::printn("jasm 0.2.0 - Copyright 2025 Johannes (jotrorox) Müller");
}

fn int main(String[] args)
{
	Option[] options = {
		{
			.short_name = 'v',
			.long_name = "version",
			.description = "Displays the version",
			.has_arg = false,
			.required = false
		}
	};
	ArgParser parser = {args, options};
	parser.parse();
	if (parser.has_arg("v")) {
		handle_cmd_version(); 
		return 0;
	}

	if (args.len <= 1) {
		io::eprintn("Please input a jasm file or use the command line options!");
		io::eprintfn("Usage: %s <your jasm file>", args[0]);
		return 1;
	}

	String filename = args[1];

	if (!file::is_file(filename)) {
		io::eprintn("Please input a valid file path!");
		io::eprintfn("Usage: %s <your jasm file>", args[0]);
		return 1;
	}

	File file = file::open(filename, "r")!!;
	defer (void)file.close();

	List{ String } lines;
	while (try line = io::treadline(&file)) {
		lines.push(line);
	}

	List { int } move_lines;
	foreach (index, item : lines) {
		if (item.trim().starts_with("move")) {
			move_lines.push(((int)index)+1);
			logger::info("Found move line at index %d", index);
		}
	}

	return 0;
}
