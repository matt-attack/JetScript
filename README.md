Jet

An easy to integrate scripting language written in C++.

General Concepts:

Everything is a global variable unless the local keyword is placed before it.

How to use in your program:
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

Types

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
- Arrays - a map like type that only works with numbers
```cpp
arr = [];
arr[0] = 255;
arr[1] = "Apples";
```
- Userdata - used for user defined types, stores a void*