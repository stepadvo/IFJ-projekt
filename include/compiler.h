/**
 * @file compiler.h
 * @brief header for declarations for all the modules
 * @author Štěpán Dvořák, xdvoras@stud.fit.vutbr.cz
 */

#ifndef COMPILER_H
#define COMPILER_H

#include <stddef.h>

/** @brief enum of the possible data types in IFJ26 */
typedef enum { UNIT, INT, DOUBLE, STRING, BOOL, UNKNOWN, ANY } varType;

/** @brief enum of the possible error types */
typedef enum {
    ERR_LEXER = 1,
    ERR_SYNTAX = 2,
    ERR_SEM_UNDEF = 3,
    ERR_SEM_REDEF = 4,
    ERR_SEM_ARG = 5,
    ERR_SEM_TYPE = 6,
    ERR_SEM_RONLY = 7,
    ERR_SEM_OTHER = 10,
    ERR_INTERNAL = 99
} errType;

/** @brief global error code */
extern int ERRCODE;

/** @brief sets the global errcode to errCode if it hasn't been set yet */
void setErrCode(int errCode);

/** @brief a value for telling logErr to not show either line or column */
#define LOGERR_NO_POS 0
/** @brief a value for telling logErr not to show further message */
#define LOGERR_NO_DETMSG NULL

/**
 * @brief sets an appropriate error code and outputs an error message
 *
 * @param type type of error (will also be set to the ERRCODE)
 * @param line line of the error in source file
 * @param column column of the error in source file
 * @param detailMsgFormat formatting string (as in printf()) for the detailed
 * message
 * @param ... additional parameters for the foramatted string
 *
 * the fuction will write out a brief message describing the error (including
 * type, line and column) if the type is ERR_INTERNAL, the line, column and
 * detailMsg will be ignored if column is 0, only the line will be shown if line
 * is 0, no data about position in source file will be shown if detailMsgFormat
 * is NULL, no further info will be outputted the additional info can describe
 * the specific error or a tip for fixing it tabulators will be added before
 * every line of the detailMsgFormat (so it is indented) the additional
 * parameters must match detailMsgFormat to avoid undefined behaviour
 */
void logErr(errType type, size_t line, size_t column,
            const char *detailMsgFormat, ...);

#endif // !COMPILER_H

/*
 * end compiler.h
 */
