.DEFAULT_GOAL := run_main
#if we run make without a target, this is the default target
ROOT := /usr/bin/
SELF =  memory_management/
PATH1 := lib/
#user and library paths
SUDOO = /bin/sh
CC := gcc
CCPLUS := g++
RM := rm -f
CHMOD := chmod
RUN = ./
CHANGE_DIR = cd
WHERE = ../
SUDO = sudo
#shell commands to be run

SLASH = /
STAR := *
CODE = .c
C++ = .cpp
OBJECT = .o
HEADER = .h
TILDE = ~
LINE = -
EQUALS = =
#utils

MEM_LIB = memory_manager
MEM_LIB_U = memory_manager_u
STRUCT_LIB = list
MATH = my_math
PROGRAM = partial_test
PROGRAM_REMOTE = remote_free_test

EXECUTABLE_NAME = main
#names for lib and prog files
EXPORT = export
COMPILE_MEM = -g -fPIC -c
COMPILE = -g -c
COMPILE_DIR = -g
WARN = -Wall
LINK = -o
SHARED_OBJ = .so
CCFLAGS = -shared
POSTCCFLAGS = -ldl
FLAGS = -lm -lpthread
PERMS = 777
PERM_FLAGS = -R
RUN = ./
AT = :
DEF = -D MALLOC
LPRELOAD = LD_PRELOAD
LIB_PATH = LD_LIBRARY_PATH
ULIB = -L./lib/ -l:memory_manager_u.so #-ljemalloc
#flags and permissions
run_main: clean linker compile_libraries
#run these targets sequentially
linker:

compile_libraries:
	$(CCPLUS) $(WARN) $(DEF) $(COMPILE_MEM) $(PATH1)$(MEM_LIB)$(CODE) $(LINK) $(PATH1)$(MEM_LIB)$(OBJECT)
	$(CCPLUS) $(WARN) $(COMPILE_MEM) $(PATH1)$(MEM_LIB)$(CODE) $(LINK) $(PATH1)$(MEM_LIB_U)$(OBJECT)
	$(CCPLUS) $(WARN) $(COMPILE) $(PATH1)$(STRUCT_LIB)$(CODE) $(LINK) $(PATH1)$(STRUCT_LIB)$(OBJECT) 
	$(CC) $(WARN) $(COMPILE) $(PATH1)$(MATH)$(CODE) $(LINK) $(PATH1)$(MATH)$(OBJECT) -lm
	$(CCPLUS) $(CCFLAGS) $(LINK) $(PATH1)$(MEM_LIB)$(SHARED_OBJ) $(PATH1)$(MEM_LIB)$(OBJECT) $(PATH1)$(STRUCT_LIB)$(OBJECT) $(PATH1)$(MATH)$(OBJECT)
	$(CCPLUS) $(CCFLAGS) $(LINK) $(PATH1)$(MEM_LIB_U)$(SHARED_OBJ) $(PATH1)$(MEM_LIB_U)$(OBJECT) $(PATH1)$(STRUCT_LIB)$(OBJECT) $(PATH1)$(MATH)$(OBJECT)
	$(CC) $(WARN) $(COMPILE_DIR) $(PROGRAM)$(CODE) $(LINK) $(PROGRAM) $(FLAGS) $(ULIB)
	$(CC) $(WARN) $(COMPILE_DIR) $(PROGRAM_REMOTE)$(CODE) $(LINK) $(PROGRAM_REMOTE) $(FLAGS) $(ULIB)
##	$(CC) $(PROGRAM)$(OBJECT) $(PATH1)$(MEM_LIB)$(SHARED_OBJ) $(LINK) $(EXECUTABLE_NAME) $(FLAGS)
	g++ alloc-test/src/test_common.cpp alloc-test/src/allocator_tester.cpp -std=c++17 -g -Wall -Wextra -Wno-unused-variable -Wno-unused-parameter -Wno-empty-body -DNDEBUG -O2 -flto -fno-builtin-malloc -fno-builtin-calloc -fno-builtin-realloc -fno-builtin-free  $(ULIB) -lpthread -o test
	
clean:
	$(RM) $(EXECUTABLE_NAME) 
	$(RM) $(STAR)$(OBJECT)
	$(RM) $(PATH1)$(STAR)$(OBJECT)
	$(RM) $(PATH1)$(STAR)$(SHARED_OBJ)
	rm -f test
	rm -f remote_free_test
	rm -f partial_test
