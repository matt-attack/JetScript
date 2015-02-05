// AsmVM.cpp : Defines the entry point for the console application.
//
//add multiple returns perhaps?
#ifdef _DEBUG
#ifndef DBG_NEW      
//#define DBG_NEW new ( _NORMAL_BLOCK , __FILE__ , __LINE__ )     
//#define new DBG_NEW   
#endif

#define _CRTDBG_MAP_ALLOC
#include <crtdbg.h>
#endif

#include <stdio.h>

typedef wchar_t     _TCHAR;

#include <stack>
#include <vector>

#define CODE(code) #code

#include "JetContext.h"

#include <iostream>
#include <string>
#include <fstream>
#include <streambuf>
#include <math.h>
#ifdef _WIN32
#include <io.h>
#include <tchar.h>
#include <Windows.h>
#endif
#include <functional>

using namespace Jet;

int main(int argc, char* argv[])
{
#ifdef _WIN32
	_CrtSetDbgFlag ( _CRTDBG_ALLOC_MEM_DF | _CRTDBG_LEAK_CHECK_DF);
	COORD x;
	x.X = 80;
	x.Y = 1000;
	SetConsoleScreenBufferSize(GetStdHandle(STD_OUTPUT_HANDLE), x);
#endif
	printf("Jet Language Console:\n");

	//ok, make an execution context
	JetContext context;

	//bind basic math functions
	JetBind(context, tan);
	JetBind(context, sin);
	JetBind(context, cos);
	JetBind(context, log);
	JetBind2(context, pow, double);
	JetBind(context, sqrt);
	JetBind(context, exp);
	JetBind(context, atan);
	JetBind2(context, atan2, double);
	JetBind(context, acos);
	JetBind(context, asin);

	Value args[3];
	args[0] = Value(3);
	args[1] = Value(4);
	args[2] = Value(5);
	printf("Sizeof Value: %d\n\n", sizeof(Value));

	if (argc > 1)
	{
		if (argv[1])
		{
			try
			{
				std::ifstream t(argv[1], std::ios::in | std::ios::binary);
				if (t)
				{
					int length;
					t.seekg(0, std::ios::end);    // go to the end
					length = t.tellg();           // report location (this is the length)
					t.seekg(0, std::ios::beg);    // go back to the beginning
					char* buffer = new char[length];    // allocate memory for a buffer of appropriate dimension
					t.read(buffer, length);       // read the whole file into the buffer
					buffer[length] = 0;
					t.close();

					context.Script(buffer, argv[1]);
					delete[] buffer;
				}
				else
				{
					printf("Could not find file!");
				}
			}
			catch(CompilerException E)
			{
				printf("Exception found:\n");
				printf("%s (%d): %s\n", E.file.c_str(), E.line, E.ShowReason());
			}
		}
	}

	while (true)
	{
		printf("\n>");
		char command[80];
		char arg[50]; char command2[50];
		memset(arg, 0, 50);
		memset(command2, 0, 50);
		std::cin.getline(command, 80);

		sscanf(command, "%s %s\n", command2, arg);
		if (strcmp(command2, "run") == 0)
		{
			char* buffer = 0;
			try
			{
				std::ifstream t(arg, std::ios::in | std::ios::binary);
				if (t)
				{
					t.seekg(0, std::ios::end);    // go to the end
					int length = t.tellg();           // report location (this is the length)
					t.seekg(0, std::ios::beg);    // go back to the beginning
					buffer = new char[length+1];    // allocate memory for a buffer of appropriate dimension
					t.read(buffer, length);       // read the whole file into the buffer
					buffer[length] = 0;
					t.close();

					context.Script(buffer, arg);
					delete[] buffer;
				}
				else
				{
					printf("Could not find file!");
				}
			}
			catch(CompilerException E)
			{
				delete[] buffer;
				printf("Exception found:\n");
				printf("%s (%d): %s\n", E.file.c_str(), E.line, E.ShowReason());
			}
			catch(RuntimeException E)
			{
				printf("Exception found:\n");
				printf("%s\n",  E.reason.c_str());
			}
		}
		else if (strcmp(command2, "test") == 0 && arg[0] == 0)
		{
			try
			{
				JetContext context;//create new context for this
				printf("Running tests....\n");
				//make me into a test for unary operators
				context["effect_move"] = [](JetContext* context, Value* arguments, int nargs)
				{
					Jet::Value a = arguments[0];   // Number , -100 <- Lost string value
					Jet::Value b = arguments[1];   // Number , -100
					Jet::Value c = arguments[2];   // Number , 120
					Jet::Value d = arguments[3];   // Number , 6
					return Value();
				};
				context.Script("effect_move(\"hi\", -100, 120, 6);");

				try
				{
					//the 5++ in the if statement leaks stack
					Value ret = context.Script(
						"fail = 0;"
						"if (++5 != 6) fail = 1;"
						"if (5++ != 5) fail = 1;"
						"x = -5;"
						"y = -5;"
						"5++;"
						"++5;"
						"print(++x);"
						"return ++x;");
					if ((int)ret != -3)
						throw 7;
					
					if ((int)context["y"] != -5)
						throw 7;

					if ((int)context["fail"] != 1)
						throw 7;
				}
				catch (...)
				{
					throw CompilerException("", 0, "unary operator test failed\n");
				}
				
				try
				{
					context.Script("fun main(dt, f2, a3) { print(dt); return dt; } ");
					Value va = 52;
					auto out = context.Call("main", &va, 1);
					if ((int)out != 52)
						throw 7;
				}
				catch (...)
				{
					throw CompilerException("", 0, "context.Call test failed\n");
				}

				//test reading and setting vars
				context["x"] = 52;
				int vp = context["x"];
				if (vp != 52)
					throw CompilerException("", 0, "getting/setting var test failed\n");

				context.Set("x", 56);
				if ((int)context.Get("x") != 56)
					throw CompilerException("", 0, "explicit getting/setting var test failed\n");

				//recursive calling checks
				auto f = [](JetContext* context,Value* args, int numargs)
				{ 
					return context->Call("h");
				};

				context["inception"] = Value(f);
				context.Script("fun h() { return 7; } inception(); print(\"hi im still alive\"); print(inception());");
				
				//this should not crash or leave anything on the callstack
				//context.Script("fun h() { return p[7]; } inception(); print(\"hi im still alive\");");


				Value v = context.Script("if (5 >= 5) return \"ok\"; else return \"not ok\";", "Test 1");
				//if test
				v = context.Script("if (0) return 5; else if (2) return 6; else return 7;");
				if ((double)v != 6.0f)
				{
					printf("If Statement Test Failed!\n");
				}

				//native function test
				context["ih"] = Value([](JetContext* context, Value* args, int numargs) 
				{ 
					return args[0];
				});

				if ((v = context.Script("return ih(\"test\");")).ToString() != "test")
				{
					printf("Native Function Test Failed!\n");
				}


				//test meta tables and userdata
				try
				{
					//fix valueref breaking this
					ValueRef meta = context.NewPrototype("Test");//ok, lets give me a type
					meta["t1"] = [](JetContext* context, Value* v, int args)
					{
						printf("hi from metatable\n");
						return Value();
					};
					meta["t2"] = [](JetContext* context, Value* v, int args)
					{
						return Value(7);
					};
					meta["t3"] = [](JetContext* context, Value* v, int args)
					{
						return Value();
					};
					context["mttest"] = context.NewUserdata(0, meta);
					auto out = context.Script("mttest.t1();"
						"getprototype(mttest).t3 = fun() { print(\"hi from t3\n\");}; mttest.t3(); return mttest.t2();");
					if ((double)out != 7.0)
						throw 7;
					//release table when context is done
					ValueRef t2 = context.NewPrototype("hi");
					//meta.Release();
					//t2.Release();
				}
				catch(...)
				{
					throw CompilerException("", 0, "Metatable test failed!\n");
				}

				//loop test
				try
				{
					context.Script("x = [1,2,3,4,5];"
						"z = [];"
						"for (local i in x)"
						"	z:add(i);");
					if ((int)context["z"][0] != 1)
						throw 7;
					if ((int)context["z"][4] != 5)
						throw 7;

					context.Script("x = {a=1,b=2,c=3,d=4,e=5};"
						"z = [];"
						"print(x);"
						"for (local i in x) {"
						" print(i);"
						"	z:add(i); }");
					if ((int)context["z"][0] != 1)
						throw 7;
					if ((int)context["z"][4] != 5)
						throw 7;
				}
				catch(RuntimeException e)
				{
					throw CompilerException("", 0, "Loop test failed!\n");
				}
				catch(...)
				{
					throw CompilerException("", 0, "Loop test failed!\n");
				}

				//== operator test
				try
				{
					context.Script("fun h(v) { return v == null; } x = h(1==1);");
					if ((double)context["x"] != 0.0f)
					{
						throw 7;
					}
				}
				catch(...)
				{
					throw CompilerException("", 0, "== operator precedence test failed\n");
				}

				context.Script("apples = {};", "Test 2");
				context.Script("while(1) { print(\"this should print\"); break; print(\"this should not print\"); continue; } ", "Test 3");
				context.Script("test = [5,6,7,6,\"hello\"]; return 1;", "Test 4");
				context.Script("local apple = {}; gc(); global = apple; global[\"test\"] = 7; return 2;", "Test 5");
				context.Script("fun hi() { local z = 2; z++; x++; fun test(a,b,x) { return a+b+x;} test(); while (true) { h = 2; } ap = \"test\"; } return 7;", "Test 6");
				context.Script("for (i = 0; i < 10; i++) { for (j = 0; j < 2; j++) h = 7; } apple = fun(x,y) { return x+y;}; return apple(1,2);", "Test 7");
				context.Script("y = 0; if (x) y++; else y--;", "Test 8");
				context.Script("apples = {hi = 2, apple = 3, twenty = 4}; apples.two = 7; apples[\"hello\"] = \"testing\"; apples.a7 = 6; print(apples.a7); return 4;", "Test 9");

				//context.RunGC();
				//context["hi"]("hi", "apples", 1);
				printf("Tests complete!\n");
			}
			catch(CompilerException E)
			{
				printf("Tests failed: Exception found:\n");
				printf("%s (%d): %s\n", E.file.c_str(), E.line, E.ShowReason());
			}
			catch(RuntimeException E)
			{
				printf("Tests failed: Exception found:\n");
				printf("%s\n",E.reason.c_str());
			}
		}
		else if (strcmp(command2, "quit") == 0 && arg[0] == 0)
		{
			break;
		}
		else 
		{
			try
			{
				//try and execute
				Value ret = context.Script(command);
				if (ret.type != ValueType::Null)
				{
					printf("%s\n", ret.ToString().c_str());
				}
			}
			catch(CompilerException E)
			{
				printf("Exception found:\n");
				printf("%s (%d): %s\n", E.file.c_str(), E.line, E.ShowReason());
			}
			catch(RuntimeException E)
			{
				printf("Exception found:\n");
				printf("%s\n",  E.reason.c_str());
			}
		}
	}

	return 0;
}

