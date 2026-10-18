CC = gcc
CXX = g++
CFLAGS = -Wall -Werror -Wextra -Iinclude -std=c11
CXXFLAGS = -Wall -Werror -Wextra -Iinclude -std=c++17
LDFLAGS = -lgtest -lgtest_main -pthread -lstdc++

VPATH=src:test:include

#targets:
#add the build targets here
TEST_BST = test_bst
TEST_SYMTABLE = test_symtable

#add all test targets
TEST_STEMS = TEST_BST TEST_SYMTABLE

#objects:
BUILD_O = bst.o symtable.o	#add all further objects needed for the whole application
TEST_BST_O = bst.o test_bst.o
TEST_SYMTABLE_O = symtable.o test_symtable.o

#rules:
#add the build rules here
all: build

build:

clean:
	rm -f *.o *.out

format:
	clang-format -i include/* src/* test/*

test: $(TEST_BST)_build
	./$(TEST_BST)
	./$(TEST_SYMTABLE)

define DEFINE_TEST_RULES =
$($(1)): $($(1))_build
	./$($(1)).out

$($(1))_build: $($(1)_O)
	$(CC) $(LDFLAGS) -o $($(1)).out $($(1)_O)
endef

$(foreach t,$(TEST_STEMS),$(eval $(call DEFINE_TEST_RULES,$(t))))

.PHONY: all clean format

