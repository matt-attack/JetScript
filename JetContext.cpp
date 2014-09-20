#ifndef DBG_NEW      
#define DBG_NEW new ( _NORMAL_BLOCK , __FILE__ , __LINE__ )     
#define new DBG_NEW   
#endif

#define _CRTDBG_MAP_ALLOC
#include <crtdbg.h>
#include "JetContext.h"

using namespace Jet;

void Jet::gc(JetContext* context,Value* args, int numargs) 
{ 
	context->RunGC();
};

void Jet::print(JetContext* context,Value* args, int numargs) 
{ 
	for (int i = 0; i < numargs; i++)
	{
		printf("%s", args[i].ToString().c_str());
	}
	printf("\n");
};

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

#include <stack>
void JetContext::RunGC()
{
	INT64 start, rate, end;
	QueryPerformanceFrequency( (LARGE_INTEGER *)&rate );
	QueryPerformanceCounter( (LARGE_INTEGER *)&start );
	for (auto& ii: this->objects)
	{
		ii->flag = false;
	}

	for (auto& ii: this->arrays)
	{
		ii->flag = false;
	}

	//mark stack and locals here
	if (this->fptr >= 0)
	{
		//printf("GC run at runtime!\n");
		for (int i = fptr; i >= 0; i--)
		{
			for (int l = 0; l < 20; l++)
			{
				if (this->frames[i].locals[l].type == ValueType::Object)
				{
					this->frames[i].locals[l]._obj->flag = true;
					//printf("marked a local!\n");
				}
			}
			//printf("1 stack frame\n");
		}
	}

	//check locals here
	if (this->stack.size() > 0)
	{
		int loc = this->stack.size();
		for (int i = 0; i < loc; i++)
		{
			if (this->stack.mem[i].type == ValueType::Object)
			{
				std::stack<Value*> stack;
				stack.push(&this->stack.mem[i]);
				while( !stack.empty() ) {
					auto o = stack.top();
					stack.pop();
					if (o->type == ValueType::Object)
					{
						o->_obj->flag = true;

						for (auto ii: *o->_obj->ptr)
							stack.push(&ii.second);
					}
					else if (o->type == ValueType::Array)
					{
						o->_array->flag = true;

						for (auto ii: *o->_array->ptr)
							stack.push(&ii.second);
					}
				}
			}
			else if (this->stack.mem[i].type == ValueType::Array)
			{
				std::stack<Value*> stack;
				stack.push(&this->stack.mem[i]);
				while( !stack.empty() ) {
					auto o = stack.top();
					stack.pop();
					if (o->type == ValueType::Object)
					{
						o->_obj->flag = true;

						for (auto ii: *o->_obj->ptr)
							stack.push(&ii.second);
					}
					else if (o->type == ValueType::Array)
					{
						o->_array->flag = true;

						for (auto ii: *o->_array->ptr)
							stack.push(&ii.second);
					}
				}
			}
		}
		printf("there was stack left");
	}

	for (auto v: this->vars)
	{
		std::stack<Value*> stack;
		stack.push(&v);
		while( !stack.empty() ) {
			auto o = stack.top();
			stack.pop();
			if (o->type == ValueType::Object)
			{
				o->_obj->flag = true;

				for (auto ii: *o->_obj->ptr)
					stack.push(&ii.second);
			}
			else if (o->type == ValueType::Array)
			{
				o->_array->flag = true;

				for (auto ii: *o->_array->ptr)
					stack.push(&ii.second);
			}
		}
	}

	for (auto& ii: this->objects)
	{
		if (!ii->flag)
		{
			//printf("deleted object!");
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
			//printf("deleted array!");
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

	QueryPerformanceCounter( (LARGE_INTEGER *)&end );

	INT64 diff = end - start;
	double dt = ((double)diff)/((double)rate);

	printf("Took %lf seconds to collect garbage\n\n", dt);
}