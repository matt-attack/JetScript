
#ifndef _PARSER
#define _PARSER

#ifndef DBG_NEW      
#define DBG_NEW new ( _NORMAL_BLOCK , __FILE__ , __LINE__ )     
#define new DBG_NEW   
#endif

#define _CRTDBG_MAP_ALLOC
#include <crtdbg.h>

#include <stdio.h>
#include <map>

#include "Token.h"
#include "Compiler.h"
#include "Expressions.h"
#include "Parselets.h"
#include "Lexer.h"

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


#include "Parselets.h"

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

		~Parser();

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

		Token LookAhead(unsigned int num = 0);

		bool Match(TokenType expected);
		bool MatchAndConsume(TokenType expected);

		void Register(TokenType token, InfixParselet* parselet);
		void Register(TokenType token, PrefixParselet* parselet);
		void Register(TokenType token, StatementParselet* parselet);
	};

#define EOF -1

}
#endif