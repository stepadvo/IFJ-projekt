#include "compiler.h"
#include "ast.h"
#include "parser.h"
#include "symtable.h"
#include "parser.h"
#include <stdio.h>
#include <stdlib.h>

int ERRCODE;

int main(int argc, char *argv[]) {
    ERRCODE = 0;

    (void)argc;
    (void)argv;

    // 1. Initialize the global symbol table
    if (symTableInit() != 0) {
        fprintf(stderr, "Internal error: Failed to init symbol table\n");
        return 99;
    }

    // SA
    astNode *ast_root = parse();

    // TODO: Semantic analysis
    // TODO: Code generator

    // Free AST
    if (ast_root != NULL) {
        freeAst(ast_root);
    }
    symTableDestruct();

    if (ERRCODE != 0) {
        fprintf(stderr, "\
==================================\n\
 Compilation ended unsuccessfully.\n\
         output not valid         \n\
==================================\n\
");
    }

    return ERRCODE;
}
