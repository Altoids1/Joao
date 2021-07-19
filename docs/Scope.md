# 4. Scope

João has several ways of allowing you to access the variables and members available within a given area of the code. The following is a description of the various ways that one can create, access, and modify variables within João.

## 4.1 - Blockscope

The lowest and simplest aspect of the visibilty system are local variables. These are variables declared with the local assignment syntax, like so:
```python
Object x;
String str = "Hello!";
Value x = 3;
```

Using a typecheck keyword other than ``Value`` will cause João to run a typecheck on the right-hand side before assigning it to the variable. If it is the wrong type, a runtime error is thrown. This is the only implicit typechecking within the language, and the typecheck is not performed for any assignment to that variable thereafter.

Variables declared in this way will persist until the block in which they were declared completes itself.
```python
/main()
{
	if(true)
	{
		Value x = 6; ## x exists within this if statement!
	}
	##x no longer exists!
	print(x); ## This will throw a runtime!
}
```

Every function call has its own blockscope environment, and cannot access the local variables of functions which call it without being passed as parameters.

## 4.2 - Objectscope

Functions called as methods of an Object have access to the properties of that Object:
```python
/apple
{
	taste = "yummy";
}
/apple/get_taste()
{
	return taste; ## We can access taste here!
}
/main()
{
	Object a = /apple/New();
	print("Apples taste " .. a.get_taste() .. "!"); ## "Apples taste yummy!"
}
```

To reference a property or method explicitly within a method, one can use the ``./`` scoping operator. This allows for members and local variables to share the same name, if desired:
```python
/apple
{
	taste = "yummy";
}
/apple/get_taste()
{
	String taste = "stinky >:("; ## Uh-oh, a local variable
	return ./taste; ## We can access taste this way to make sure it returns the property and not the local.
}
/main()
{
	Object a = /apple/New();
	print("Apples taste " .. a.get_taste() .. "!"); ## "Apples taste yummy!"
}
```

To access methods and properties of the *inherited* classes of an object, one can use the ``../`` operator. Note that these only access the default, "initial" values of such things.

```python
/apple
{
	taste = "yummy";
}
/apple/red
{
	taste = "not as yummy";
}
/apple/red/get_taste()
{
	return ../taste;
}
/main()
{
	Object a = /apple/New();
	print("Apples taste " .. a.get_taste() .. "!"); ## "Apples taste yummy!"
}
```

You cannot use the ``../`` operator to access methods and variables under the root directory ``/``.

## 4.3 - Globalscope

Sometimes, it is necessary to make a variable accessible in all contexts, thereby somewhat avoiding the complications of normal scoping. For this, global variables are available.

Several default global variables exist, such as ``__VERSION``, which is a string that denotes the current version of João being run:
```python
/main()
{
	print("This is Joao version " .. __VERSION .. "!"); ## This is Joao version 1.2.0!
	## obviously the version number may be different on other versions of João.
}
```

New globals may be set via the root scoping operator, ``/``. Additionally, globals be explicitly accessed through the same syntax.
```python
/foo()
{
	print(/x); ## I love globals!
}
/main()
{
	/x = "I love globals!";
	foo();
}
```

## 4.4 - Scope disambiguation

When you invoke the name of a variable, such as one named ``my_variable``, João does the following process to find the location and value of that variable, in order, iterating until it finds a definition:

1. João looks through every block in the blockscope of the current function/method, last to first, for a local variable named ``my_variable.``

2. For methods, João then looks for any member of the ``this`` object which has the identifier ``my_variable``.

3. João then looks in the globalscope in order to find a global variable named ``my_variable``.

If all else fails, João gives a runtime and returns a value containing ``null``.

If the scoping operators are used (such as ``../``, ``./``, and ``/``), then the disambiguation process is skipped, and a ``null`` and runtime are returned upon failure to access at the explicitly-requested scope.