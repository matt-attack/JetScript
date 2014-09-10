#ifndef _EXPRESSIONS_HEADER
#define _EXPRESSIONS_HEADER

#include <string>
#include <stdio.h>

#include "Compiler.h"

class Compiler;

class Expression
{
public:

	Expression()
	{
		Parent = 0;
	}

	virtual ~Expression()
	{

	};

	virtual void print() = 0;

	Expression* Parent;
	virtual void SetParent(Expression* parent)
	{
		this->Parent = parent;
	}

	virtual void Compile(CompilerContext* context) 
	{

	};
};

class NameExpression: public Expression
{
	std::string name;
public:
	NameExpression(std::string name)
	{
		this->name = name;
	}

	std::string GetName()
	{
		return this->name;
	}

	void print()
	{
		printf(name.c_str());
	}

	void Compile(CompilerContext* context)
	{
		//add load variable instruction
		context->Load(name);
	}
};

class NumberExpression: public Expression
{
	double value;
public:
	NumberExpression(double value)
	{
		this->value = value;
	}

	double GetValue()
	{
		return this->value;
	}

	void print()
	{
		printf("%f", this->value);
	}

	void Compile(CompilerContext* context)
	{
		context->Number(this->value);
	}
};

class StringExpression: public Expression
{
	std::string value;
public:
	StringExpression(std::string value)
	{
		this->value = value;
	}

	std::string GetValue()
	{
		return this->value;
	}

	void print()
	{
		printf("'%s'", this->value.c_str());
	}

	void Compile(CompilerContext* context)
	{
		context->String(this->value);
	}
};

class AssignExpression: public Expression
{
	std::string name;

	Expression* right;
public:
	AssignExpression(std::string name, Expression* r)
	{
		this->name = name;
		this->right = r;
	}

	virtual void SetParent(Expression* parent)
	{
		this->Parent = parent;
		right->SetParent(this);
	}

	~AssignExpression()
	{
		delete this->right;
	}

	void print()
	{
		printf("(%s = ", name.c_str());
		right->print();
		printf(")\n");
	}

	void Compile(CompilerContext* context);
	/*{
	this->right->Compile(context);
	//insert store here
	if (dynamic_cast<ScopeExpression*>(this->Parent) == 0)
	context->Duplicate();
	//if my parent is not block expression, we need the result, so push it
	//bool 
	context->Store(name);
	//context->BinaryOperation(this->_operator);
	}*/
};

class OperatorAssignExpression: public Expression
{
	std::string name;

	Token t;
	Expression* right;
public:
	OperatorAssignExpression(Token token, std::string name, Expression* r)
	{
		this->t = token;
		this->name = name;
		this->right = r;
	}

	virtual void SetParent(Expression* parent)
	{
		this->Parent = parent;
		right->SetParent(this);
	}

	~OperatorAssignExpression()
	{
		delete this->right;
	}

	void print()
	{
		printf("(%s %s ", t.getText().c_str(), name.c_str());
		right->print();
		printf(")\n");
	}

	void Compile(CompilerContext* context);
	/*{
	this->right->Compile(context);
	//insert store here
	if (dynamic_cast<ScopeExpression*>(this->Parent) == 0)
	context->Duplicate();
	//if my parent is not block expression, we need the result, so push it
	//bool 
	context->Store(name);
	//context->BinaryOperation(this->_operator);
	}*/
};

class SwapExpression: public Expression
{
	std::string name;

	Expression* right;
public:
	SwapExpression(std::string name, Expression* r)
	{
		this->name = name;
		this->right = r;
	}

	~SwapExpression()
	{
		delete this->right;
	}

	virtual void SetParent(Expression* parent)
	{
		this->Parent = parent;
		right->SetParent(this);
	}
	void print()
	{
		printf("(%s swap ", name.c_str());
		right->print();
		printf(")\n");
	}

	void Compile(CompilerContext* context);
	/*{
	std::string name2 = dynamic_cast<NameExpression*>(right)->GetName();
	context->Load(name2);
	context->Load(name);
	context->Store(name2);
	if (dynamic_cast<BlockExpression*>(this->Parent) == 0)
	context->Duplicate();
	context->Store(name);
	//this->right->Compile(context);
	//if (dynamic_cast<BlockExpression*>(this->Parent) == 0)
	//context->Duplicate();
	}*/
};

class PrefixExpression: public Expression
{
	TokenType _operator;

	Expression* right;
public:
	PrefixExpression(TokenType type, Expression* r)
	{
		this->_operator = type;
		this->right = r;
	}

	virtual void SetParent(Expression* parent)
	{
		this->Parent = parent;
		right->SetParent(this);
	}

	void print()
	{
		printf("(%s", Operator(_operator));
		right->print();
		printf(")");
	}

