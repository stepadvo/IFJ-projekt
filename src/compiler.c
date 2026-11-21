/**
 * @file compiler.c
 * @brief a top level file for the most basic routines
 * @author Štěpán Dvořák, xdvoras@stud.fit.vutbr.cz
 *
 * detailed description
 */

#include "compiler.h"
#include <stdio.h>
#include <stdlib.h>

int main(int argc, char *argv[]) {
    ERRCODE = 0;

    // TODO call parser

    if (ERRCODE != EXIT_SUCCESS) {
        fprintf(stderr, "Compilation terminated\n");
    }
    return ERRCODE;
}

/*
 * end compiler.c
 */
