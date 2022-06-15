#include "mpc.h"
#define LASSERT(args, cond, err) \
  if (!(cond)) { lval_del(args); return lval_err(err) ;}

#define MIN(x,y) \
  ({(x < y) ? x : y;}) \

#define MAX(x,y) \
  ({(x > y) ? x : y;}) \

struct lval;
struct lenv;
typedef struct lval lval;
typedef struct lenv lenv;

enum { LVAL_NUM, LVAL_ERR, LVAL_SYM, LVAL_FUN, LVAL_SEXPR, LVAL_QEXPR  };
enum { LERR_DIV_ZERO, LERR_BAD_OP, LERR_BAD_NUM };

typedef lval*(*lbuiltin) (lenv*, lval*);

typedef struct lval{
  int type;
  long num;

  /* store string data*/
  char* err;
  char* sym;
  lbuiltin fun;

  /* Count and pointer to a list of "lval*" */
  int count;
  struct lval** cell;
} lval;

void lval_expr_print(lval* v, char open, char close);
lval* lval_eval_sexpr(lval* v);
lval* lval_take(lval* v, int i);
lval* lval_pop(lval* v, int i);
lval* built_in_op(char* op, lval* a);
lval* bulitin(char* func, lval *a);

static char buffer[2048];



lval* lval_num(long x) {
  lval* v = malloc(sizeof(lval));
  v->type = LVAL_NUM;
  v->num = x;
  return v;
}

lval* lval_err(char *m) {
  lval* v = malloc(sizeof(lval));
  v->type = LVAL_ERR;
  v->err =  malloc(strlen(m)+1);
  strcpy(v->err, m);
  return v;
}

lval* lval_sym(char *m) {
  lval* v = malloc(sizeof(lval));
  v->type = LVAL_SYM;
  v->sym =  malloc(strlen(m)+1);
  strcpy(v->sym, m);
  return v;
}

/* empty sexpr lval*/
lval* lval_sexpr(void) {
  lval* v = malloc(sizeof(lval));
  v->type = LVAL_SEXPR;
  v->count = 0;
  v->cell = NULL;
  return v;
}

lval* lval_qexpr(void) {
  lval* v = malloc(sizeof(lval));
  v->type = LVAL_QEXPR;
  v->count = 0;
  v->cell = NULL;
  return v;
}

lval* lval_fun(lbuiltin func) {
  lval* v = malloc(sizeof(lval));
  v->type =  LVAL_FUN;
  v->fun = func;
  return v;
}

void lval_del(lval* v) {

  switch (v->type) {
  case LVAL_NUM:
    break;
  case LVAL_ERR: 
    free(v->err);
    v->err = NULL;
    break;
  case LVAL_SYM: 
    free(v->sym);
    v->sym = NULL;
    break;
  case LVAL_QEXPR:
  case LVAL_SEXPR:
    for (int i = 0; i< v->count; i++) {
      lval_del(v->cell[i]);
    }
    free(v->cell);
    v->cell = NULL;
    v->count = 0;
    break; 
  
  case LVAL_FUN: break; //dont do anything when deleting 
  
  }
  
  free(v);
  v = NULL;
}

lval* lval_read_num(mpc_ast_t* t) {
  errno = 0;
  long x =  strtol(t->contents, NULL, 10);

  return errno != ERANGE ?
    lval_num(x) : lval_err("invalid number");  
}

lval* lval_add(lval*v, lval* x) {
  v->count++;
  v->cell = realloc(v->cell, sizeof(lval*) * v->count);
  v->cell[v->count -1] = x;
  return  v;
}

lval* lval_read(mpc_ast_t* t) {
  if (strstr(t->tag, "number")) {
    return lval_read_num(t);
  }

  if (strstr(t->tag, "symbol")) {
    return lval_sym(t->contents);
  }

  /* if root (>) or sexpr then create empty list, > / >()*/
  lval* x = NULL;

  if (strcmp(t->tag, ">") == 0) { x = lval_sexpr(); }
  if (strstr(t->tag, "sexpr")) { x = lval_sexpr(); }
  if (strstr(t->tag, "qexpr")) { x = lval_qexpr(); }

  /*fill the list with a valid expressions within*/
  for (int i =0; i < t->children_num; i++) {
    if (strcmp(t->children[i]->contents, "(") == 0) {
     continue; 
    }
    if (strcmp(t->children[i]->contents, ")") == 0) {
     continue; 
    }
    if (strcmp(t->children[i]->contents, "{") == 0) {
     continue; 
    }
    if (strcmp(t->children[i]->contents, "}") == 0) {
     continue; 
    }
    if (strcmp(t->children[i]->tag, "regex") == 0) {
     continue; 
    }
    x = lval_add(x, lval_read(t->children[i]));

  }
  return x;
}

