/**
 * @file ast_print.h
 * @brief Human-readable tree printer for debugging the IFJ26 AST.
 * @author Jiří Dohnal, xdohnaj00@stud.fit.vutbr.cz
 */

#ifndef AST_PRINT_H
#define AST_PRINT_H

#include "ast.h"

/**
 * @brief Prints the AST as a tree to standard output.
 * @param root Root node of the tree. NULL is allowed.
 *
 * The function only reads the tree and does not change or free any nodes.
 */
void printAst(astNode *root);

#endif // AST_PRINT_H
