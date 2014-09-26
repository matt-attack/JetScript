#ifndef _JET_LEXER_HEADER
#define _JET_LEXER_HEADER

#include "Token.h"
#include <map>
namespace Jet
{
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
}
#endif