	void Compile(CompilerContext* context);
	/*{
	right->Compile(context);
	context->UnaryOperation(this->_operator);
	if (dynamic_cast<BlockExpression*>(this->Parent) == 0)
	context->Duplicate();
	context->Store(dynamic_cast<NameExpression*>(right)->GetName());
	//context->BinaryOperation(this->_operator);
	}*/
};

class PostfixExpression: public Expression
{
	TokenType _operator;

	Expression* left;
public:
	PostfixExpression(Expression* l, TokenType type)
	{
		this->_operator = type;
		this->left = l;
	}

	virtual void SetParent(Expression* parent)
	{
		this->Parent = parent;
		left->SetParent(this);
	}

	void print()
	{
		printf("(");
		left->print();
		printf("%s)", Operator(_operator));
	}

	void Compile(CompilerContext* context);
	/*{
	left->Compile(context);
	if (dynamic_cast<BlockExpression*>(this->Parent) == 0)
	context->Duplicate();
	context->UnaryOperation(this->_operator);
	context->Store(dynamic_cast<NameExpression*>(left)->GetName());
	//context->BinaryOperation(this->_operator);
	}*/
};

class OperatorExpression: public Expression
{
	TokenType _operator;

	Expression* left, *right;

public:
	OperatorExpression(Expression* l, TokenType type, Expression* r)
	{
		this->_operator = type;
		this->left = l;
		this->right = r;
	}

	virtual void SetParent(Expression* parent)
	{
		this->Parent = parent;
		left->SetParent(this);
		right->SetParent(this);
	}

	~OperatorExpression()
	{
		delete this->right;
		delete this->left;
	}

	void print()
	{
		printf("(");
		left->print();
		printf(" %s ", Operator(_operator));
		right->print();
		printf(")");
	}

	void Compile(CompilerContext* context)
	{
		this->left->Compile(context);
		this->right->Compile(context);
		context->BinaryOperation(this->_operator);
	}
};

#include <vector>

class StatementExpression: public Expression
{
public:
};

class BlockExpression: public Expression
{

public:
	std::vector<Expression*>* statements;
	BlockExpression(Token token, std::vector<Expression*>* statements)
	{
		this->statements = statements;
	}

	BlockExpression(std::vector<Expression*>* statements)
	{
		this->statements = statements;
	}

	~BlockExpression()
	{
		for (auto ii: *this->statements)
			delete ii;

		delete this->statements;
	}

	BlockExpression() { };

	virtual void SetParent(Expression* parent)
	{
		this->Parent = parent;
		for (auto ii: *statements)
			ii->SetParent(this);
	}

	void print()
	{
		for (auto ii: *statements)
			ii->print();
	}

	void Compile(CompilerContext* context)
	{
		//push scope
		for (auto ii: *statements)
			ii->Compile(context);

		//pop stack
	}
};

class ScopeExpression: public BlockExpression
{
public:
	ScopeExpression(BlockExpression* r)
	{
		this->statements = r->statements;
	}

	void Compile(CompilerContext* context)
	{
		//push scope
		BlockExpression::Compile(context);
		//pop scope
	}
};

class WhileExpression: public Expression
{
	Expression* condition;
	ScopeExpression* block;
public:
	WhileExpression(Token token, Expression* cond, ScopeExpression* block)
	{
		this->condition = cond;
		this->block = block;
	}

	virtual void SetParent(Expression* parent)
	{
		this->Parent = parent;
		block->SetParent(this);
		condition->SetParent(this);
	}

	void print()
	{
		printf("while(");
		condition->print();
		printf(") {\n");
		block->print();
		printf("\n}\n");
	}

	void Compile(CompilerContext* context)
	{
		std::string uuid = context->GetUUID();
		context->Label("loopstart_"+uuid);
		this->condition->Compile(context);
		context->JumpFalse(("loopend_"+uuid).c_str());
		this->block->Compile(context);
		context->Jump(("loopstart_"+uuid).c_str());
		context->Label("loopend_"+uuid);
	}
};

class ForExpression: public Expression
{
	Expression* condition, *initial, *incr;
	ScopeExpression* block;
public:
	ForExpression(Token token, Expression* init, Expression* cond, Expression* incr, ScopeExpression* block)
	{
		this->condition = cond;
		this->block = block;
		this->incr = incr;
		this->initial = init;
	}

	virtual void SetParent(Expression* parent)
	{
		this->Parent = parent;
		block->SetParent(this);
		incr->SetParent(block);
		condition->SetParent(block);
		initial->SetParent(block);
	}

	void print()
	{
		printf("while(");
		initial->print();
		printf("; ");
		condition->print();
		printf("; ");
		incr->print();
		printf(") {\n");
		block->print();
		printf("\n}\n");
	}

