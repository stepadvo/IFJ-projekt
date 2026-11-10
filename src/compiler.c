#include "compiler.h"
#include "ast.h"
#include "parser.h"
#include "symtable.h"
#include "parser.h"
#include <stdio.h>
#include <stdlib.h>

int main(int argc, char *argv[]) {
    (void)argc;
    (void)argv;
    ERRCODE = 0;

    // TODO call parser

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
