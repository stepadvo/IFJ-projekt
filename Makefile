CC = gcc
CXX = g++
CFLAGS = -Wall -Werror -Wextra -Iinclude -std=c11
CXXFLAGS = -Wall -Werror -Wextra -Iinclude -std=c++17
LDFLAGS = -lgtest_main -lgtest -pthread

VPATH=src:test:include

#targets:
TEST_LEXER = test_lexer
TEST_TOKEN = test_token
#add the build targets here
#TEST_EXAMPLE = test_example
TEST_BST = test_bst
TEST_SYMTABLE = test_symtable
TEST_AST = test_ast
TEST_EXPRESSION = test_expression
#TEST_PARSER = test_parser


#add all test targets
TEST_STEMS = TEST_BST TEST_SYMTABLE TEST_AST TEST_EXPRESSION TEST_LEXER TEST_TOKEN #TEST_PARSER

#objects:
TEST_LEXER_O = test_lexer.o lexer.o token.o errcode.o
TEST_TOKEN_O = test_token.o token.o
OBJECTS = compiler.o parser.o ast.o lexer.o token.o symtable.o bst.o errcode.o ast_print.o expression.o
#TEST_EXAMPLE_O = test_example.o src_example.o
TEST_BST_O = bst.o test_bst.o errcode.o
TEST_SYMTABLE_O = bst.o symtable.o test_symtable.o errcode.o
TEST_AST_O = ast.o test_ast.o
TEST_EXPRESSION_O = expression.o ast.o test_expression.o
#TEST_PARSER_O = parser.o expression.o ast.o lexer.o token.o errcode.o test_parser.o

#rules:
all: build

build: $(OBJECTS)
	$(CC) $(CFLAGS) -o compiler.out $(OBJECTS)

clean:
	rm -f *.o *.out

format:
	clang-format -i include/* src/* test/*

lexer.o test_lexer.o: include/lexer.h include/token.h include/compiler.h
token.o test_token.o: include/token.h
errcode.o: include/compiler.h

test: $(foreach t,$(TEST_STEMS),$($(t))_build)
	@set -e; $(foreach t,$(TEST_STEMS),./$($(t)).out;)

printast:
	$(MAKE) clean
	$(MAKE) CFLAGS="$(CFLAGS) -DDEBUG_AST" all
	rm -f *.o

define DEFINE_TEST_RULES =
$($(1)): $($(1))_build
	./$($(1)).out

$($(1))_build: $($(1)_O)
	$(CXX) -o $($(1)).out $($(1)_O) $(LDFLAGS)
endef

$(foreach t,$(TEST_STEMS),$(eval $(call DEFINE_TEST_RULES,$(t))))

.PHONY: all clean format test printast
	$(foreach t,$(TEST_STEMS),$($(t)) $($(t))_build) printast test