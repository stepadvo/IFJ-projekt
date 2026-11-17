CC = gcc
CXX = g++
CFLAGS = -Wall -Werror -Wextra -Iinclude -std=c11
CXXFLAGS = -Wall -Werror -Wextra -Iinclude -std=c++17
LDFLAGS = -lgtest_main -lgtest -pthread

VPATH=src:test:include

#targets:
TEST_LEXER = test_lexer
TEST_TOKEN = test_token

TEST_STEMS = TEST_LEXER TEST_TOKEN

#objects:
TEST_LEXER_O = test_lexer.o lexer.o token.o errcode.o
TEST_TOKEN_O = test_token.o token.o

#rules:
all: build

build:

clean:
	rm -f *.o *.out

format:
	clang-format -i include/* src/* test/*

lexer.o test_lexer.o: include/lexer.h include/token.h include/compiler.h
token.o test_token.o: include/token.h
errcode.o: include/compiler.h

test: $(foreach t,$(TEST_STEMS),$($(t)))

define DEFINE_TEST_RULES =
$($(1)): $($(1))_build
	./$($(1)).out

$($(1))_build: $($(1)_O)
	$(CXX) -o $($(1)).out $($(1)_O) $(LDFLAGS)
endef

$(foreach t,$(TEST_STEMS),$(eval $(call DEFINE_TEST_RULES,$(t))))

.PHONY: all clean format test \
	$(foreach t,$(TEST_STEMS),$($(t)) $($(t))_build)

