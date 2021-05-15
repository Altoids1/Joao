# 3. Expressions

**Expressions** are phrases that, once run, resolve to being a Value.

Many things are expressions in João: **Numbers** and **Strings**, **Variables**, **function calls**, **Constructors**, as well as **unary** and **binary** operations of all of those things.

## 3.1 - Binary & Unary Operations

### Arithmetic

João is capable of any and all mathematics that one would expect to be able to do on a scientific calculator. As such, many common arithmetic operations are available, '+' (addition), '\*' multiplication, '/' (division), and others, which are listed in full later in this chapter.

João distinguishes between two types of operations: **binary** (meaning that it is something with two, such as ``a + b``) and **unary** (meaning that it is something done on only one input, such as ``-a``).

### Comparison

João provides operations for comparing different values:

> \<   \>   \<=  \>=  ==  !=

All these operators always result in ``true`` or ``false``. For example, ``1 > 2`` evaluates to ``false``, as 1 is not greater than 2. Conversely, ``2 > 1`` returns ``true``.

João compares Objects and other higher-order types by reference; meaning, two values are equal only if they are both pointers to the exact same object in memory. An object and its clone are not equal under this system.

For example, consider the following code:

```python
	Object a = /table/New("apple");
	Object b = /table/New("apple");
	Object c = a;
```

It would be true that ``a == c`` but not true that ``a == b``.

### Logic

An important part of a program is being able to do logic, so important that they have their own operators in João:

> && \|\| ! ~~

Referred to in speech as "AND", "OR", "NOT", and "XOR", respectively. They, in general, return ``true`` or ``false`` depending on the truth of their operands. **Note:** When doing logic, everything in the language is considered to be ``true`` except ``false`` and ``null``, which are considered to be ``false``.

> **AND** is used to check that two statements are both true. ``A && B`` returns ``true`` if both A and B are ``true``, and ``false`` otherwise.

> **OR** is used to check that any statements are true. ``A || B`` returns ``true`` if A or B (or both!) are ``true``, and ``false`` otherwise.

> **NOT** is a unary operator to invert the truth of its operand. ``!A`` returns ``false`` if A is ``true`` and ``true`` if A is ``false``.

> **XOR** is used to check that only one statement is true. ``A ~~ B`` returns ``true`` if A or B (*but not both!*) are ``true``, and ``false`` otherwise.


These logical operations can be used to check that multiple conditions are true. For instance, ``(a > b) && (c > a)`` returns ``true`` only if a is both greater than b *AND* less than c, thereby forcing it to be within the range (c,b), exclusive.

### Bitwise

All data on a computer, including data held by João, is held on the computer as a particular collection of bits in memory, ones and zeroes in bundles of eight.

There are situations (such as in cryptography or bitflagging) where it becomes useful to operate on those bits directly in some way that is not a true mathematical operation. In this case, each bit (instead of storing data) becomes perceived as being a switch which can be flipped on or off according to particular rules which depend on the operation used.

The bitwise operators that are available in João to carry out such switch-flipping are the following:
> & \| ~ \>\> \<\< ~

The first three operate over two operands on a bit-by-bit basis, insofar that in ``C = A & B;`` the 1st bit of C is set to the result of ANDing the 1st bit of A with the 1st bit of B, and so on, forming an Integer of 64 bits.

The rest operate on the entire number all at once.

## 3.2 - Order of Operations

João understands the order of operations of typical mathematics. Beyond that, it also has orderings for all extra operators that exist in the language. They are evaluated, first to last, in the following order:

Operators | Associativity
--------- | -------------
()		  | None, Left
^         | Right
\- ! ~    | Right
\* \/ \/\/ % | Left
\+ \-     | Left
..        | Left
& ~ \| \>\> \<\< | Left
\< \<= \> \>= != | Left
&& \|\| ~~ | Left

**Left** Associativity on this chart indicates that, when the operators of a given precedence are repeated (such as ``a + b + c`` or ``a \* b \/ c`` that the expression is evaluated from left to right. That is to say, that a + b + c is evaluated as (a + b) + c.

**Right** Associativity is instead right-to-left. For instance, ``a ^ b ^ c`` is instead evaluated as a ^ (b ^ c). This is not meant to be confusing, but to correctly implement the user's expectation for what given operators (such as exponents) ought to do.




