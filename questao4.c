#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>

#define MAXTOK 4096
#define MAXVARS 16
#define MAXNAME 64

typedef enum {
    T_END, T_LP, T_RP, T_NOT, T_AND, T_OR, T_IMPL, T_IFF, T_IDENT
} TokType;

typedef struct {
    TokType type;
    char ident[MAXNAME];
} Token;

typedef struct {
    Token t[MAXTOK];
    int n, i;
} TokStream;

static void push_tok(TokStream* ts, TokType tp, const char* id) {
    if (ts->n >= MAXTOK) { fprintf(stderr, "Muitos tokens.\n"); exit(1); }
    ts->t[ts->n].type = tp;
    if (id) strncpy(ts->t[ts->n].ident, id, MAXNAME-1);
    else ts->t[ts->n].ident[0] = '\0';
    ts->n++;
}

static void lex(const char* s, TokStream* ts) {
    ts->n = ts->i = 0;
    for (int i = 0; s[i]; ) {
        if (isspace((unsigned char)s[i])) { i++; continue; }

        if (s[i] == '(') { push_tok(ts, T_LP, NULL); i++; continue; }
        if (s[i] == ')') { push_tok(ts, T_RP, NULL); i++; continue; }
        if (s[i] == '!' || s[i] == '~') { push_tok(ts, T_NOT, NULL); i++; continue; }
        if (s[i] == '&') { push_tok(ts, T_AND, NULL); i++; continue; }
        if (s[i] == '|' || s[i] == 'v' || s[i] == 'V') { push_tok(ts, T_OR, NULL); i++; continue; }

        if (s[i] == '<' && s[i+1] == '-' && s[i+2] == '>') { push_tok(ts, T_IFF, NULL); i += 3; continue; }
        if (s[i] == '-' && s[i+1] == '>') { push_tok(ts, T_IMPL, NULL); i += 2; continue; }

        if (isalpha((unsigned char)s[i])) {
            char buf[MAXNAME]; int k = 0;
            while (isalnum((unsigned char)s[i]) || s[i]=='_') {
                if (k < MAXNAME-1) buf[k++] = s[i];
                i++;
            }
            buf[k] = '\0';
            push_tok(ts, T_IDENT, buf);
            continue;
        }

        fprintf(stderr, "Caractere inválido no input: '%c'\n", s[i]);
        exit(1);
    }
    push_tok(ts, T_END, NULL);
}

static Token* peek(TokStream* ts) { return &ts->t[ts->i]; }
static Token* next(TokStream* ts) { return &ts->t[ts->i++]; }

typedef enum { N_VAR, N_NOT, N_AND, N_OR, N_IMPLIES, N_IFF } NodeType;

typedef struct Node {
    NodeType type;
    int var_index;        
    struct Node* l;
    struct Node* r;
} Node;

typedef struct { char name[MAXNAME]; } Var;
typedef struct { Var vars[MAXVARS]; int n; } VarTable;

static int vartable_get(VarTable* vt, const char* name) {
    for (int i = 0; i < vt->n; i++) if (strcmp(vt->vars[i].name, name)==0) return i;
    if (vt->n >= MAXVARS) { fprintf(stderr, "Excedeu limite de %d variáveis.\n", MAXVARS); exit(1); }
    strncpy(vt->vars[vt->n].name, name, MAXNAME-1);
    vt->vars[vt->n].name[MAXNAME-1] = '\0';
    return vt->n++;
}

static Node* parse_expr(TokStream* ts, VarTable* vt);
static Node* parse_impl(TokStream* ts, VarTable* vt);
static Node* parse_or(TokStream* ts, VarTable* vt);
static Node* parse_and(TokStream* ts, VarTable* vt);
static Node* parse_unary(TokStream* ts, VarTable* vt);

static Node* make_node(NodeType t, Node* l, Node* r, int vidx) {
    Node* n = (Node*)malloc(sizeof(Node));
    if (!n) { fprintf(stderr, "Sem memória.\n"); exit(1); }
    n->type = t; n->l = l; n->r = r; n->var_index = vidx;
    return n;
}

