 Jet
==========

An easy to integrate scripting language written in C++.

[Try Jet!](http://subopti.ml/)

### Example
```cpp
fun fibo(n)
{
	if (n < 2)
		return n;
	else
		return fibo(n-1)+fibo(n-2);
}
print("Fibonacci of 10: ", fibo(10));
```

### General Concepts:

Everything is a global variable unless the local keyword is placed before it.

### How to use in your program:
```cpp
#include <JetContext.h>

Jet::JetContext context;
try
{
	Jet::Value return = context.Script("return 7;");
}
catch (ParserException e)
{
	//an exception occured while compiling
}
```

### Types
- Numbers
```cpp
number = 256;
number = 3.1415926535895;
```
- Strings
```cpp
string = "hello";
```
- Objects - a map like type
```cpp
obj = {};
//two different ways to index items in the object
obj.apple = 2;
obj["apple2"] = 3;
```
- Arrays - a contiguous array of values with a set size
```cpp
arr = [];
arr:resize(2);
arr[0] = 255;
arr[1] = "Apples";
```
- Userdata - used for user defined types, stores a void*
