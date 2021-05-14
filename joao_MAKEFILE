# My first, crappy attempt at making a makefile for Joao.
# So scared about this working that I'm too timid to use a circumflex on the a, there.
# Typing 'make' or 'make count' will create the executable file.
#

# define some Makefile variables for the compiler and compiler flags
# to use Makefile variables later in the Makefile: $()
#
#  -g    adds debugging information to the executable file
#  -Wall turns on most, but not all, compiler warnings
#
# for C++ define  CC = g++
CC = g++

WARNINGS = -Waddress -Warray-bounds=1 -Wbool-compare -Wbool-operation -Wcatch-value -Wchar-subscripts -Wcomment -Wformat -Wformat-overflow -Wformat-truncation -Wint-in-bool-context -Winit-self -Wlogical-not-parentheses -Wmaybe-uninitialized -Wmemset-elt-size -Wmemset-transposed-args -Wmisleading-indentation -Wmissing-attributes -Wmultistatement-macros -Wnarrowing -Wnonnull -Wnonnull-compare -Wopenmp-simd -Wparentheses -Wpessimizing-move -Wrestrict -Wreturn-type -Wsequence-point -Wsizeof-pointer-div -Wsizeof-pointer-memaccess -Wstrict-aliasing -Wstrict-overflow=1 -Wswitch -Wtautological-compare -Wtrigraphs -Wuninitialized -Wunknown-pragmas -Wunused-function -Wunused-label -Wunused-value -Wunused-variable -Wvolatile-register-var

CFLAGS = -g $(WARNINGS)

# typing 'make' will invoke the first target entry in the file 
# (in this case the default target entry)
# you can name this target entry anything, but "default" or "all"
# are the most commonly used names by convention
#
default: joao

# To create the executable file count we need the object files
joao:  Args.o AST.o Directory.o Interpreter.o main.o math.o string.o tablelib.o Object.o Parser.o Program.o Scanner.o Table.o
	$(CC) $(CFLAGS) -o joao Args.o AST.o Directory.o Interpreter.o main.o Object.o Parser.o Program.o Scanner.o Table.o

Args.o: Args.h Scanner.h Parser.h Interpreter.h
	$(CC) $(CFLAGS) -c Args.cpp

AST.o: AST.cpp Forward.h AST.h Object.h Interpreter.h Parser.h Table.h
	$(CC) $(CFLAGS) -c AST.cpp

Directory.o: Directory.cpp Forward.h Directory.h
	$(CC) $(CFLAGS) -c Directory.cpp

Interpreter.o: Interpreter.cpp Forward.h AST.h Interpreter.h
	$(CC) $(CFLAGS) -c Interpreter.cpp

main.o: main.cpp Forward.h AST.h Interpreter.h Object.h Parser.h Args.h
	$(CC) $(CFLAGS) -c main.cpp
	
file.o: ./nativefuncs/file.cpp Program.h Object.h Table.h
	$(CC) $(CFLAGS) -c ./nativefuncs/file.cpp
	
math.o: ./nativefuncs/math.cpp Program.h
	$(CC) $(CFLAGS) -c ./nativefuncs/math.cpp

string.o: ./nativefuncs/string.cpp Program.h
	$(CC) $(CFLAGS) -c ./nativefuncs/string.cpp

tablelib.o: ./nativefuncs/tablelib.cpp Program.h Object.h
	$(CC) $(CFLAGS) -c ./nativefuncs/tablelib.cpp

Object.o: Object.cpp Object.h Interpreter.h Parser.h Table.h
	$(CC) $(CFLAGS) -c Object.cpp

Parser.o: Parser.cpp Parser.h Object.h Directory.h ObjectTree.h
	$(CC) $(CFLAGS) -c Parser.cpp

Program.o: Program.cpp Program.h
	$(CC) $(CFLAGS) -c Program.cpp

Scanner.o:  Scanner.cpp Scanner.h 
	$(CC) $(CFLAGS) -c Scanner.cpp
	
Table.o: Table.cpp Table.h Interpreter.h
	$(CC) $(CFLAGS) -c Table.cpp

# To start over from scratch, type 'make clean'.  This
# removes the executable file, as well as old .o object
# files and *~ backup files:
#
clean: 
	$(RM) joao *.o *~