static Node* parse_expr(TokStream* ts, VarTable* vt) {
    Node* left = parse_impl(ts, vt);
    while (peek(ts)->type == T_IFF) {
        next(ts);
        Node* right = parse_impl(ts, vt);
        left = make_node(N_IFF, left, right, -1);
    }
    return left;
}

static Node* parse_impl(TokStream* ts, VarTable* vt) {
    Node* left = parse_or(ts, vt);
    while (peek(ts)->type == T_IMPL) {
        next(ts);
        Node* right = parse_or(ts, vt);
        left = make_node(N_IMPLIES, left, right, -1);
    }
    return left;
}

static Node* parse_or(TokStream* ts, VarTable* vt) {
    Node* left = parse_and(ts, vt);
    while (peek(ts)->type == T_OR) {
        next(ts);
        Node* right = parse_and(ts, vt);
        left = make_node(N_OR, left, right, -1);
    }
    return left;
}

static Node* parse_and(TokStream* ts, VarTable* vt) {
    Node* left = parse_unary(ts, vt);
    while (peek(ts)->type == T_AND) {
        next(ts);
        Node* right = parse_unary(ts, vt);
        left = make_node(N_AND, left, right, -1);
    }
    return left;
}

static Node* parse_unary(TokStream* ts, VarTable* vt) {
    Token* p = peek(ts);
    if (p->type == T_NOT) {
        next(ts);
        Node* u = parse_unary(ts, vt);
        return make_node(N_NOT, u, NULL, -1);
    } else if (p->type == T_LP) {
        next(ts);
        Node* e = parse_expr(ts, vt);
        if (peek(ts)->type != T_RP) { fprintf(stderr, "Falta ')'.\n"); exit(1); }
        next(ts);
        return e;
    } else if (p->type == T_IDENT) {
        next(ts);
        int idx = vartable_get(vt, p->ident);
        return make_node(N_VAR, NULL, NULL, idx);
    } else {
        fprintf(stderr, "Token inesperado ao parsear.\n");
        exit(1);
    }
}

static int eval(Node* n, unsigned mask) {
    switch (n->type) {
        case N_VAR: {
            int bit = (mask >> n->var_index) & 1u;
            return bit;
        }
        case N_NOT:     return !eval(n->l, mask);
        case N_AND:     return eval(n->l, mask) && eval(n->r, mask);
        case N_OR:      return eval(n->l, mask) || eval(n->r, mask);
        case N_IMPLIES: {
            int p = eval(n->l, mask), q = eval(n->r, mask);
            return (!p) || q;
        }
        case N_IFF: {
            int p = eval(n->l, mask), q = eval(n->r, mask);
            return p == q;
        }
        default: return 0;
    }
}

static void free_ast(Node* n) {
    if (!n) return;
    free_ast(n->l);
    free_ast(n->r);
    free(n);
}

static void strip_nl(char* s) {
    size_t L = strlen(s);
    while (L && (s[L-1]=='\n' || s[L-1]=='\r')) { s[L-1]='\0'; L--; }
}

int main(void) {
    char line1[8192], line2[8192];
    printf("Digite a primeira sentença:\n");
    if (!fgets(line1, sizeof(line1), stdin)) return 1;
    printf("Digite a segunda sentença:\n");
    if (!fgets(line2, sizeof(line2), stdin)) return 1;
    strip_nl(line1); strip_nl(line2);

    TokStream ts1, ts2;
    VarTable vt; vt.n = 0;

    lex(line1, &ts1);
    Node* a = parse_expr(&ts1, &vt);

    lex(line2, &ts2);
    Node* b = parse_expr(&ts2, &vt);

    if (vt.n > MAXVARS) { fprintf(stderr, "Variáveis demais.\n"); return 1; }

    unsigned total = 1u << vt.n;
    int eq = 1;
    for (unsigned m = 0; m < total; m++) {
        int va = eval(a, m);
        int vb = eval(b, m);
        if (va != vb) { eq = 0; break; }
    }

    if (eq) {
        printf("As sentenças são LOGICAMENTE EQUIVALENTES.\n");
    } else {
        printf("As sentenças NÃO são logicamente equivalentes.\n");
    }

    free_ast(a);
    free_ast(b);
    return 0;
}
