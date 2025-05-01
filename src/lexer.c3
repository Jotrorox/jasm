module lexer;

import std::collections, std::io, std::core::mem::allocator;
import logger;

extern fn char *strtok(char *strToken, char *strDelimit);

faultdef INCORRECT_TOKEN, WRONG_TOKEN_FMT;

enum TokenTypes {
    KEYWORD,
    LITERAL,
    REGISTER,
    INVALID,
    NONE
}

enum KeyWords {
    MOVE,
    CALL,
    NONE
}

enum ExpressionTypes {
    MOVE,
    CALL,
    NONE
}

struct Token {
    TokenTypes type;
    KeyWords key_word;
    String value;
}

struct Expression {
    ExpressionTypes type;
    List{ Token } tokens;
    int line;
}

struct Lexer {
    List{ String } lines;
    logger::Logger logger;
}

fn TokenTypes[3]? lex_move(String line) @local @inline
{
    TokenTypes[3] tokens = {TokenTypes.KEYWORD, TokenTypes.REGISTER, TokenTypes.LITERAL};
    return tokens;
}

fn TokenTypes? lex_call(String line) @local @inline
{
    if (line.trim() != "call") return WRONG_TOKEN_FMT?;
    return TokenTypes.KEYWORD;
}

fn List { TokenTypes }? Lexer.lex(&self)
{
    List { String } lines = self.lines;
    List { TokenTypes } tokens;
    defer lines.free();
    defer tokens.free();

    char *delimiters = " ,";

    self.logger.info("Lexing input of length %s", lines.len());

    foreach (index, item : lines) {
        String line = item.copy(allocator::temp());
        defer line.free(allocator::temp());

        self.logger.info("Line %d: %s", index, line);

        if (line.starts_with("move")) {
            TokenTypes[3]? tmp_tokens = lex_move(line);
            if (catch error = tmp_tokens) return INCORRECT_TOKEN?;
            foreach (token : tmp_tokens) tokens.push(token);
        } else if (line.starts_with("call")) {
            TokenTypes? tmp_token = lex_call(line);
            if (catch error = tmp_token) return INCORRECT_TOKEN?;
            tokens.push(tmp_token);
        } else {
            return INCORRECT_TOKEN?;
        }
    }

    self.logger.info("Successfully lexed %s Tokens", tokens.len());

    return tokens;
}