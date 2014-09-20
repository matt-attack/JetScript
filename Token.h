#ifndef _TOKEN_HEADER
#define _TOKEN_HEADER

#undef EOF

#ifndef DBG_NEW      
#define DBG_NEW new ( _NORMAL_BLOCK , __FILE__ , __LINE__ )     
#define new DBG_NEW   
#endif

#define _CRTDBG_MAP_ALLOC
#include <crtdbg.h>

#include <string>
#undef EOF

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
		Semicolon,
		Comma,
		Function,
		For,
		Local,

		Swap,

		Ret,

		Increment,
		Decrement,

		LineComment,
		CommentBegin,
		CommentEnd,

		EOF
	};

	char* Operator(Jet::TokenType t);
	/*{
	if (t == TokenType::Plus)
	return "+";
	else if (t == TokenType::Minus)
	return "-";
	else if (t == TokenType::Asterisk)
	return "*";
	else if (t == TokenType::Slash)
	return "/";
	return "";
	}*/

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