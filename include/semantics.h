/** 
 * @file semantic.h
 * @brief Interface of the semantic analysis of the IFJ26 AST
 * @author Kristian Havranek, xhravrak00
 *
 * Project IFJ26 compiler
*/
#ifndef SEMANTIC_H
#define SEMANTIC_H

#include "ast.h"

/**
 * @brief runs semantic analysis over the whole AST
 * @param root AST_PROGRAM node
 * @return 0 on success; on semantic err the function reports it to 
             stderr and exits with the matching code (3, 4, 5, 6. 7, 10)    
 
*/


int semanticAnalyze(astNode *root);

#endif