#include "Lexer.h"
#include "Parser.h"
#undef EOF

using namespace Jet;

std::map<TokenType,std::string> Jet::TokenToString; 

bool Jet::IsLetter(char c)
{
	return (c >= 'a' && c <= 'z') || ( c >= 'A' && c <= 'Z');
}

bool Jet::IsNumber(char c)
{
	return (c >= '0' && c <= '9');
}

Lexer::Lexer(::std::istream* input, std::string filename)
{
	this->stream = input;
	this->linenumber = 1;
	this->index = 0;
	this->filename = filename;

	this->Init();
}

Lexer::Lexer(::std::string text, std::string filename)
{
	this->stream = 0;
	this->linenumber = 1;
	this->index = 0;
	this->text = text;
	this->filename = filename;

	this->Init();
}

void Lexer::Init()
{
	//math and assignment
	operators["="] = TokenType::Assign;
	operators["+"] = TokenType::Plus;
	operators["-"] = TokenType::Minus;
	operators["*"] = TokenType::Asterisk;
	operators["/"] = TokenType::Slash;
	operators["%"] = TokenType::Modulo;

	operators["|"] = TokenType::Or;//or
	operators["&"] = TokenType::And;//and
	operators["^"] = TokenType::Xor;//xor
	operators["~"] = TokenType::BNot;//binary not
	operators["<<"] = TokenType::LeftShift;
	operators[">>"] = TokenType::RightShift;

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
	operators[":"] = TokenType::Colon;
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
	operators["<="] = TokenType::LessThanEqual;
	operators[">="] = TokenType::GreaterThanEqual;

	//special stuff
	operators["<>"] = TokenType::Swap;
	operators["\""] = TokenType::String;

	operators["..."] = TokenType::Ellipses;

	//comments
	operators["//"] = TokenType::LineComment;
	operators["-[["] = TokenType::CommentBegin;
	operators["]]-"] = TokenType::CommentEnd;
	operators["/*"] = TokenType::CommentBegin;
	operators["*/"] = TokenType::CommentEnd;

	//keywords
	keywords["while"] = TokenType::While;
	keywords["mientras"] = TokenType::While;
	keywords["if"] = TokenType::If;
	keywords["si"] = TokenType::If;
	keywords["elseif"] = TokenType::ElseIf;
	keywords["otrosi"] = TokenType::ElseIf;
	keywords["else"] = TokenType::Else;
	keywords["otro"] = TokenType::Else;
	keywords["fun"] = TokenType::Function;
	keywords["return"] = TokenType::Ret;
	keywords["volver"] = TokenType::Ret;
	keywords["for"] = TokenType::For;
	//add spanish mode yo!
	keywords["por"] = TokenType::For;
	keywords["local"] = TokenType::Local;
	keywords["break"] = TokenType::Break;
	keywords["romper"/*"parar"*/] = TokenType::Break;
	keywords["continue"] = TokenType::Continue;
	keywords["continuar"] = TokenType::Continue;

	keywords["const"] = TokenType::Const;

	for (auto ii = operators.begin(); ii != operators.end(); ii++)
	{
		TokenToString[ii->second] = ii->first;
	}
}

Token Lexer::Next()
{
	while (index < text.length())
	{
		char c = this->ConsumeChar();
		std::string str = text.substr(index-1, 1);
		bool found = false; unsigned int len = 0;
		for (auto ii: operators)
		{
			if (ii.first.length() <= (text.length()+1-index))//fixes crashes/overruns if input string too short like "+"
			{
				if(strncmp(ii.first.c_str(), &text.c_str()[index-1], ii.first.length()) == 0 && ii.first.length() > len)
				{
					len = ii.first.length();
					str = ii.first;
					found = true;
				}
			}
		}

		if (found)
		{
			for (unsigned int i = 0; i < len-1; i++)
				this->ConsumeChar();

			if (operators[str] == TokenType::LineComment)
			{
				//go to next line
				char c = this->ConsumeChar();
				while(c != '\n' && c != 0) 
				{
					c = this->ConsumeChar();
				};

				if (c == 0)
					break;

				continue;
			}
			else if (operators[str] == TokenType::CommentBegin)
			{
				int startline = this->linenumber;

				Token n = this->Next();
				while(n.type != TokenType::CommentEnd) 
				{
					if (n.type == TokenType::EoF)
					{
						throw ParserException(n.filename, n.line, "Missing end to comment block starting at line "+std::to_string(startline));
					}
					n = this->Next();
				}
				continue;
			}
			else if (operators[str] == TokenType::String)
			{
				std::string txt;

				int start = index;
				while (index < text.length())
				{
					if (text[index] == '\\')
					{
						//handle escape sequences
						char c = text[index+1];
						switch(c)
						{
						case 'n':
							{
								txt.push_back('\n');
								break;
							}
						case '\\':
							{
								txt.push_back('\\');
								break;
							}
						case 't':
							{
								txt.push_back('\t');
								break;
							}
						case '"':
							{
								txt.push_back('"');
								break;
							}
						default:
							throw ParserException(filename, this->linenumber, "Unhandled Escape Sequence!");
						}

						index += 2;
					}
					else if (text[index] == '"')
					{
						break;
					}
					else
					{
						txt.push_back(text[index++]);
					}
				}

				index++;
				return Token(filename, linenumber, operators[str], txt);
			}

			return Token(filename, linenumber, operators[str], str);
		}
		else if (IsLetter(c) || c == '_')//word
		{
			int start = index - 1;
			while (index < text.length())
			{
				char c = this->PeekChar();
				if (!(IsLetter(c) || c == '_'))
					if (!IsNumber(c))
						break;

				this->ConsumeChar();
			}

			std::string name = text.substr(start, index-start);
			//check if it is a keyword
			if (keywords.find(name) != keywords.end())//is keyword?
				return Token(filename, linenumber, keywords[name], name);
			else//just a variable name
				return Token(filename, linenumber, TokenType::Name, name);
		}
		else if (IsNumber(c))//number
		{
			int start = index-1;
			while (index < text.length())
			{
				char c = text[index];
				if (!(c == '.' || IsNumber(c)))
					break;

				this->ConsumeChar();
			}

			std::string num = text.substr(start, index-start);
			return Token(filename, linenumber, TokenType::Number, num);
		}
		else
		{
			//character to ignore like whitespace
		}
	}
	return Token(filename, linenumber, TokenType::EoF, "EOF");
}

char Lexer::ConsumeChar()
{
	if (index >= text.size())
		return 0;

	if (text.at(index) == '\n')
		this->linenumber++;
	return text.at(index++);
}

char Lexer::MatchAndConsumeChar(char c)
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

char Lexer::PeekChar()
{
	return text.at(index);
}