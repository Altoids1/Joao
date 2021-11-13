![Joao_Color_Smaller](https://user-images.githubusercontent.com/29939414/114955666-c9ffe700-9e22-11eb-95fd-649fd2ef250b.png)

#### A directory- and object-oriented interpreted programming language.



## Why João?

I don't like how most languages handle object-orientation.

Often, the process by which things inherit from each other can end up being very incestuous and confusing to people joining new projects. Antipatterns like class [inheritence from multiple base classes](https://docs.microsoft.com/en-us/cpp/cpp/multiple-base-classes?view=msvc-160) can make things confusing, both for the debugger and for the programmer. Further, working with junior programmers who are unfamiliar with OOP (or your project's specific OOP implementation) is a chronic pain that wastes time and energy.

The reality is, though, that we already have a structure we use everyday that is very accessible, even to those new to OOP as a concept, and powerful. They're called **Directories.**

This language is built on three, harmonized principles:

1. Everything is a directory if you really think about it. *(Directory-orientation)*
2. Everything is a hashtable if you really think about it. *(Array-orientation)*
3. Everything is an object if you really think about it. *(Object-orientation)*

While #1 is the prime orientation by which the language's base syntax is designed, all three are held to be equal and valid interpretations of how to do programming and how to work with complicated object/array/directory structures.

Beyond that, this language is made with a love for Lua, a contempuous acquaintance with PHP, and a burning hatred for C# and other languages.

I hope you like it.

*<3 Altoids*

### Syntax Example

```dm
/main()
{
	Value a = /apple/New();
	print(a.get_color(), "is the color of my apple!");
	return 0;
}

/apple
{
	Value stinky = false;
	String color = "red";
}

/apple/rotten
{
	Value stinky = true;
	Value color = "green";
}

/apple/get_color()
{
	return ./color;
}
```
