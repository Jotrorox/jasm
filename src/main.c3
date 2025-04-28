module jasm;

import std::io;
import logger;

fn int main(String[] args)
{
	File file = file::open("test.jasm", "r")!!;
	defer (void)file.close();

	logger::info("Reading file...");

	return 0;
}
