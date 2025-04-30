module lexer;

import std::collections, std::io, std::core::mem::allocator;
import logger;

extern fn char *strtok(char *strToken, char *strDelimit);

enum Token {
    KEYWORD,
    LITERAL,
    REGISTER,
    INVALID,
    NONE
}

enum KeyWords {
    MOVE,
    CALL
}

struct Lexer {
    List{ String } lines;
    logger::Logger logger;
}

fn List { Token }? Lexer.lex(&self) {
    List {String} lines = self.lines;
    List { Token } tokens;
    defer lines.free();
    defer tokens.free();

    char *delimiters = " ,";

    self.logger.info("Lexing input of length %s", lines.len());

    foreach (index, item : lines) {
        String line = item.copy(allocator::temp());
        defer line.free(allocator::temp());

        self.logger.info("Line %d: %s", index, line);

        // Max 3 Tokens per line `Keyword Register, Literal`
        int token_count = 0;
        Token[3] tmp_tokens;
        char *token = strtok(line, delimiters);

        while (token != null && token_count < 3) {
            if (token_count == 0) {
                switch (token) {
                    case "move": tmp_tokens[token_count] = Token.KEYWORD;
                    case "call": tmp_tokens[token_count] = Token.KEYWORD;
                    default: tmp_tokens[token_count] = Token.INVALID;
                }
            } else if (token_count == 1) {
                tmp_tokens[token_count] = Token.REGISTER;
            } else {
                tmp_tokens[token_count] = Token.LITERAL;
            }

            token = strtok(null, delimiters);
        }

        foreach (tmp_token : tmp_tokens) {
            tokens.push(tmp_token);
        }
    }

    self.logger.info("Successfully lexed %s Tokens", tokens.len());

    return tokens;
}