/**
 * @file compiler.h
 * @brief header for declarations for all the modules
 * @author Štěpán Dvořák, xdvoras@stud.fit.vutbr.cz
 */

#ifndef COMPILER_H
    #define COMPILER_H

/** @brief enum of the possible data types in IFJ26 */
typedef enum { UNIT, INT, DOUBLE, STRING, BOOL, UNKNOWN, NUMERIC, ANY } varType;

extern int ERRCODE;

#endif // !COMPILER_H

/*
 * end compiler.h
 */
