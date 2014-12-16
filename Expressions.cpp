#include "Expressions.h"
#include "Compiler.h"
#include "Parser.h"

using namespace Jet;

void PrefixExpression::Compile(CompilerContext* context)
{
	context->Line(this->_operator.filename, this->_operator.line);

	right->Compile(context);
	context->UnaryOperation(this->_operator.type);
	if (dynamic_cast<BlockExpression*>(this->Parent) == 0)
		context->Duplicate();

	if (dynamic_cast<IStorableExpression*>(this->right))
		dynamic_cast<IStorableExpression*>(this->right)->CompileStore(context);
}

void IndexExpression::Compile(CompilerContext* context)
{
	context->Line(token.filename, token.line);

	//add load variable instruction
	left->Compile(context);
	//if the index is constant compile to a special instruction carying that constant
	if (dynamic_cast<StringExpression*>(index))
	{
		context->LoadIndex(dynamic_cast<StringExpression*>(index)->GetValue().c_str());
	}
	else
	{
		index->Compile(context);
		context->LoadIndex();
	}

	if (dynamic_cast<BlockExpression*>(this->Parent) != 0)
		context->Pop();
}

void IndexExpression::CompileStore(CompilerContext* context)
{
	context->Line(token.filename, token.line);

	left->Compile(context);
	//if the index is constant compile to a special instruction carying that constant
	if (dynamic_cast<StringExpression*>(index))
	{
		context->StoreIndex(dynamic_cast<StringExpression*>(index)->GetValue().c_str());
	}
	else
	{
		index->Compile(context);
		context->StoreIndex();
	}
}

void ObjectExpression::Compile(CompilerContext* context)
{
	int count = 0;
	if (this->inits)
	{
		//set these up
		count = this->inits->size();
		for (auto ii: *this->inits)
		{
			context->String(ii.first);
			ii.second->Compile(context);
		}
	}
	context->NewObject(count);

	if (dynamic_cast<BlockExpression*>(this->Parent) != 0)
		context->Pop();
}

void ArrayExpression::Compile(CompilerContext* context)
{
	int count = 0;
	if (this->initializers)
	{
		count = this->initializers->size();
		for (auto i: *this->initializers)
		{
			i->Compile(context);
		}
	}
	context->NewArray(count);

	if (dynamic_cast<BlockExpression*>(this->Parent) != 0)
		context->Pop();
}

void StringExpression::Compile(CompilerContext* context)
{
	context->String(this->value);

	if (dynamic_cast<BlockExpression*>(this->Parent) != 0)
		context->Pop();
}

void NullExpression::Compile(CompilerContext* context)
{
	context->Null();

	if (dynamic_cast<BlockExpression*>(this->Parent) != 0)
		context->Pop();
}

void NumberExpression::Compile(CompilerContext* context)
{
	context->Number(this->value);

	if (dynamic_cast<BlockExpression*>(this->Parent) != 0)
		context->Pop();
}

void PostfixExpression::Compile(CompilerContext* context)
{
	context->Line(this->_operator.filename, this->_operator.line);

	left->Compile(context);

	if (dynamic_cast<BlockExpression*>(this->Parent) == 0)
		context->Duplicate();

	context->UnaryOperation(this->_operator.type);

	if (dynamic_cast<IStorableExpression*>(this->left))
		dynamic_cast<IStorableExpression*>(this->left)->CompileStore(context);
}

