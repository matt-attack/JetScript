
#ifndef _JET_PARSELETS_HEADER
#define _JET_PARSELETS_HEADER

#include "Expressions.h"
#include "Token.h"

#include <stdlib.h>

namespace Jet
{
	class Parser;
	class Expression;

	//Parselets
	class PrefixParselet
	{
	public:
		virtual Expression* parse(Parser* parser, Token token) = 0;
	};

	class NameParselet: public PrefixParselet
	{
	public:
		Expression* parse(Parser* parser, Token token);
	};

	class NumberParselet: public PrefixParselet
	{
	public:
		Expression* parse(Parser* parser, Token token)
		{
			return new NumberExpression(::atof(token.getText().c_str()));
		}
	};

	class NullParselet: public PrefixParselet
	{
	public:
		Expression* parse(Parser* parser, Token token)
		{
			return new NullExpression();
		}
	};

	class LambdaParselet: public PrefixParselet
	{
	public:
		Expression* parse(Parser* parser, Token token);
	};

	class ArrayParselet: public PrefixParselet
	{
	public:
		Expression* parse(Parser* parser, Token token);
	};

	class ObjectParselet: public PrefixParselet
	{
	public:
		Expression* parse(Parser* parser, Token token);
	};

	class StringParselet: public PrefixParselet
	{
	public:
		Expression* parse(Parser* parser, Token token)
		{
			return new StringExpression(token.getText());
		}
	};

	class GroupParselet: public PrefixParselet
	{
	public:
		Expression* parse(Parser* parser, Token token);
	};

	class PrefixOperatorParselet: public PrefixParselet
	{
		int precedence;
	public:
		PrefixOperatorParselet(int precedence)
		{
			this->precedence = precedence;
		}

		Expression* parse(Parser* parser, Token token);

		int GetPrecedence()
		{
			return precedence;
		}
	};
	class InfixParselet
	{
	public:
		virtual Expression* parse(Parser* parser, Expression* left, Token token) = 0;

		virtual int getPrecedence() = 0;
	};

	class AssignParselet: public InfixParselet
	{
	public:
		Expression* parse(Parser* parser, Expression* left, Token token);

		int getPrecedence()
		{
			return 1;
		}
	};

	class OperatorAssignParselet: public InfixParselet
	{
	public:

		Expression* parse(Parser* parser, Expression* left, Token token);

		int getPrecedence()
		{
			return 1;
		}
	};

	class SwapParselet: public InfixParselet
	{
	public:
		Expression* parse(Parser* parser, Expression* left, Token token);

		int getPrecedence()
		{
			return 1;
		}
	};

	class PostfixOperatorParselet: public InfixParselet
	{
		int precedence;
	public:
		PostfixOperatorParselet(int precedence)
		{
			this->precedence = precedence;
		}

		Expression* parse(Parser* parser, Expression* left, Token token)
		{
			return new PostfixExpression(left, token);
		}

		int getPrecedence()
		{
			return precedence;
		}
	};

	class BinaryOperatorParselet: public InfixParselet
	{
		int precedence;
		bool isRight;
	public:
		BinaryOperatorParselet(int precedence, bool isRight)
		{
			this->precedence = precedence;
			this->isRight = isRight;
		}

		Expression* parse(Parser* parser, Expression* left, Token token);

		int getPrecedence()
		{
			return precedence;
		}
	};

	class IndexParselet: public InfixParselet
	{
	public:
		Expression* parse(Parser* parser, Expression* left, Token token);

		int getPrecedence()
		{
			return 9;
		}
	};

	class MemberParselet: public InfixParselet
	{
	public:
		Expression* parse(Parser* parser, Expression* left, Token token);

		int getPrecedence()
		{
			return 3;//maybe?
		}
	};

	class CallParselet: public InfixParselet
	{
	public:

		Expression* parse(Parser* parser, Expression* left, Token token);

		int getPrecedence()
		{
			return 9;//whatever postfix precedence is
		}
	};

	class StatementParselet
	{
	public:
		bool TrailingSemicolon;

		virtual Expression* parse(Parser* parser, Token token) = 0;
	};

	class ReturnParselet: public StatementParselet
	{
	public:
		ReturnParselet()
		{
			this->TrailingSemicolon = true;
		}

		Expression* parse(Parser* parser, Token token);
	};

	class ContinueParselet: public StatementParselet
	{
	public:
		ContinueParselet()
		{
			this->TrailingSemicolon = true;
		}

		Expression* parse(Parser* parser, Token token)
		{
			return new ContinueExpression();
		}
	};

	class BreakParselet: public StatementParselet
	{
	public:
		BreakParselet()
		{
			this->TrailingSemicolon = true;
		}

		Expression* parse(Parser* parser, Token token)
		{
			return new BreakExpression();
		}
	};

	class WhileParselet: public StatementParselet
	{
	public:
		WhileParselet()
		{
			this->TrailingSemicolon = false;
		}

		Expression* parse(Parser* parser, Token token);
	};

	class FunctionParselet: public StatementParselet
	{
	public:
		FunctionParselet()
		{
			this->TrailingSemicolon = false;
		}

		Expression* parse(Parser* parser, Token token);
	};

	class IfParselet: public StatementParselet
	{
	public:
		IfParselet()
		{
			this->TrailingSemicolon = false;
		}

		Expression* parse(Parser* parser, Token token);
	};

	class ForParselet: public StatementParselet
	{
	public:
		ForParselet()
		{
			this->TrailingSemicolon = false;
		}

		Expression* parse(Parser* parser, Token token);
	};

	class LocalParselet: public StatementParselet
	{
	public:
		LocalParselet()
		{
			this->TrailingSemicolon = false;
		}

		Expression* parse(Parser* parser, Token token);
	};

	//use me for parallelism
	class ConstParselet: public StatementParselet
	{
	public:
		ConstParselet()
		{
			this->TrailingSemicolon = true;
		}

		Expression* parse(Parser* parser, Token token);
	};
};

#endif