void lval_print(lval* v) {
  switch (v->type) {
    case LVAL_NUM : printf("%li", v->num); break;
    case LVAL_ERR : printf("Error: %s", v->err); break;
    case LVAL_SYM : printf("%s", v->sym); break;
    case LVAL_FUN : printf("<function>"); break;
    case LVAL_SEXPR : lval_expr_print(v, '(', ')'); break;
    case LVAL_QEXPR : lval_expr_print(v, '{', '}'); break;
  }
}

void lval_expr_print(lval* v, char open, char close) {
  putchar(open);
  for (int i = 0; i < v->count; i++) {
    lval_print(v->cell[i]);
    
    /* Don't print trailing space if last element */
    if (i != (v->count -1)) {
      putchar(' ');
    }
  }

  putchar(close);
}

void lval_println(lval* v) { lval_print(v); putchar('\n'); }

lval* lval_eval(lval* v) {
  if (v->type == LVAL_SEXPR) {
    return lval_eval_sexpr(v);
  }

  /*all other lval types remain the same*/
  return v;
}

lval *lval_eval_sexpr(lval* v) {


  for (int i = 0; i < v->count; i++) {
    v->cell[i] =  lval_eval(v->cell[i]);
  }
  
  for (int i = 0; i < v->count; i++) {
    if (v->cell[i]->type == LVAL_ERR) {
      return lval_take(v, i);
    }
  }
  
  /* No child ()*/
  if (v->count == 0) return v;
  if (v->count == 1) {return lval_take(v, 0);}

  lval* f = lval_pop(v, 0);
  if (f->type != LVAL_SYM ) {
    lval_del(f);
    lval_del(v);
    return lval_err("S-expression Does not start with symbol!");
  }

  lval* result =  bulitin(f->sym, v);
  lval_del(f);
  return result;
}

lval* lval_pop(lval* v, int i) {
  lval* x = v->cell[i];
  memmove(&v->cell[i], &v->cell[i+1], sizeof(lval*)*(v->count-i-1));

  v->count--;
  v->cell = realloc(v->cell, sizeof(lval*)* v->count);
  return x;
}

lval* lval_take(lval* v, int i) {
  lval* x = lval_pop(v, i);
  lval_del(v);
  return x;
}

lval* lval_join(lval* v, lval* x) {
  while (x->count) {
    v = lval_add(v, lval_pop(x, 0));
  }
  lval_del(x);
  return v;
}

char* readline(char* prompt) {
  fputs(prompt, stdout);
  fgets(buffer, 2048, stdin);
  char* cpy = malloc(strlen(buffer)+1);
  strcpy(cpy, buffer);
  cpy[strlen(cpy)-1] = '\0';
  return cpy;
}

void add_history(char* unused) {}

lval* built_in_head(lval* a) {
  
  LASSERT(a, a->count != 0 , "Function 'head' no args!");
  LASSERT(a, a->count == 1 , "Function 'head' too many args!");
  LASSERT(a, a->cell[0]->type == LVAL_QEXPR , "Function 'head' wrong args!");
  LASSERT(a, a->cell[0]->count != 0 , "Function 'head' passed {}!");
  
  /* Otherwise take first argument */
  lval *v = lval_take(a, 0);
  while (v->count > 1) {lval_del(lval_pop(v,1)); }
  return v;  
}

lval* built_in_tail(lval* a) {
  
  LASSERT(a, a->count !=0 , "Function 'head' no args!");
  LASSERT(a, a->count ==1 , "Function 'head' too many args!");
  LASSERT(a, a->cell[0]->type == LVAL_QEXPR , "Function 'head' wrong args!");
  LASSERT(a, a->cell[0]->count != 0 , "Function 'head' passed {}!");

  lval* v = lval_take(a, 0);
  /*delete the first element and return*/
  lval_del(lval_pop(v, 0)); 
  return v;
}

lval* built_in_list(lval* a) {
  a->type = LVAL_QEXPR;
  return a;
}

lval* built_in_eval(lval* a) {

  LASSERT(a, a->count == 1, "FUnction 'eval' passed too many arguments!" );
  LASSERT(a, a->cell[0]->type == LVAL_QEXPR, "Function 'eval' invalid argument!" );

  lval* x = lval_take(a, 0);
  x->type = LVAL_SEXPR;
  return lval_eval(x);

}

