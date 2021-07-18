# 1. Getting Started

### Quickstart

To keep with tradition, our first program in João just prints "Hello world!" into the command line.

```dm
/main()
{
	print("Hello world!");
}
```

If you are using the stand-alone João interpreter, running your first program means calling the interpreter (usually named joao on Linux or Joao.exe on Windows) with the name of a text file that contains your program as the binary's first argument. For instance, if you write the above program in a file hello.jao, the following command should run it in your command line:

```
Joao.exe hello.jao
```

or on Linux:

```
joao hello.jao
```

More complex programs can also be made in João. For instance, the following is a program which prints the value of the factorial of seven:

```python
/fact(n)
{
	if(n == 0)
	{
		return 1;
	}
	
	return n * fact(n - 1);
}

/main()
{
	print(fact(7));
}
```

### How to read this documentation

As João is a programming language, with many powerful features, many things are possible to do using it. Mathematics, video games, whatever. The following guide is an attempt to explain the concepts needed to do these things in clear language, with the goal of being readable by someone who is very new to programming.

As with any documentation for a programming language, it is recommended to not read the entire documentation in one sitting; rather, it is better to learn a particular concept or idea and then try to write code which features that new concept, to experiment with its syntax and its behavior.


# 1.1 - Directories and /main()

João is a programming language, and so, unsurprisingly, you can make **programs** in it.

Often, very large programs have an issue of organizing all their code in a reasonable and human-readible way, even when the code expands to become millions of lines of text long. João's solution to that problem is to use some behavior inherited form the concept of a **Directory.**

João is, in some sense, a **directory-oriented language.** This is to mean that much of the structure of your program is organized into something analogous to the directories of some file system, like the files on a Windows or Linux computer.

These directories can contain **classes** and **functions**. For example, ``/main()`` is a *function* which is supposed to be the start of your program; when your program is first run, the code within ``/main()`` is the first, and sometimes only, code that is ran. An example of how ``/main()`` looks is featured above in the first two code examples.

# 1.2 - Global Variables

Within ``/main()``, and in many other places within our program, we can create **variables** which are used to store things in memory for later.

For example, we can store numbers into such a variable, and then do math with that variable:

```python
/main()
{
    /x = 7;
    print(3 + x); ## Prints "10"
}
```

The ``/`` before the variable's name is used to indicate that it is a **Global Variable**, accessable at the "root" directory. This means that all code within the program has access to the variable, even things not in ``/main()``, and can change or read its value at any time.

# 1.3 - Lexical Conventions

## Naming things

Variable names, as well as class and function names and whatever else, can be any string of letters, digits, and underscores, as long as it does not begin with a digit. Examples include:

```
idea
jester
variable10
_i
aFunCamelCaseVariable
_GILBERTO
```

These identifiers are **case-sensitive**. This means that ``A`` and ``a`` are treated as different identifiers by João, and can mean different things.

As of João v1.1, Only ASCII letters are available for identifiers. This is a bug, not a feature, and alternative letters will become available for later versions.

The following words are *reserved* by the language, to be used as **Keywords** for particular actions that can be done by the language, and so cannot be used as identifiers:

```
if elseif else
for while
return break
Value Object Number String Boolean local
true false null
include require
```

Since João is case-sensitive, it is possible (albeit discouraged) to use identifiers such as IF or oBJECT without technically using a reserved word.

## Comments

**Comments** are text which is not part of the program itself, but written into the code so as to, typically, provide some information written in English (or whatever) that helps you (or someone else) understand your code better.

To create a comment which exists on a single line of code, you can write ``##`` before the text to mark that everything past it is a comment, like so:

```python
/main()
{
	/what = 3; ## I am a comment!
	print(what);
}
```

Sometimes you may want to make a comment which spans multiple lines of text. To make such a **long comment**, write ``###`` at the very beginning of a line. Everything on or past that line will be comments until you write a second ``###`` line at the end of the comment. A quick example:

```python
/main()
{
###
Olha que coisa mais linda, mais cheia de graça
É ela a menina que vem e que passa
###
	print("Hello!");
}
```

Starting a long comment on anything but the very beginning of a line (ignoring whitespace) is forbidden. Further, long comments cannot occur on the same line as functional code.
