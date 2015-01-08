#ifndef _EXPRESSIONS_HEADER
#define _EXPRESSIONS_HEADER

#ifdef _DEBUG
#ifndef DBG_NEW      
#define DBG_NEW new ( _NORMAL_BLOCK , __FILE__ , __LINE__ )     
#define new DBG_NEW   
#endif

#define _CRTDBG_MAP_ALLOC
#include <crtdbg.h>
#endif

#include <string>
#include <stdio.h>
#include <vector>

#include "Compiler.h"
#include "Value.h"

namespace Jet
{
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

	class IStorableExpression
	{
	public:
		virtual void CompileStore(CompilerContext* context) = 0;
	};

	class NameExpression: public Expression, public IStorableExpression
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
			printf("%s", name.c_str());
		}

		void Compile(CompilerContext* context);

		void CompileStore(CompilerContext* context)
		{
			//todo make me detect if this is a local or not
			context->Store(name);
		}
	};

	class ArrayExpression: public Expression
	{
		std::vector<Expression*>* initializers;
	public:
		ArrayExpression(std::vector<Expression*>* inits)
		{
			this->initializers = inits;
		}

		~ArrayExpression()
		{
			if (this->initializers)
				for (auto ii: *this->initializers)
					delete ii;

			delete this->initializers;
		}

		void print()
		{
			//printf(_name.getText().c_str());
		}

		void Compile(CompilerContext* context);
	};

	class ObjectExpression: public Expression
	{
		std::vector<std::pair<std::string, Expression*>>* inits;
	public:
		ObjectExpression(std::vector<std::pair<std::string, Expression*>>* initializers)
		{
			this->inits = initializers;
		}

		~ObjectExpression()
		{
			if (this->inits)
				for (auto ii: *this->inits)
					delete ii.second;
			delete this->inits;
		}

		void print()
		{
			//printf(_name.getText().c_str());
		}

		void Compile(CompilerContext* context);
	};

	class LocalExpression: public Expression
	{
		std::vector<Token>* _names;
		std::vector<Expression*>* _right;
	public:
		LocalExpression(std::vector<Token>* names, std::vector<Expression*>* right)
		{
			this->_names = names;
			this->_right = right;
		}

		~LocalExpression()
		{
			for (auto ii: *this->_right)
				delete ii;

			delete this->_right;
			delete this->_names;
		}

		std::string GetName()
		{
			return "";//this->_name.getText();
		}

		void print()
		{
			//printf(_name.getText().c_str());
		}

		void Compile(CompilerContext* context);
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

		void Compile(CompilerContext* context);
	};

	class NullExpression: public Expression
	{
	public:
		NullExpression()
		{
		}

		void print()
		{
			printf("Null");
		}

		void Compile(CompilerContext* context);
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

		void Compile(CompilerContext* context);
	};

	class IndexExpression: public Expression, public IStorableExpression
	{
		Expression* left, *index;
	public:
		Token token;
		IndexExpression(Expression* left, Expression* index, Token t)
		{
			this->token = t;
			this->left = left;
			this->index = index;
		}

		~IndexExpression()
		{
			delete left;
			delete index;
		}

		void print()
		{
			printf("[");
			index->print();
			printf("]");
		}

		void Compile(CompilerContext* context);

		void CompileStore(CompilerContext* context);
	};

	class AssignExpression: public Expression
	{
		Expression* left;
		Expression* right;
	public:
		AssignExpression(Expression* l, Expression* r)
		{
			this->left = l;
			this->right = r;
		}

		~AssignExpression()
		{
			delete this->right;
			delete this->left;
		}

		virtual void SetParent(Expression* parent)
		{
			this->Parent = parent;
			right->SetParent(this);
			left->SetParent(this);
		}

		void print()
		{
			printf("(%s = ", "expr");//name.c_str());
			right->print();
			printf(")\n");
		}

		void Compile(CompilerContext* context);
	};

	class OperatorAssignExpression: public Expression
	{
		Token token;

		Expression* left;
		Expression* right;
	public:
		OperatorAssignExpression(Token token, Expression* l, Expression* r)
		{
			this->token = token;
			this->left = l;
			this->right = r;
		}

		~OperatorAssignExpression()
		{
			delete this->right;
			delete this->left;
		}

		void SetParent(Expression* parent)
		{
			this->Parent = parent;
			right->SetParent(this);
			left->SetParent(this);
		}

		void print()
		{
			printf("(%s ", token.getText().c_str());
			left->print();
			printf(" ");
			right->print();
			printf(")\n");
		}

		void Compile(CompilerContext* context);
	};

	class SwapExpression: public Expression
	{
		Expression* left;
		Expression* right;
	public:
		SwapExpression(Expression* l, Expression* r)
		{
			this->left = l;
			this->right = r;
		}

		~SwapExpression()
		{
			delete this->right;
			delete this->left;
		}

		void SetParent(Expression* parent)
		{
			this->Parent = parent;
			right->SetParent(this);
			left->SetParent(this);
		}

		void print()
		{
			printf("(%s swap ", "idk");//name.c_str());
			right->print();
			printf(")\n");
		}

		void Compile(CompilerContext* context);
	};

	class PrefixExpression: public Expression
	{
		Token _operator;

		Expression* right;
	public:
		PrefixExpression(Token type, Expression* r)
		{
			this->_operator = type;
			this->right = r;
		}

		~PrefixExpression()
		{
			delete this->right;
		}

		void SetParent(Expression* parent)
		{
			this->Parent = parent;
			right->SetParent(this);
		}

		void print()
		{
			printf("(%s", Operator(_operator.type));
			right->print();
			printf(")");
		}

		void Compile(CompilerContext* context);
	};

	class PostfixExpression: public Expression
	{
		Token _operator;

		Expression* left;
	public:
		PostfixExpression(Expression* l, Token type)
		{
			this->_operator = type;
			this->left = l;
		}

		~PostfixExpression()
		{
			delete this->left;
		}

		void SetParent(Expression* parent)
		{
			this->Parent = parent;
			left->SetParent(this);
		}

		void print()
		{
			printf("(");
			left->print();
			printf("%s)", Operator(_operator.type));
		}

		void Compile(CompilerContext* context);
	};

	class OperatorExpression: public Expression
	{
		Token _operator;

		Expression* left, *right;

	public:
		OperatorExpression(Expression* l, Token type, Expression* r)
		{
			this->_operator = type;
			this->left = l;
			this->right = r;
		}

		~OperatorExpression()
		{
			delete this->right;
			delete this->left;
		}

		void SetParent(Expression* parent)
		{
			this->Parent = parent;
			left->SetParent(this);
			right->SetParent(this);
		}

		void print()
		{
			printf("(");
			left->print();
			printf(" %s ", Operator(_operator.type));
			right->print();
			printf(")");
		}

		void Compile(CompilerContext* context);
	};

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
			if (this->statements)
			{
				for (auto ii: *this->statements)
					delete ii;

				delete this->statements;
			}
		}

		BlockExpression() { };

		void SetParent(Expression* parent)
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
			for (auto ii: *statements)
				ii->Compile(context);
		}
	};

	class ScopeExpression: public BlockExpression
	{
		//BlockExpression* block;
	public:
		//add a list of local variables here mayhaps?

		ScopeExpression(BlockExpression* r)
		{
			this->statements = r->statements;
			r->statements = 0;
			delete r;
		}

		/*~ScopeExpression()
		{
		//delete block;
		}*/

		void Compile(CompilerContext* context)
		{
			//push scope
			context->PushScope();

			//block->Compile(context);
			BlockExpression::Compile(context);

			//pop scope
			context->PopScope();
		}
	};

	class WhileExpression: public Expression
	{
		Expression* condition;
		ScopeExpression* block;
		Token token;
	public:

		WhileExpression(Token token, Expression* cond, ScopeExpression* block)
		{
			this->condition = cond;
			this->block = block;
			this->token = token;
		}

		~WhileExpression()
		{
			delete condition;
			delete block;
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
			context->Line(token.filename, token.line);

			std::string uuid = context->GetUUID();
			context->Label("loopstart_"+uuid);
			this->condition->Compile(context);
			context->JumpFalse(("loopend_"+uuid).c_str());

			context->PushLoop("loopend_"+uuid, "loopstart_"+uuid);
			this->block->Compile(context);
			context->PopLoop();

			context->Jump(("loopstart_"+uuid).c_str());
			context->Label("loopend_"+uuid);
		}
	};

	class ForExpression: public Expression
	{
		Expression* condition, *initial, *incr;
		ScopeExpression* block;
		Token token;
	public:
		ForExpression(Token token, Expression* init, Expression* cond, Expression* incr, ScopeExpression* block)
		{
			this->condition = cond;
			this->block = block;
			this->incr = incr;
			this->initial = init;
			this->token = token;
		}

		~ForExpression()
		{
			delete this->condition;
			delete this->block;
			delete this->incr;
			delete this->initial;
		}

		void SetParent(Expression* parent)
		{
			this->Parent = parent;
			block->SetParent(this);
			incr->SetParent(block);//block);
			condition->SetParent(this);//block);
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
			context->Line(token.filename, token.line);

			std::string uuid = context->GetUUID();
			this->initial->Compile(context);
			context->Label("forloopstart_"+uuid);
			this->condition->Compile(context);
			context->JumpFalse(("forloopend_"+uuid).c_str());

			context->PushLoop("forloopend_"+uuid, "forloopcontinue_"+uuid);
			this->block->Compile(context);
			context->PopLoop();

			//this wont work if we do some kind of continue keyword unless it jumps to here
			context->Label("forloopcontinue_"+uuid);
			this->incr->Compile(context);
			context->Jump(("forloopstart_"+uuid).c_str());
			context->Label("forloopend_"+uuid);
		}
	};

	class ForEachExpression: public Expression
	{
		Token name, container;
		ScopeExpression* block;
	public:
		ForEachExpression(Token name, Token container, ScopeExpression* block)
		{
			this->container = container;
			this->block = block;
			this->name = name;
		}

		~ForEachExpression()
		{
			delete this->block;
		}

		void SetParent(Expression* parent)
		{
			this->Parent = parent;
			block->SetParent(this);
		}

		void print()
		{
			/*printf("while(");
			initial->print();
			printf("; ");
			condition->print();
			printf("; ");
			incr->print();
			printf(") {\n");
			block->print();
			printf("\n}\n");*/
		}

		void Compile(CompilerContext* context)
		{
			//return;
			context->PushScope();

			auto uuid = context->GetUUID();
			context->RegisterLocal(this->name.text);
			context->RegisterLocal("_iter");

			context->Load(this->container.text);
			context->Duplicate();
			context->LoadIndex("getIterator");
			context->ECall(1);
			context->Store("_iter");


			context->Label("_foreachstart"+uuid);
			context->Load("_iter");//this->container.text);
			context->Duplicate();
			context->LoadIndex("current");
			context->ECall(1);
			context->Store(this->name.text);

			context->PushLoop("_foreachend"+uuid, "_foreachstart"+uuid);
			this->block->Compile(context);
			context->PopLoop();

			context->Load("_iter");
			context->Duplicate();
			context->LoadIndex("advance");
			context->ECall(1);
			context->JumpFalse(("_foreachend"+uuid).c_str());

			context->Jump(("_foreachstart"+uuid).c_str());
			context->Label("_foreachend"+uuid);
			context->PopScope();
		}
	};


	struct Branch
	{
		BlockExpression* block;
		Expression* condition;
	};
	class IfExpression: public Expression
	{
		::std::vector<Branch*>* branches;
		Branch* Else;
		Token token;
	public:
		IfExpression(Token token, ::std::vector<Branch*>* branches, Branch* elseBranch)
		{
			this->branches = branches;
			this->Else = elseBranch;
			this->token = token;
		}

		~IfExpression()
		{
			if (this->Else)
			{
				delete this->Else->block;
				//delete this->Else->condition;
				delete this->Else;
			}

			for (auto ii: *this->branches)
			{
				delete ii->block;
				delete ii->condition;
				delete ii;
			}

			delete this->branches;
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
			context->Line(token.filename, token.line);

			::std::string uuid = context->GetUUID();
			::std::string bname = "ifstatement_" + uuid + "_I";
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
		Token token;
		Expression* left;
		std::vector<Expression*>* args;
	public:
		friend class FunctionParselet;
		CallExpression(Token token, Expression* left, std::vector<Expression*>* args)
		{
			this->token = token;
			this->left = left;
			this->args = args;
		}

		~CallExpression()
		{
			delete this->left;
			if (args)
			{
				for (auto ii: *args)
					delete ii;

				delete args;
			}
		}

		void SetParent(Expression* parent)
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
		Token token;
		NameExpression* varargs;
	public:

		FunctionExpression(Token token, Expression* name, std::vector<Expression*>* args, ScopeExpression* block, NameExpression* varargs = 0)
		{
			this->args = args;
			this->block = block;
			this->name = name;
			this->token = token;
			this->varargs = varargs;
		}

		~FunctionExpression()
		{
			delete block;
			delete name;
			delete varargs;

			if (args)
			{
				for (auto ii: *args)
					delete ii;
				delete args;
			}
		}

		void SetParent(Expression* parent)
		{
			this->Parent = parent;
			block->SetParent(this);
			if (name)
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
		Token token;
		Expression* right;
	public:
		ReturnExpression(Token token, Expression* right)
		{
			this->token = token;
			this->right = right;
		}

		~ReturnExpression()
		{
			delete this->right;
		}

		void SetParent(Expression* parent)
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
			context->Line(token.filename, token.line);

			if (right)
				right->Compile(context);
			else
				context->Null();//bad value to prevent use

			context->Return();
		}
	};

	class BreakExpression: public Expression
	{
	public:
		BreakExpression()
		{

		}

		~BreakExpression()
		{

		}

		void SetParent(Expression* parent)
		{
			this->Parent = parent;
		}

		void print()
		{

		}

		void Compile(CompilerContext* context)
		{
			context->Break();
		}
	};

	class ContinueExpression: public Expression
	{
	public:
		ContinueExpression()
		{

		}

		~ContinueExpression()
		{

		}

		void SetParent(Expression* parent)
		{
			this->Parent = parent;
		}

		void print()
		{

		}

		void Compile(CompilerContext* context)
		{
			context->Continue();
		}
	};
}
#endif