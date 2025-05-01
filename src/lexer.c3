module lexer;

import std::collections, std::io, std::core::mem::allocator;
import logger;

extern fn char *strtok(char *strToken, char *strDelimit);

faultdef INCORRECT_TOKEN, WRONG_TOKEN_FMT, WRONG_EXPR_FMT;

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

fn Token[3]? lex_move(String line) @local @inline
{
    Token[3] tokens;
    String[] splitLine = line.split(allocator::temp(), " ");
    if (splitLine.len != 3) return WRONG_TOKEN_FMT?;
    tokens[0] = {.type = TokenTypes.KEYWORD, .key_word = KeyWords.MOVE, .value = splitLine[0]};
    tokens[1] = {.type = TokenTypes.REGISTER, .key_word = KeyWords.MOVE, .value = splitLine[0]};
    tokens[2] = {.type = TokenTypes.LITERAL, .key_word = KeyWords.MOVE, .value = splitLine[0]};
    return tokens;
}

fn Token? lex_call(String line) @local @inline
{
    if (line.trim() != "call") return WRONG_TOKEN_FMT?;
    return {.type = TokenTypes.KEYWORD, .key_word = KeyWords.CALL, .value = line.trim()};
}

fn List { Token }? Lexer.lex(&self)
{
    List { String } lines = self.lines;
    List { Token } tokens;
    defer lines.free();
    defer tokens.free();

    char *delimiters = " ,";

    self.logger.info("Lexing input of length %s", lines.len());

    foreach (index, item : lines) {
        String line = item.copy(allocator::temp());
        defer line.free(allocator::temp());

        self.logger.info("Line %d: %s", index, line);

        if (line.starts_with("move")) {
            Token[3]? tmp_tokens = lex_move(line);
            if (catch error = tmp_tokens) return INCORRECT_TOKEN?;
            foreach (token : tmp_tokens) tokens.push(token);
        } else if (line.starts_with("call")) {
            Token? tmp_token = lex_call(line);
            if (catch error = tmp_token) return INCORRECT_TOKEN?;
            tokens.push(tmp_token);
        } else {
            return INCORRECT_TOKEN?;
        }
    }

    self.logger.info("Successfully lexed %s Tokens", tokens.len());

    return tokens;
}