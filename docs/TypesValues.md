# 2. Types and Values

João is a **dynamically-typed language.** Every variable can store any arbitrary data you can stuff in it.

That being said, when you stuff something into a variable, that variable then has data within it that has a type.

As of version 1.1, there are six types in João: **Null**, **Boolean**, **Double**, **Integer**, **String**, and **Object**.

- **Null**, a type (and value) which indicates a lack of data. If you declare a variable but do not set it to anything, or try to access something that doesn't exist, the value you will receive is ``null`` and its type will be **Null**.

- **Boolean**, which stores a singular bit of data, with two possible values: ``true`` and ``false``.

- **Double**, which stores a number in a sort of scientific notation internally. This type is used to store very large numbers and fractions.

- **Integer**, which stores some integer number. It can be positive or negative or zero, but it can be no bigger than 2,147,483,647 and no smaller than -2,147,483,648.

- **String**, which stores text. Strings are created by encasing the text in quotes, "like this". More on that in 2.1.

- **Object**. which stores a value that has a "type" created by you (or a library of João) with certain special functionality. This includes things like **tables**, which are used to store groups of data, and **files**, which are handles to an actual file on the computer. More on that in 2.2, 2.3, and 2.4.

```python
/main()
{
	print(typeof(7)); ## Prints "Integer"
	
	print(typeof("Hello world!")); ## Prints "String"
	
	print(typeof(1.5 * 2)); ## Prints "Double"
	
	print(typeof(null)); ## Prints "Null"
	
	print(/table/New()); ## Prints "Object"
}
```

# 2.1 - Strings

**Strings** have the usual meaning: a sequence of characters, like the word "apple." Each character is any eight-bit number, and so strings may contain characters with any numeric value, but are typically assumed to contain ASCII text.

## Escape Sequences

**Escape Sequences** allow a string to contain special characters. For instance, if you wanted to actually have a ``"`` character within a string, it would need to be escaped so that it isn't perceived as being the end of the string. Strings in João can contain the following C-like escape sequences:

Escape Character | Meaning
---------------- | -------
\\n | newline
\\t | tab
\\  | backslash
\\" | double quote
\\' | single quote

## Operations on Strings

### Concatenation

Strings have a special operator for pasting two strings together, called **concatenation**, which uses the ``..`` symbol. It can be used like any typical math or logic operator:

```python
/main()
{
	print("Hello" .. "world"); ## Prints "Helloworld".
	## If you want "Hello world", you actually need to put a space character in one of these strings!
}
```

When the concatenation operator is used with things that are not strings, those things are **coerced** into being strings:

```python
/main()
{
	print(10 .. 1); ## Prints "101"
	
	/x = 3;
	print("x's value is " .. x .. "!"); ## Prints "x's value is 3!"
}
```

To convert anything into a string, you may call the function ``tostring(x)`` on the given value.

### Logical Operations

Despite those automatic conversions, strings and numbers are different things. A comparison like 10 == "10" is always false, because 10 is an integer and "10" is a string. As of version 1.1, there is no native function for converting a string into a number.

# 2.2 - Objects

**Objects** are the bread and butter of João. They use the directory system to organize groups of code and variables into which can be used together to form some greater, more complex thing.

Since their concept is a bit abstract, lets start with a concrete example. Lets say you want to make a video game, and ergo might want something that can store the state of a particular player. You may want to, on a player-by-player basis, store things like their health and position on the map. Here's an object-oriented way of storing such data:

```python
/player
{
	Number health = 100; ## The player's health, out of 100 points

	Number x = 0; ## The x-coordinate of this player
	Number y = 0; ## y-coordinate
	Number z = 0; ## z-coordinate
}

/main()
{
	Object my_player = /player/New();
	print("My player has " .. my_player.health .. " HP!"); ## Prints "My player has 100 HP!"
}
```

There's several steps in the creation of an object:

1. The object must have a **class** that is defined in a **class definition**, a braced block containing the class' name and a list of its properties, little variables that belong to any object created from that class.

2. Once the class is defined, an object of that class can be created anywhere by doing ``class_name/New()``, with ``class_name`` being whatever the directory name of the class is. ``New()`` is a function inherent to all classes, which creates an Object with that class as its class.

## Inheritence

Further, classes can "inherit" from other classes via the directory system, like so:

```python
/player
{
	Number health = 100;

	Number x = 0;
	Number y = 0;
	Number z = 0;
}

/player/red
{
	String team = "Red";
}

/player/blue
{
	String team = "Blue";
}

/main()
{
	Object redmann = /player/red/New();
	Object blumann = /player/blue/New();
	
	print(blumann.team .. " team hates " .. redmann.team .. " team!"); ## Prints "Blue team hates Red team!"
}

```

[Object-oriented programming](https://en.wikipedia.org/wiki/Object-oriented_programming) is a large and complicated subject, too large to discuss meaningfully here, but this is the syntax one can use in João to carry it out.

Further, this behavior is used to implement several important things in João: files and tables.

# 2.3 - Files

**Files** are Objects which are used to allow João code to read, write, and create files on the computer it runs on.

The following are the key functions (or *methods*) that files have which are used to interface with them:


**/file/open(String filepath, Boolean start_blank = false)**

Used to create (or open) a file with the given filepath as its full name. 

The second argument, ``start_blank``, is by default ``false``. If set to ``true``, the file's entire contents are blanked without warning before reading/writing begins.

Returns a Boolean which is ``true`` if opening the file succeeded, and ``false`` if it failed.`


**/file/lines()**

Returns a table which is a list of every single line of text within the (supposed) text file currently open, for possible iteration.

If no file is open, the function returns ``null`` instead.

**/file/write(String text)**

Writes a given string of text into the file currently opened.

Returns ``true`` if successful and ``null`` if no file is open.

**/file/close()**

Closes the file's stream, thereby disconnecting it from João. Useful if you're interested in having another program read the file while João is still running.

Returns ``true`` if it worked, and ``null`` if no file is open or the file is already closed.

# 2.4 - Tables

The table class implements associative arrays. An associative array is an array that can be indexed not only with non-negative integers, but also with strings. Moreover, unlike the properties of objects, tables have no fixed size; you can add as many elements as you want to a table dynamically. Tables are the main ad-hoc data structuring mechanism in João, and a powerful one, sidestepping the rigidity of the typical class system. We can use tables to represent ordinary arrays, databases, and even other tables, in a simple, uniform, and efficient way.

You can construct tables simply through a typical ``/New()`` call. As well, you can use brackets to index into it, to set values and acquire them later:

```python
/main()
{
	Object tbl = /table/New();
	tbl[0] = "apple"; ## Tables are zero-indexed in João.
	print("I love " .. tbl[0] "s!"); ## Prints "I love apples!"
}
```

Like all objects, when a program has no references to a table left, João memory management is supposed to eventually delete the table and free the memory it was using for other purposes.

Because we can index a table with both non-negative integers and strings, oddities can occur when using strings that contain numeric characters. For instance, using the index "0" is different from using the index 0, and one is not necessarily equivalent to the other. When in doubt, typecast it out for clarity and confidence.
