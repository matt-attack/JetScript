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

			std::string s;
			s += c;
			if (operators[str] == TokenType::LineComment)
			{
				//go to next line
				while(this->ConsumeChar() != '\n') {};

				continue;
			}
			else if (operators[str] == TokenType::CommentBegin)
			{
				Token n = this->Next();
				while(n.type != TokenType::CommentEnd) 
				{
					if (n.type == TokenType::EOF)
					{
						throw ParserException(n.filename, n.line, "Missing end to comment block starting at:");
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
					if (text.at(index) == '\\')
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
								txt.push_back('t');
								break;
							}
						case '"':
							{
								txt.push_back('"');
								break;
							}
						default:
							throw ParserException("test", this->linenumber, "Unhandled Escape Sequence!");
						}

						index += 2;
					}
					else if (text.at(index) == '"')
					{
						break;
					}
					else
					{
						txt.push_back(text[index++]);
					}
				}

				index++;
				return Token("test", linenumber, operators[str], txt);
			}

			return Token("test", linenumber, operators[str], str);
		}
		else if (IsLetter(c))//word
		{
			int start = index - 1;
			while (index < text.length())
			{
				char c = this->PeekChar();
				if (!IsLetter(text.at(index)))
					if (!IsNumber(text.at(index)))
						break;

				this->ConsumeChar();
			}

			std::string name = text.substr(start, index-start);
			//check if it is a keyword
			if (keywords.find(name) != keywords.end())//is keyword?
				return Token("test", linenumber, keywords[name], name);
			else//just a variable name
				return Token("test", linenumber, TokenType::Name, name);
		}
		else if (c >= '0' && c <= '9')//number
		{
			int start = index-1;
			while (index < text.length())
			{
				if (!(text[index] == '.' || (text[index] >= '0' && text[index] <= '9')))
					break;

				this->ConsumeChar();
			}

			std::string num = text.substr(start, index-start);
			return Token("test", linenumber, TokenType::Number, num);
		}
		else
		{
			//character to ignore like whitespace
		}
	}
	return Token("test", linenumber, TokenType::EOF, "EOF");
}

char Lexer::ConsumeChar()
{
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