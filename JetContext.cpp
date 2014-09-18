#ifndef DBG_NEW      
#define DBG_NEW new ( _NORMAL_BLOCK , __FILE__ , __LINE__ )     
#define new DBG_NEW   
#endif

#define _CRTDBG_MAP_ALLOC
#include <crtdbg.h>
#include "JetContext.h"

using namespace Jet;

const char* JetContext::Compile(const char* code)
{
	INT64 start, rate, end;
	QueryPerformanceFrequency( (LARGE_INTEGER *)&rate );
	QueryPerformanceCounter( (LARGE_INTEGER *)&start );

	Lexer lexer = Lexer(code);
	Parser parser = Parser(&lexer);

	//printf("In: %s\n\nResult:\n", code);
	BlockExpression* result = parser.parseAll();
	//result->print();
	//printf("\n\n");

	std::string out = compiler.Compile(result);

	delete result;

	QueryPerformanceCounter( (LARGE_INTEGER *)&end );

	INT64 diff = end - start;
	double dt = ((double)diff)/((double)rate);

	printf("Took %lf seconds to compile\n\n", dt);

	char* nout = new char[out.size()+1];
	out.copy(nout, out.size(), 0);
	nout[out.size()] = 0;
	return nout;
}

void JetContext::RunGC()
{
	/*for (auto& ii: this->objects)
	{
	if (ii->ptr == 0)
	continue;

	ii->flag = false;
	for (auto v: this->vars)
	{
	if (v.type == ValueType::Object)
	{
	if (v._obj == ii)
	{
	ii->flag = true;
	break;
	}
	else
	{
	if (v.Contains(ii))
	{
	ii->flag = true;
	break;
	}
	}
	}
	}
	if (!ii->flag)
	{
	printf("deleted object");
	delete ii->ptr;
	ii->ptr = 0;
	delete ii;

	ii = 0;
	}
	}*/

	for (auto& ii: this->objects)
	{
		ii->flag = false;
	}

	for (auto& ii: this->arrays)
	{
		ii->flag = false;
	}

	for (auto v: this->vars)
	{
		if (v.type == ValueType::Object)
		{
			v.Mark();
		}
		else if (v.type == ValueType::Array)
		{
			v.Mark();
		}
	}

	for (auto& ii: this->objects)
	{
		if (!ii->flag)
		{
			printf("deleted object!");
			delete ii->ptr;
			ii->ptr = 0;
			delete ii;
			ii = 0;
		}
	}

	for (auto& ii: this->arrays)
	{
		if (!ii->flag)
		{
			printf("deleted array!");
			delete ii->ptr;
			ii->ptr = 0;
			delete ii;
			ii = 0;
		}
	}

	//compact object list here
	auto list = std::move(this->objects);
	this->objects.clear();
	for (auto ii: list)
	{
		if (ii)
		{
			this->objects.push_back(ii);
		}
	}

	/*for (auto& ii: this->arrays)
	{
		ii->flag = false;
		for (auto v: this->vars)
		{
			if (v.type == ValueType::Array)
			{
				if (v._array == ii)
				{
					ii->flag = true;
					break;
				}
				else
				{
					if (v.Contains(ii))
					{
						ii->flag = true;
						break;
					}
				}
			}
		}
		if (!ii->flag)
		{
			printf("deleted array");
			delete ii->ptr;
			ii->ptr = 0;
			delete ii;
			ii = 0;
		}
	}*/

	//compact array list here
	auto arrlist = std::move(this->arrays);
	this->arrays.clear();
	for (auto ii: arrlist)
	{
		if (ii)
		{
			this->arrays.push_back(ii);
		}
	}
}