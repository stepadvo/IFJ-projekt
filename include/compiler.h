/**
 * @file compiler.h
 * @brief header for declarations for all the modules
 * @author Štěpán Dvořák, xdvoras@stud.fit.vutbr.cz
 */

#ifndef COMPILER_H
#define COMPILER_H

/** @brief enum of the possible data types in IFJ26 */
typedef enum { UNIT, INT, DOUBLE, STRING, BOOL, UNKNOWN, ANY } varType;

/** @brief global error code */
extern int ERRCODE;

/** @brief sets the global errcode to errCode if it hasn't been set yet */
void setErrCode(int errCode);

#endif // !COMPILER_H

/*
 * end compiler.h
 */
