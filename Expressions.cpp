#include "Expressions.h"
#include "Compiler.h"
#include "Parser.h"

using namespace Jet;

void PrefixExpression::Compile(CompilerContext* context)
{
	right->Compile(context);
	context->UnaryOperation(this->_operator);
	if (dynamic_cast<BlockExpression*>(this->Parent) == 0)
		context->Duplicate();
	if (dynamic_cast<IStorableExpression*>(this->right))
		dynamic_cast<IStorableExpression*>(this->right)->CompileStore(context);
}

void PostfixExpression::Compile(CompilerContext* context)
{
	left->Compile(context);
	if (dynamic_cast<BlockExpression*>(this->Parent) == 0)
		context->Duplicate();
	context->UnaryOperation(this->_operator);
	if (dynamic_cast<IStorableExpression*>(this->left))
		dynamic_cast<IStorableExpression*>(this->left)->CompileStore(context);
}

void SwapExpression::Compile(CompilerContext* context)
{
	//std::string name2 = dynamic_cast<NameExpression*>(right)->GetName();
	right->Compile(context);
	//context->Load(name2);
	left->Compile(context);
	//context->Load(name);
	//context->Store(name2);
	if (dynamic_cast<IStorableExpression*>(this->right))
		dynamic_cast<IStorableExpression*>(this->right)->CompileStore(context);

	if (dynamic_cast<BlockExpression*>(this->Parent) == 0)
		context->Duplicate();

	if (dynamic_cast<IStorableExpression*>(this->left))
		dynamic_cast<IStorableExpression*>(this->left)->CompileStore(context);

	//context->Store(name);
}

void AssignExpression::Compile(CompilerContext* context)
{
	this->right->Compile(context);
	//insert store here
	if (dynamic_cast<BlockExpression*>(this->Parent) == 0)
		context->Duplicate();//if my parent is not block expression, we need the result, so push it

	if (dynamic_cast<IStorableExpression*>(this->left))
		dynamic_cast<IStorableExpression*>(this->left)->CompileStore(context);
}

void CallExpression::Compile(CompilerContext* context)
{
	//push args onto stack
	for (auto i: *args)
		i->Compile(context);

	if (dynamic_cast<NameExpression*>(left) != 0)
	{
		context->Call(dynamic_cast<NameExpression*>(left)->GetName(), args->size());
	}
	else if (dynamic_cast<IStorableExpression*>(left) != 0)
	{
		//compile left I guess?
		left->Compile(context);
		context->ECall(args->size());
	}
	else
	{
		throw ParserException("dummy", 555, "Error: Cannot call an expression that is not a name");
	}

	//pop off return value if we dont need it
	if (dynamic_cast<BlockExpression*>(this->Parent) != 0)
		context->Pop();//if my parent is block expression, we dont the result, so pop it
}

void OperatorAssignExpression::Compile(CompilerContext* context)
{
	left->Compile(context);//context->Load(name);
	this->right->Compile(context);
	context->BinaryOperation(t.getType());

	//insert store here
	if (dynamic_cast<BlockExpression*>(this->Parent) == 0)
		context->Duplicate();//if my parent is not block expression, we need the result, so push it

	if (dynamic_cast<IStorableExpression*>(this->left))
		dynamic_cast<IStorableExpression*>(this->left)->CompileStore(context);
	//context->Store(name);
}

void FunctionExpression::Compile(CompilerContext* context)
{
	//todo make this push make a new functioncompilecontext
	std::string fname;
	if (name)
		fname = dynamic_cast<NameExpression*>(name)->GetName();
	else
		fname = "_lambda_id_"+context->GetUUID();//todo generate id

	CompilerContext* function = context->AddFunction(fname);
	
	//ok push locals, in opposite order
	for (int i = this->args->size() - 1; i >= 0; i--)
	{
		auto aname = dynamic_cast<NameExpression*>((*this->args)[i]);
		function->RegisterLocal(aname->GetName());
		function->StoreLocal(aname->GetName());
	}
	block->Compile(function);

	//if last instruction was a return, dont insert another one
	if (block->statements->size() > 0)
	{
		if (dynamic_cast<ReturnExpression*>(block->statements->at(block->statements->size()-1)) == 0)
		{
			function->Null();//return nil
			function->Return();
		}
	}
	else
	{
		function->Null();//return nil
		function->Return();
	}

	context->FinalizeFunction(function);

	//only named functions need to be stored here
	if (name)
		context->Store(dynamic_cast<NameExpression*>(name)->GetName());

	//vm will pop off locals when it removes the call stack
}

void LocalExpression::Compile(CompilerContext* context)
{
	//add load variable instruction
	//todo make me detect if this is a local or not
	if (this->_right)
		this->_right->Compile(context);

	//make sure to create identifier
	if (context->RegisterLocal(_name.getText()) == false)
		throw ParserException(_name.filename, _name.line, "Duplicate Local Variable '"+_name.getText()+"'");

	//actually store if we have something to store
	if (this->_right)
		context->StoreLocal(_name.getText());
}

