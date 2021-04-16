![Joao_White_Small](https://user-images.githubusercontent.com/29939414/114955235-d0419380-9e21-11eb-8947-ab3eb557f886.png)

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
	/apple a;
	return a.get_color();
}

/apple
{
	stinky = false;
	color = "red";
}

/apple/New()
{
	print("An apple has been born! Rejoice!");
}

/apple/rotten
{
	stinky = true;
	color = "green";
}

/apple/get_color()
{
	return this.color;
}
```
