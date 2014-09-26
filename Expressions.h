#ifndef _EXPRESSIONS_HEADER
#define _EXPRESSIONS_HEADER

#ifndef DBG_NEW      
#define DBG_NEW new ( _NORMAL_BLOCK , __FILE__ , __LINE__ )     
#define new DBG_NEW   
#endif

#define _CRTDBG_MAP_ALLOC
#include <crtdbg.h>

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
		::std::string name;
	public:
		NameExpression(::std::string name)
		{
			this->name = name;
		}

		::std::string GetName()
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
			//todo make me detect if this is a local or not
			context->Load(name);
		}

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

		void Compile(CompilerContext* context)
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
		}
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

		void Compile(CompilerContext* context)
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
		}
	};

	class LocalExpression: public Expression
	{
		Token _name;
		Expression* _right;
	public:
		LocalExpression(Token name, Expression* right)
		{
			this->_name = name;
			this->_right = right;
		}

		~LocalExpression()
		{
			delete this->_right;
		}

		std::string GetName()
		{
			return this->_name.getText();
		}

		void print()
		{
			printf(_name.getText().c_str());
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

		void Compile(CompilerContext* context)
		{
			context->Number(this->value);
		}
	};

	class StringExpression: public Expression
	{
		::std::string value;
	public:
		StringExpression(::std::string value)
		{
			this->value = value;
		}

		::std::string GetValue()
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

	class IndexExpression: public Expression, public IStorableExpression
	{
		Expression* left, *index;
	public:
		IndexExpression(Expression* left, Expression* index)
		{
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

		void Compile(CompilerContext* context)
		{
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
		}

		void CompileStore(CompilerContext* context)
		{
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
		Expression* left;

		Token t;
		Expression* right;
	public:
		OperatorAssignExpression(Token token, Expression* l, Expression* r)
		{
			this->t = token;
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
			printf("(%s ", t.getText().c_str());
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
		TokenType _operator;

		Expression* right;
	public:
		PrefixExpression(TokenType type, Expression* r)
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
			printf("(%s", Operator(_operator));
			right->print();
			printf(")");
		}

		void Compile(CompilerContext* context);
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
			printf("%s)", Operator(_operator));
		}

		void Compile(CompilerContext* context);
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

	class StatementExpression: public Expression
	{
	public:
	};

	class BlockExpression: public Expression
	{

	public:
		::std::vector<Expression*>* statements;
		BlockExpression(Token token, ::std::vector<Expression*>* statements)
		{
			this->statements = statements;
		}

		BlockExpression(::std::vector<Expression*>* statements)
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
	public:
		WhileExpression(Token token, Expression* cond, ScopeExpression* block)
		{
			this->condition = cond;
			this->block = block;
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
			::std::string uuid = context->GetUUID();
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
			::std::string uuid = context->GetUUID();
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
		::std::vector<Branch*>* branches;
		Branch* Else;
	public:
		IfExpression(Token token, ::std::vector<Branch*>* branches, Branch* elseBranch)
		{
			this->branches = branches;
			this->Else = elseBranch;
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
		Expression* left;
		::std::vector<Expression*>* args;
	public:
		friend class FunctionParselet;
		CallExpression(Token token, Expression* left, ::std::vector<Expression*>* args)
		{
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
		::std::vector<Expression*>* args;
		ScopeExpression* block;
	public:

		FunctionExpression(Token token, Expression* name, ::std::vector<Expression*>* args, ScopeExpression* block)
		{
			this->args = args;
			this->block = block;
			this->name = name;
		}

		~FunctionExpression()
		{
			delete block;
			delete name;

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
		Expression* right;
	public:
		ReturnExpression(Token token, Expression* right)
		{
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
			if (right)
				right->Compile(context);
			else
				context->Number(-555555);//bad value to prevent use

			context->Return();
		}
	};

}
#endif