void SwapExpression::Compile(CompilerContext* context)
{
	right->Compile(context);
	left->Compile(context);

	if (dynamic_cast<IStorableExpression*>(this->right))
		dynamic_cast<IStorableExpression*>(this->right)->CompileStore(context);

	if (dynamic_cast<BlockExpression*>(this->Parent) == 0)
		context->Duplicate();

	if (dynamic_cast<IStorableExpression*>(this->left))
		dynamic_cast<IStorableExpression*>(this->left)->CompileStore(context);
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
	context->Line(token.filename, token.line);

	//need to check if left is a local, or a captured value before looking at globals
	if (dynamic_cast<NameExpression*>(left) != 0 && context->IsLocal(dynamic_cast<NameExpression*>(left)->GetName()) == false)
	{
		//push args onto stack
		for (auto i: *args)
			i->Compile(context);

		context->Call(dynamic_cast<NameExpression*>(left)->GetName(), args->size());
	}
	else// if (dynamic_cast<IStorableExpression*>(left) != 0)
	{
		auto index = dynamic_cast<IndexExpression*>(left);
		if (index && index->token.type == TokenType::Colon)
		{
			//push args onto stack
			for (auto i: *args)
				i->Compile(context);//pushes args

			//compile left I guess?
			left->Compile(context);//pushes function

			//ok, fix the order here, this isnt working right
			//need to insert before the last instruction
			auto t = context->out.back();
			context->out.pop_back();//pushes this
			context->Duplicate();//duplicates this
			context->out.push_back(t);//pushes function
			//could just have this as last argument, idk
			//increase number of args
			context->ECall(args->size()+1);
		}
		else
		{
			//push args onto stack
			for (auto i: *args)
				i->Compile(context);

			//compile left I guess?
			left->Compile(context);

			context->ECall(args->size());
		}
	}
	//else
	//{
	//throw ParserException(token.filename, token.line, "Error: Cannot call an expression that is not a name");
	//}
	//help, how should I handle this for multiple returns
	//pop off return value if we dont need it
	if (dynamic_cast<BlockExpression*>(this->Parent) != 0)
		context->Pop();//if my parent is block expression, we dont the result, so pop it
}

void NameExpression::Compile(CompilerContext* context)
{
	//add load variable instruction
	//todo make me detect if this is a local or not
	context->Load(name);

	if (dynamic_cast<BlockExpression*>(this->Parent) != 0)
		context->Pop();
}

void OperatorAssignExpression::Compile(CompilerContext* context)
{
	context->Line(token.filename, token.line);

	left->Compile(context);
	this->right->Compile(context);
	context->BinaryOperation(token.type);

	//insert store here
	if (dynamic_cast<BlockExpression*>(this->Parent) == 0)
		context->Duplicate();//if my parent is not block expression, we need the result, so push it

	if (dynamic_cast<IStorableExpression*>(this->left))
		dynamic_cast<IStorableExpression*>(this->left)->CompileStore(context);
}

void OperatorExpression::Compile(CompilerContext* context)
{
	this->left->Compile(context);
	this->right->Compile(context);
	context->BinaryOperation(this->_operator);

	//fix these things in like if statements and for loops
	if (dynamic_cast<BlockExpression*>(this->Parent) != 0)
		context->Pop();
}

void FunctionExpression::Compile(CompilerContext* context)
{
	context->Line(token.filename, token.line);

	std::string fname;
	if (name)
		fname = dynamic_cast<NameExpression*>(name)->GetName();
	else
		fname = "_lambda_id_";//+context->GetUUID();

	CompilerContext* function = context->AddFunction(fname, this->args->size(), this->varargs);

	//ok push locals, in opposite order
	for (int i = 0; i < this->args->size(); i++)
	{
		auto aname = dynamic_cast<NameExpression*>((*this->args)[i]);
		function->RegisterLocal(aname->GetName());
	}
	if (this->varargs)
		function->RegisterLocal(this->varargs->GetName());

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
	context->Line((*_names)[0].filename, (*_names)[0].line);

	//add load variable instruction
	for (auto ii: *this->_right)
		ii->Compile(context);

	//make sure to create identifier
	for (auto _name: *this->_names)
		if (context->RegisterLocal(_name.getText()) == false)
			throw CompilerException(_name.filename, _name.line, "Duplicate Local Variable '"+_name.text+"'");

	//actually store if we have something to store
	for (int i = _right->size()-1; i >= 0; i--)
	{
		context->StoreLocal((*_names)[i].text);
	}
}

