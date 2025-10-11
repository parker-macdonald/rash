#include <stddef.h>

#include "../vector.h"
#include "lex.h"
#include "parser.h"

ast_node *parse(const token_t *const tokens) {
  VECTOR(char *) argv;
  VECTOR_INIT(argv);

  ast_node *root = malloc(sizeof(ast_node));
  root->type = AST_OPERAND;

  for (size_t i = 0; tokens[i].type != END; i++) {
    if (tokens[i].type == STRING) {
      VECTOR_PUSH(argv, (char *)tokens[i].data);

      continue;
    }

    if (tokens[i].type == STDIN_REDIR) {
      int fd = open()
      root->operand.
    }
  }
}