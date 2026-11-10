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

#add all test targets
TEST_STEMS = TEST_BST TEST_SYMTABLE # TEST_EXAMPLE ...

#objects:
BUILD_O = compiler.o parser.o ast.o lexer.o token.o symtable.o bst.o
#TEST_EXAMPLE_O = test_example.o src_example.o
TEST_BST_O = bst.o test_bst.o errcode.o
TEST_SYMTABLE_O = bst.o symtable.o test_symtable.o errcode.o

#rules:
#add the build rules here
all: build

build: $(BUILD_O)
	$(CC) $(CFLAGS) -o compiler.out $(BUILD_O)

clean:
	rm -f *.o *.out

format:
	clang-format -i include/* src/* test/*

test: $(foreach t,$(TEST_STEMS),$($(t))_build)
	$(foreach t,$(TEST_STEMS), ./$($(t)).out)

define DEFINE_TEST_RULES =
$($(1)): $($(1))_build
	./$($(1)).out

$($(1))_build: $($(1)_O)
	$(CC) $(LDFLAGS) -o $($(1)).out $($(1)_O)
endef

$(foreach t,$(TEST_STEMS),$(eval $(call DEFINE_TEST_RULES,$(t))))

.PHONY: all clean format

