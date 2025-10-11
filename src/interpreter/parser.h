#ifndef PARSER_H
#define PARSER_H

#include "../execute.h"
#include "lex.h"

enum ast_node_type {
  AST_OPERATOR,
  AST_OPERAND
};

enum ast_operator_type {
  AST_AND,
  AST_OR,
  AST_PIPE,
  AST_SEMI
};

typedef struct {
  enum ast_operator_type type;
} ast_operator;

typedef struct {
  enum ast_node_type type;
  union {
    execution_context operand;
    ast_operator operator;
  };
} ast_node;

ast_node *parse(const token_t *const tokens);

#endif