	void Compile(CompilerContext* context)
	{
		//context->output += "\n";
		std::string uuid = context->GetUUID();
		this->initial->Compile(context);
		context->Label("forloopstart_"+uuid);
		this->condition->Compile(context);
		context->JumpFalse(("forloopend_"+uuid).c_str());
		this->block->Compile(context);
		//this wont work if we do some kind of continue keyword unless it jumps to here
		this->incr->Compile(context);
		context->Jump(("forloopstart_"+uuid).c_str());
		context->Label("forloopend_"+uuid);
		//context->output += "\n";
	}
};


struct Branch
{
	BlockExpression* block;
	Expression* condition;
};
class IfExpression: public Expression
{
	std::vector<Branch*>* branches;
	Branch* Else;
public:
	IfExpression(Token token, std::vector<Branch*>* branches, Branch* elseBranch)
	{
		this->branches = branches;
		this->Else = elseBranch;
	}

	virtual void SetParent(Expression* parent)
	{
		this->Parent = parent;
		if (this->Else)
			this->Else->block->SetParent(this);
		for (auto ii: *branches)
		{
			ii->block->SetParent(this);
			ii->condition->SetParent(this);
		}
	}

	void print()
	{
		int i = 0;
		for (auto ii: *branches)
		{
			if (i == 0)
				printf("if (");
			else
				printf("else if (");
			ii->condition->print();
			printf(") {\n");
			ii->block->print();
			printf("\n}\n");
			i++;
		}
		if (Else)
		{
			printf("else {\n");
			Else->block->print();
			printf("\n}\n");
		}
	}

	void Compile(CompilerContext* context)
	{
		std::string uuid = context->GetUUID();
		std::string bname = "ifstatement_" + uuid + "_I";
		int pos = 0;
		bool hasElse = this->Else ? this->Else->block->statements->size() > 0 : false;
		for (auto ii: *this->branches)
		{
			if (pos != 0)//no jump label needed on first one
				context->Label(bname);

			ii->condition->Compile(context);

			//if no else and is last go to end
			if (hasElse == false && pos == (this->branches->size()-1))
				context->JumpFalse(("ifstatementend_"+uuid).c_str());
			else
				context->JumpFalse((bname+"I").c_str());

			ii->block->Compile(context);

			if (pos != (this->branches->size()-1) || hasElse)//if isnt last one
				context->Jump(("ifstatementend_"+uuid).c_str());

			bname += "I";
			pos++;
		}

		if (hasElse)//this->Else && this->Else->block->statements->size() > 0)
		{
			context->Label(bname);
			this->Else->block->Compile(context);
		}
		context->Label("ifstatementend_"+uuid);
	}
};

class CallExpression: public Expression
{
	Expression* left;
	std::vector<Expression*>* args;
public:
	friend class FunctionParselet;
	CallExpression(Token token, Expression* left, std::vector<Expression*>* args)
	{
		this->left = left;
		this->args = args;
	}

	virtual void SetParent(Expression* parent)
	{
		this->Parent = parent;
		left->SetParent(this);
		for (auto ii: *args)
			ii->SetParent(this);
	}

	void print()
	{
		left->print();
		printf("(");
		for (auto i: *args)
		{
			i->print();
			printf(", ");
		}
		printf(")");
	}

	void Compile(CompilerContext* context);
};

class FunctionExpression: public Expression
{
	Expression* name;
	std::vector<Expression*>* args;
	ScopeExpression* block;
public:

	FunctionExpression(Token token, Expression* name, std::vector<Expression*>* args, ScopeExpression* block)
	{
		this->args = args;
		this->block = block;
		this->name = name;
	}

	virtual void SetParent(Expression* parent)
	{
		this->Parent = parent;
		block->SetParent(this);
		name->SetParent(this);
		for (auto ii: *args)
			ii->SetParent(this);
	}

	void print()
	{
		printf("Function ");
		name->print();
		printf("(");
		for (auto i: *args)
		{
			i->print();
			printf(", ");
		}
		printf(") {\n");
		block->print();
		printf("}\n");
	}

	void Compile(CompilerContext* context);
};

class ReturnExpression: public Expression
{
	Expression* right;
public:
	ReturnExpression(Token token, Expression* right)//, Expression* cond, ScopeExpression* block)
	{
		this->right = right;
	}

	virtual void SetParent(Expression* parent)
	{
		this->Parent = parent;
		if (right)
			this->right->SetParent(this);
	}

	void print()
	{
		if (right == 0)
		{
			printf("return;\n");
		}
		else
		{
			printf("return ");
			right->print();
			printf(";\n");
		}
	}

	void Compile(CompilerContext* context)
	{
		if (right)
		{
			right->Compile(context);
		}
		else
		{
			context->Number(-555555);//bad value to prevent use
		}
		//todo, remove this so it doenst clog up stack
		context->Return();
	}
};

#endif