#include "Expressions.h"
#include "Compiler.h"
#include "Parser.h"

void PrefixExpression::Compile(CompilerContext* context)
{
	right->Compile(context);
	context->UnaryOperation(this->_operator);
	if (dynamic_cast<BlockExpression*>(this->Parent) == 0)
		context->Duplicate();
	context->Store(dynamic_cast<NameExpression*>(right)->GetName());
}

void PostfixExpression::Compile(CompilerContext* context)
{
	left->Compile(context);
	if (dynamic_cast<BlockExpression*>(this->Parent) == 0)
		context->Duplicate();
	context->UnaryOperation(this->_operator);
	context->Store(dynamic_cast<NameExpression*>(left)->GetName());
}

void SwapExpression::Compile(CompilerContext* context)
{
	std::string name2 = dynamic_cast<NameExpression*>(right)->GetName();
	context->Load(name2);
	context->Load(name);
	context->Store(name2);
	if (dynamic_cast<BlockExpression*>(this->Parent) == 0)
		context->Duplicate();
	context->Store(name);
}

void AssignExpression::Compile(CompilerContext* context)
{
	this->right->Compile(context);
	//insert store here
	if (dynamic_cast<BlockExpression*>(this->Parent) == 0)
		context->Duplicate();//if my parent is not block expression, we need the result, so push it

	context->Store(name);
}

void CallExpression::Compile(CompilerContext* context)
{
	//push args onto stack
	for (auto i: *args)
		i->Compile(context);

	if (dynamic_cast<NameExpression*>(left) == 0)
		throw ParserException("dummy", 555, "Error: Cannot call an expression that is not a name");

	context->Call(dynamic_cast<NameExpression*>(left)->GetName(), args->size());

	//pop off return value if we dont need it
	if (dynamic_cast<BlockExpression*>(this->Parent) != 0)
		context->Pop();//if my parent is block expression, we dont the result, so pop it
}

void OperatorAssignExpression::Compile(CompilerContext* context)
{
	context->Load(name);
	this->right->Compile(context);
	context->BinaryOperation(t.getType());

	//insert store here
	if (dynamic_cast<BlockExpression*>(this->Parent) == 0)
		context->Duplicate();//if my parent is not block expression, we need the result, so push it

	context->Store(name);
}

void FunctionExpression::Compile(CompilerContext* context)
{
	//todo make this push make a new functioncompilecontext
	CompilerContext* function = context->AddFunction(dynamic_cast<NameExpression*>(name)->GetName());
	//context->Label(dynamic_cast<NameExpression*>(name)->GetName());
	//todo make these locals
	//ok push locals, in opposite order
	for (int i = this->args->size() - 1; i >= 0; i--)//auto ii = this->args->end(); ii != this->args->begin(); ii--)
	{
		auto name = dynamic_cast<NameExpression*>((*this->args)[i]);
		function->StoreLocal(name->GetName());
	}
	block->Compile(function);

	//if last instruction was a return, dont insert another one
	if (dynamic_cast<ReturnExpression*>(block->statements->at(block->statements->size()-1)) == 0)
		function->Return();

	context->FinalizeFunction(function);
	//vm will pop off locals when it removes the call stack
}