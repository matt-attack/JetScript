
#ifndef _PARSER
#define _PARSER

#include <stdio.h>
#include <map>
//#include <list>

#include "Token.h"
#include "Compiler.h"
#include "Expressions.h"

#include <list>

namespace Jet
{
	//exceptions
	class ParserException
	{
	public:
		unsigned int line;
		::std::string file;
		::std::string reason;

		ParserException(std::string file, unsigned int line, std::string r)
		{
			this->line = line;
			this->file = file;
			reason = r;
		};
		~ParserException() {};

		const char *ShowReason() const { return reason.c_str(); }
	};

	bool IsLetter(char c);
	bool IsNumber(char c);

	extern ::std::map<Jet::TokenType,::std::string> TokenToString; 
	class Lexer
	{
		unsigned int index;
		::std::string text;
		::std::map<::std::string, Jet::TokenType> operators;
		::std::map<::std::string, Jet::TokenType> keywords;

		unsigned int linenumber;
	public:

		Lexer(::std::string text)
		{
			this->linenumber = 0;
			this->index = 0;
			this->text = text;

			//math and assignment
			operators["="] = TokenType::Assign;
			operators["+"] = TokenType::Plus;
			operators["-"] = TokenType::Minus;
			operators["*"] = TokenType::Asterisk;
			operators["/"] = TokenType::Slash;
			operators["%"] = TokenType::Modulo;
			operators["|"] = TokenType::Or;//or
			operators["&"] = TokenType::And;//xor

			//grouping
			operators["("] = TokenType::LeftParen;
			operators[")"] = TokenType::RightParen;
			operators["{"] = TokenType::LeftBrace;
			operators["}"] = TokenType::RightBrace;

			//array stuff
			operators["["] = TokenType::LeftBracket;
			operators["]"] = TokenType::RightBracket;

			//object stuff
			operators["."] = TokenType::Dot;

			operators[";"] = TokenType::Semicolon;
			operators[","] = TokenType::Comma;

			operators["++"] = TokenType::Increment;
			operators["--"] = TokenType::Decrement;

			//operator + equals sign
			operators["+="] = TokenType::AddAssign;
			operators["-="] = TokenType::SubtractAssign;
			operators["*="] = TokenType::MultiplyAssign;
			operators["/="] = TokenType::DivideAssign;

			//boolean logic
			operators["!="] = TokenType::NotEqual;
			operators["=="] = TokenType::Equals;

			//comparisons
			operators["<"] = TokenType::LessThan;
			operators[">"] = TokenType::GreaterThan;

			//special stuff
			operators["<>"] = TokenType::Swap;
			operators["\""] = TokenType::String;

			//comments
			operators["//"] = TokenType::LineComment;
			operators["-[["] = TokenType::CommentBegin;
			operators["]]-"] = TokenType::CommentEnd;

			//keywords
			keywords["while"] = TokenType::While;
			keywords["if"] = TokenType::If;
			keywords["elseif"] = TokenType::ElseIf;
			keywords["else"] = TokenType::Else;
			keywords["fun"] = TokenType::Function;
			keywords["return"] = TokenType::Ret;
			keywords["for"] = TokenType::For;
			keywords["local"] = TokenType::Local;

			for (auto ii = operators.begin(); ii != operators.end(); ii++)
			{
				TokenToString[ii->second] = ii->first;
			}
		}

		Token Next();
	private:
		char ConsumeChar()
		{
			if (text.at(index) == '\n')
				this->linenumber++;
			return text.at(index++);
		}

		char MatchAndConsumeChar(char c)
		{
			char ch = text.at(index);

			if (c == ch)
			{
				if (ch == '\n')
					this->linenumber++;
				index++;
			}
			return ch;
		}

		char PeekChar()
		{
			return text.at(index);
		}
	};


	class Parser;

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
		/*{
		Expression* right = parser->parseExpression(precedence);
		return new PrefixExpression(token.getType(), right);//::atof(token.getText().c_str()));
		}*/

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
			return new PostfixExpression(left, token.getType());//::atof(token.getText().c_str()));
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
			return 2;//maybe?
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

	/*public class Precedence {
	// Ordered in increasing precedence.
	public static final int ASSIGNMENT = 1;
	public static final int CONDITIONAL = 2;
	public static final int SUM = 3;
	public static final int PRODUCT = 4;
	public static final int EXPONENT = 5;
	public static final int PREFIX = 6;
	public static final int POSTFIX = 7;
	public static final int CALL = 8;
	}*/
	class Parser
	{
		Lexer* lexer;
		::std::map<TokenType, InfixParselet*> mInfixParselets;
		::std::map<TokenType, PrefixParselet*> mPrefixParselets;
		::std::map<TokenType, StatementParselet*> mStatementParselets;
		::std::list<Jet::Token> mRead;
	public:
		Parser(Lexer* l);

		Expression* parseExpression(int precedence = 0)
		{
			Token token = Consume();
			PrefixParselet* prefix = mPrefixParselets[token.getType()];

			if (prefix == 0)
			{
				std::string str = "ParseExpression: No Parser Found for: " + token.getText();
				throw ParserException(token.filename, token.line, str);//printf("Consume: TokenType not as expected!\n");
			}

			Expression* left = prefix->parse(this, token);
			while (precedence < getPrecedence())
			{
				token = Consume();

				InfixParselet* infix = mInfixParselets[token.getType()];
				left = infix->parse(this, left, token);
			}
			return left;
		}

		Expression* ParseStatement(bool takeTrailingSemicolon = true);//call this until out of tokens (hit EOF)

		BlockExpression* parseBlock(bool allowsingle = true);

		BlockExpression* parseAll();

		int getPrecedence() {
			InfixParselet* parser = mInfixParselets[LookAhead(0).getType()];
			if (parser != 0) 
				return parser->getPrecedence();

			return 0;
		}

		Token Consume();
		Token Consume(TokenType expected);

		Token LookAhead(int num = 0);

		bool Match(TokenType expected);
		bool MatchAndConsume(TokenType expected);

		void Register(TokenType token, InfixParselet* parselet);
		void Register(TokenType token, PrefixParselet* parselet);
		void Register(TokenType token, StatementParselet* parselet);
	};

#define EOF -1

}
#endif