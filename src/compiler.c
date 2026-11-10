#include "compiler.h"
#include "ast.h"
#include "ast_print.h"
#include "symtable.h"
#include "parser.h"
#include <stdio.h>
#include <stdlib.h>

int main(int argc, char *argv[]) {
    (void)argc;
    (void)argv;
    ERRCODE = 0;

    astNode *root = parse();

    #ifdef DEBUG_AST
    printAst(root);
    #endif

    freeAst(root);

    if (ERRCODE != EXIT_SUCCESS) {
        fprintf(stderr, "\
==================================\n\
 Compilation ended unsuccessfully.\n\
         output not valid         \n\
==================================\n\
");
    }

    return ERRCODE;
}
