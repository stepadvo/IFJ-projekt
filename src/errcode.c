/**
 * @file compiler.c
 * @brief a top level file for the most basic routines
 * @author Štěpán Dvořák, xdvoras@stud.fit.vutbr.cz
 *
 * detailed description
 */

#include "compiler.h"

int ERRCODE;

void setErrCode(int errCode) {
    if (!ERRCODE)
        ERRCODE = errCode;
}

/*
 * end compiler.c
 */
