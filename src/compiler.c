#include "compiler.h"
#include "ast.h"
#include "ast_print.h"
#include "parser.h"
#include "semantics.h"
#include <stdio.h>
#include <stdlib.h>

int main(int argc, char *argv[]) {
    (void)argc;
    (void)argv;
    ERRCODE = 0;

    // read the input and parse it into the ast
    astNode *root = parse();

    // check the semantic correctness of the ast and generate it
    semanticAnalyze(root);

#ifdef DEBUG_AST
    printAst(root);
#endif

    freeAst(root);

    if (ERRCODE != EXIT_SUCCESS) {
        fprintf(stderr, "\
==================================\n\
 Compilation ended unsuccessfully.\n\
==================================\n\
");
    }

    return ERRCODE;
}
