#ifndef _TOKEN_HEADER
#define _TOKEN_HEADER

#ifdef _DEBUG
#ifndef DBG_NEW      
#define DBG_NEW new ( _NORMAL_BLOCK , __FILE__ , __LINE__ )     
#define new DBG_NEW   
#endif

#define _CRTDBG_MAP_ALLOC
#include <crtdbg.h>
#endif

#include <string>

namespace Jet
{
	enum class TokenType
	{
		Name,
		Number,
		String,
		Assign,

		Dot,

		Minus,
		Plus,
		Asterisk,
		Slash,
		Modulo,
		Or,
		And,


		AddAssign,
		SubtractAssign,
		MultiplyAssign,
		DivideAssign,

		NotEqual,
		Equals,

		LessThan,
		GreaterThan,
		LessThanEqual,
		GreaterThanEqual,

		RightParen,
		LeftParen,

		LeftBrace,
		RightBrace,

		LeftBracket,
		RightBracket,

		While,
		If,
		ElseIf,
		Else,

		Colon,
		Semicolon,
		Comma,

		Function,
		For,
		Local,
		Break,
		Continue,

		Swap,

		Ret,

		Increment,
		Decrement,

		LineComment,
		CommentBegin,
		CommentEnd,

		EoF
	};

	char* Operator(Jet::TokenType t);

	struct Token
	{
		Jet::TokenType type;
		::std::string text;
		unsigned int line;
		::std::string filename;

		Token()
		{

		}

		Token(::std::string file, unsigned int line, TokenType type, ::std::string txt)
		{
			this->type = type;
			this->text = txt;
			this->filename = file;
			this->line = line;
		}

		TokenType getType()
		{
			return type;
		}

		::std::string getText()
		{
			return text;
		}
	};
}

#endif