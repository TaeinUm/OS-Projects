## [M1]
#  Set CUR_DIR to the current directory of the Makefile using shell commands.
#  ...
CUR_DIR := $(shell dirname $(realpath $(lastword $(MAKEFILE_LIST))))

## [M2]
#  Define the C compiler (CC) and its associated flags.
#  ...
CC = gcc
CFLAGS += -g -O2 -Werror -Wall
LDFLAGS +=

## [M3] -- REMOVED
#  ...

## [M5]
#  Use the `wildcard` function to find all C files in the current directory.
#  ...
SRC_FILES = $(wildcard *.c)

## [M6]
#  Create a list of executable file names by replacing the ".c" extension with an empty string.
#  ...
EXE_FILES = $(SRC_FILES:.c=)

## [M7]
#  Define a default target 'all' which depends on all the executable files (EXE_FILES).
#  ...
all: $(EXE_FILES)

## [M8] -- UPDATED
#  Define a generic rule to build an executable from a C source file.
#  ...
%:%.c
	$(CC) $(CFLAGS) $(LDFLAGS) -o $@ $< 

## [M9]
#  Define 'clean' to remove all the executable files.
#  ...
clean:
	rm -f $(EXE_FILES)

## [M10]
#  Declare the 'all' and 'clean' targets as phony.
#  ...
.PHONY: all clean
