#include "Token.h"

const char* tokenKindToString(TokenKind kind) {
    switch (kind) {
        case TokenKind::IDENT: return "IDENT";
        case TokenKind::INT_LIT: return "INT_LIT";
        case TokenKind::FLOAT_LIT: return "FLOAT_LIT";
        case TokenKind::STRING_LIT: return "STRING_LIT";
        case TokenKind::EOF: return "EOF";
        case TokenKind::ERROR: return "ERROR";
        default: return "KEYWORD/OPERATOR";
    }
}