/**
 * @file parser.h
 * @brief Public interface of the IFJ26 parser.
 * @author Jiří Dohnal, xdohnaj00@stud.fit.vutbr.cz
 */

#ifndef PARSER_H
#define PARSER_H

#include "ast.h"
#include "symtable.h"

/**
 * @brief Reads source code from stdin and builds an AST.
 * @return AST root on success, NULL on error.
 *         The caller is responsible for calling freeAst().
 *
 * Currently only the mandatory prolog is parsed. Parsing functions,
 * blocks and expressions will be added in later commits.
 */
astNode *parse(void);

#endif // PARSER_H
