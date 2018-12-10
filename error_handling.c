#include "mpc.h"

#ifdef _WIN32

static char buffer[2048];

char* readline(char* prompt) {
  fputs(prompt, stdout);
  fgets(buffer, 2048, stdin);
  char* cpy = malloc(strlen(buffer)+1);
  strcpy(cpy, buffer);
  cpy[strlen(cpy)-1] = '\0';
  return cpy;
}

void add_history(char* unused) {}

#else
#include <editline/readline.h>
#include <editline/history.h>
#endif

/* enumerate error types */
enum { LERR_DIV_ZERO, LERR_BAD_OP, LERR_BAD_NUM };

/* enumerate lval types */
enum { LVAL_NUM, LVAL_ERR };

/* lval struct */
typedef struct {
  int type;
  long num;
  int err;
} lval;

/* number type lval */
lval lval_num(long x) {
  lval v;
  v.type = LVAL_NUM;
  v.num = x;
  return v;
}

/* error type lval */
lval lval_err(int x) {
  lval v;
  v.type = LVAL_ERR;
  v.err = x;
  return v;
}

/* print an lval */
void lval_print(lval v){
  switch(v.type) {
    case LVAL_NUM: printf("%li", v.num); 
      break;

    case LVAL_ERR:
      if (v.err == LERR_DIV_ZERO) {
        printf("Error: Division By Zero!");
      }
      if (v.err == LERR_BAD_OP) {
        printf("Error: Invalid Operator!");
      }
      if (v.err == LERR_BAD_NUM) {
        printf("Error: Invalid Number!");
      }
      break; 
  }
}

/* print lval with newline */
void lval_println(lval v) { lval_print(v); putchar('\n'); }

/* check string and perform operation */
 lval eval_op(lval x, char* op, lval y){

  /* return error if there's one */
  if (x.type == LVAL_ERR) { return x; }
  if (y.type == LVAL_ERR) { return y; }

  /* otherwise do the maths and shit */
  if (strcmp(op, "+") == 0) { return lval_num(x.num + y.num); }
  if (strcmp(op, "-") == 0) { return lval_num(x.num - y.num); }
  if (strcmp(op, "*") == 0) { return lval_num(x.num * y.num); }
  if (strcmp(op, "/") == 0) { 
    /* guard divisions by zero */
    return y.num == 0
      ? lval_err(LERR_DIV_ZERO)
      : lval_num(x.num / y.num);
  }

  return lval_err(LERR_BAD_OP);
}


lval eval(mpc_ast_t* t) {

  /* if tagged as number, return it */
  if (strstr(t->tag, "number")) {
    /* check for conversion error */
    errno = 0;
    long x = strtol(t->contents, NULL, 10);
    return errno != ERANGE ? lval_num(x) : lval_err(LERR_BAD_NUM);
  }

  /* the operator is always the 2nd child */
  char* op = t->children[1]->contents;

  /* store the 3rd child in var x */
  lval x = eval(t->children[2]);

  /* keep iterating */
  int i = 3;

  while (strstr(t->children[i]->tag, "expr")) {
    x = eval_op(x, op, eval(t->children[i]));
    i++;
  }

  return x;
}


int main(int argc, char** argv) {

  /* parsers */
  mpc_parser_t* Number   = mpc_new("number");
  mpc_parser_t* Operator = mpc_new("operator");
  mpc_parser_t* Expr     = mpc_new("expr");
  mpc_parser_t* Lispy    = mpc_new("lispy");

  /* language definition */
  mpca_lang(MPCA_LANG_DEFAULT, 
      "                                                   \
        number  : /-?[0-9]+/ ;                            \
        operator: '+' | '-' | '*' | '/' ;                 \
        expr    : <number> | '(' <operator> <expr>+ ')' ; \
        lispy   : /^/ <operator> <expr>+ /$/ ;            \
      ",
      Number, Operator, Expr, Lispy);

  puts("Lispy Version 0.0.0.0.4");
  puts("Press Ctrl+c to Exit\n");

  while (1) {

    char* input = readline("lispy> ");
    add_history(input);

    /* try to parse input */
    mpc_result_t r;
    if (mpc_parse("<stdin>", input, Lispy, &r)) {

      lval result = eval(r.output);
      lval_println(result);

      mpc_ast_print(r.output); // outputs parsed tree */

      mpc_ast_delete(r.output);
    } else {
      mpc_err_print(r.error);
      mpc_err_delete(r.error);
    }

    free(input);

  }

  /* undefine and delete parsers */
  mpc_cleanup(4, Number, Operator, Expr, Lispy);

  return 0;
}

