CC = gcc
CXX = g++
CFLAGS = -Wall -Werror -Wextra -Iinclude -std=c11
CXXFLAGS = -Wall -Werror -Wextra -Iinclude -sdt=c++14
LDFLAGS = -lgtest -lgtest_main -ptrhead

#add the flags for clang-format here
CFORMATFLAGS =

#targets:
#add the build targets here

#rules:
#add the build rules here
all: build

build:

clean:

format:

.PHONY: all clean format


