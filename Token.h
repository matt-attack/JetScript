#ifndef _TOKEN_HEADER
#define _TOKEN_HEADER

#undef EOF

enum TokenType
{
	Name,
	Number,
	String,
	Assign,

	Minus,
	Plus,
	Asterisk,
	Slash,
	Modulo,

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

char* Operator(TokenType t);
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
	TokenType type;
	std::string text;
	unsigned int line;
	std::string filename;

	Token()
	{

	}

	Token(std::string file, unsigned int line, TokenType type, std::string txt)
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

	std::string getText()
	{
		return text;
	}
};

#endif