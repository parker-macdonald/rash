#include "interpreter/token.h"
#include "lib/buffer.h"
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