lval* built_in_join(lval* a) {
  for (int i =0; i < a->count; i++){
    LASSERT(a, a->cell[i]->type == LVAL_QEXPR, "Function 'join' passed incorrect type");
  }

  lval* x = lval_pop(a, 0);

  while (a->count) {
    x = lval_join(x, lval_pop(a,0));
  }

  lval_del(a);
  return x;
}


lval* built_in_op(char * op, lval* a) {

  for (int i = 0; i< a->count ; i++) {
    LASSERT(a, a->cell[i]->type == LVAL_NUM, "Cannot operate on non number!");
  }

  lval* x = lval_pop(a, 0);

  if ((strcmp(op, "-") ==0) && a->count == 0) {
    x->num = -x->num;
  }
  while (a->count > 0 ) {

    lval *y = lval_pop(a, 0);
    
    if (strcmp(op, "+") == 0) { x->num += y->num; }
    if (strcmp(op, "c") == 0) { x->num -= y->num; }
    if (strcmp(op, "*") == 0) { x->num *= y->num; }
    if (strcmp(op, "/") == 0) {
      if (y->num == 0) {
        lval_del(y);
        x = lval_err("division by Zerro!");
        break;
      }
      x->num /= y->num;
    }

    lval_del(y);

  }

  lval_del(a);

  return x;
}

lval* bulitin(char* func, lval *a) {

  if (strcmp("list", func) == 0) { return built_in_list(a);}
  if (strcmp("head", func) == 0) { return built_in_head(a);}
  if (strcmp("tail", func) == 0) { return built_in_tail(a);}
  if (strcmp("join", func) == 0) { return built_in_join(a);}
  if (strcmp("eval", func) == 0) { return built_in_eval(a);}
  if (strstr("+-/*", func)) { return built_in_op(func, a);}
  lval_del(a);
  return lval_err("Unknow Functions!");
}
/*
lval eval(mpc_ast_t* t) {
  if (strstr(t->tag, "number")) {
    errno = 0;
    long x = strtol(t->contents, NULL, 10);
    return errno != ERANGE ? lval_num(x) : lval_err(LERR_BAD_NUM);
  }

  char *op =  t->children[1]->contents;
  lval x =  eval(t->children[2]);
  
  int i = 3;
  while (strstr(t->children[i]->tag, "expr")) {
    x = eval_op(x, op, eval(t->children[i]));
    i++;
  }

  return x;
}

*/

int main(int argc, char** argv) {
  
  /* Create Some Parsers */
  mpc_parser_t* Number   = mpc_new("number");
  mpc_parser_t* Symbol = mpc_new("symbol");
  mpc_parser_t* Sexpr = mpc_new("sexpr");
  mpc_parser_t* Qexpr = mpc_new("qexpr");
  mpc_parser_t* Expr     = mpc_new("expr");
  mpc_parser_t* Lispy    = mpc_new("lispy");
  
  mpca_lang(MPCA_LANG_DEFAULT,
    "                                                     \
      number   : /-?[0-9]+/ ;                             \
      symbol   :  /[a-zA-Z0-9_+\\-*\\/\\\\=<>!&]+/ ;      \
      sexpr    : '(' <expr>* ')' ;                        \
      qexpr    : '{' <expr>* '}' ;                        \
      expr     : <number> | <symbol> | <sexpr> | <qexpr> ;  \
      lispy    : /^/ <expr>* /$/ ;             \
    ",
    Number, Symbol, Sexpr, Qexpr, Expr, Lispy);
  
  puts("Lispy Version 0.0.0.0.2");
  puts("Press Ctrl+c to Exit\n");
  
  while (1) {
  
    char* input = readline("lispy> ");
    add_history(input);
    
    /* Attempt to parse the user input */
    mpc_result_t r;
    if (mpc_parse("<stdin>", input, Lispy, &r)) {

      mpc_ast_print(r.output);
      lval* x = lval_eval(lval_read(r.output));
      lval_println(x);
      lval_del(x);
      mpc_ast_delete(r.output);
    
    } else {
      /* Otherwise print and delete the Error */
      mpc_err_print(r.error);
      mpc_err_delete(r.error);
    }
    
    free(input);
  }
  
  /* Undefine and delete our parsers */
  mpc_cleanup(4, Number, Symbol, Sexpr ,Qexpr , Expr, Lispy);
  
  return 0;
}