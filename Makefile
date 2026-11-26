CC = gcc
CXX = g++
CFLAGS = -Wall -Werror -Wextra -Iinclude -std=c11 -g
CXXFLAGS = -Wall -Werror -Wextra -Iinclude -std=c++17 -g
LDFLAGS = -lgtest -lgtest_main -pthread -lstdc++

VPATH=src:test:include

#targets:
#add the build targets here
#TEST_EXAMPLE = test_example
TEST_BST = test_bst
TEST_SYMTABLE = test_symtable
TEST_AST = test_ast
TEST_LEXER = test_lexer
TEST_TOKEN = test_token
TEST_GEN = test_gen
TEST_SEM = test_semantics
TEST_EXPRESSION = test_expression

#add all test targets
TEST_STEMS = TEST_BST TEST_SYMTABLE TEST_AST TEST_LEXER TEST_TOKEN TEST_GEN TEST_SEM TEST_EXPRESSION # TEST_EXAMPLE ...

#objects:
OBJECTS = compiler.o parser.o ast.o lexer.o token.o symtable.o bst.o errcode.o expression.o internalSemantic.o semantics.o builtins.o collectFunctionHeaders.o generator.o
#TEST_EXAMPLE_O = test_example.o src_example.o
TEST_BST_O = bst.o test_bst.o errcode.o
TEST_SYMTABLE_O = bst.o symtable.o test_symtable.o errcode.o
TEST_AST_O = ast.o test_ast.o
TEST_LEXER_O = test_lexer.o lexer.o token.o errcode.o
TEST_TOKEN_O = test_token.o token.o
TEST_GEN_O = ast.o symtable.o bst.o generator.o test_generator.o errcode.o
TEST_SEM_O = test_semantics.o internalSemantic.o semantics.o builtins.o collectFunctionHeaders.o \
             ast.o symtable.o bst.o generator.o errcode.o token.o
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

test: $(foreach t,$(TEST_STEMS),$($(t))_build)
	@set -e; $(foreach t,$(TEST_STEMS),./$($(t)).out;)

printast:
	$(MAKE) clean
	$(MAKE) CFLAGS="$(CFLAGS) -DDEBUG_AST" all
	rm -f *.o

test_generator_function: generator.o test_generator_functional.o symtable.o errcode.o bst.o ast.o
	$(MAKE) clean
	$(MAKE) CFLAGS="$(CFLAGS) -DIFJ_GEN_DEBUG -DIFJ_AST_NOOWN" $^
	$(CC) $(CFLAGS) -o $@.out $^
	./test_generator.sh

define DEFINE_TEST_RULES =
$($(1)): $($(1))_build
	./$($(1)).out

$($(1))_build: $($(1)_O)
	$(CXX) -o $($(1)).out $($(1)_O) $(LDFLAGS)
endef

$(foreach t,$(TEST_STEMS),$(eval $(call DEFINE_TEST_RULES,$(t))))

.PHONY: all clean format printast test test_generator_function
