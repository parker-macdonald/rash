#include "interpreter/token.h"
#include "lib/buffer.h"
#include "lib/error.h"
#include "lib/vector.h"

void token_destroy(Token *token) {
  if (IS_BUFFER_TOKEN(token->kind)) {
    buffer_destroy(&token->buffer);
  }
}

void token_list_destroy(TokenList *list) {
  for (size_t i = 0; i < list->length; i++) {
    token_destroy(list->data + i);
  }

  VECTOR_DESTROY(*list);
}

const char *get_token_kind_name(TokenKind kind) {
  switch (kind) {
    case TK_STDIN_REDIR:
      return "‘<’";
    case TK_STDIN_REDIR_STRING:
      return "‘<<<’";
    case TK_STDOUT_REDIR:
      return "‘>’";
    case TK_STDOUT_REDIR_APPEND:
      return "‘>>’";
    case TK_STDERR_REDIR:
      return "‘2>’";
    case TK_STDERR_REDIR_APPEND:
      return "‘2>>’";
    case TK_STDOUT_ERR_REDIR:
      return "‘&>’";
    case TK_STDOUT_ERR_REDIR_APPEND:
      return "‘&>>’";
    case TK_PIPE:
      return "‘|’";
    case TK_LOGICAL_OR:
      return "‘||’";
    case TK_SEMI:
      return "‘;’";
    case TK_LOGICAL_AND:
      return "‘&&’";
    case TK_AMP:
      return "‘&’";
    case TK_ARG_BEGIN:
      return "argument";
    case TK_ARG_END:
      return "(internal TK_ARG_END)";
    case TK_ARG_STRING:
      return "string";
    case TK_ARG_WILDCARD:
      return "‘*’";
    case TK_ARG_DOUBLESTAR:
      return "‘**’";
    case TK_ARG_ENV:
      return "environment variable";
    case TK_ARG_SUBSHELL:
      return "subshell";
    case TK_ARG_TILDE:
      return "‘~’";
    case TK_NONE:
      return "none";
    case TK_SE_BEGIN:
      return "(internal TK_SE_BEGIN)";
    case TK_SE_END:
      return "(internal TK_SE_END)";
    case TK_SE_NONE:
      return "none";
    case TK_SE_ADD:
      return "‘+’";
    case TK_SE_SUB:
      return "‘-’";
    case TK_SE_MUL:
      return "‘*’";
    case TK_SE_POW:
      return "‘**’";
    case TK_SE_DIV:
      return "‘/’";
    case TK_SE_MOD:
      return "‘%’";
    case TK_SE_EQ:
      return "‘==’";
    case TK_SE_NEQ:
      return "‘!=’";
    case TK_SE_GT:
      return "‘>’";
    case TK_SE_LT:
      return "‘<’";
    case TK_SE_GTE:
      return "‘>=’";
    case TK_SE_LTE:
      return "‘<=’";
    case TK_SE_NOT:
      return "‘!’";
    case TK_SE_QUESTION:
      return "‘?’";
    case TK_SE_COLON:
      return "‘:’";
    case TK_SE_O_PAREN:
      return "‘(’";
    case TK_SE_C_PAREN:
      return "‘)’";
    case TK_SE_COMMA:
      return "‘,’";
    case TK_SE_STRING_LIT:
      return "string literal";
    case TK_SE_NUMBER_LIT:
      return "number literal";
    case TK_SE_NULL_LIT:
      return "null";
    case TK_SE_TRUE_LIT:
      return "true";
    case TK_SE_FALSE_LIT:
      return "false";
    case TK_SE_IDENTIFIER:
      return "identifier";
  }

  unreachable();
}
