#ifndef _JET_LEXER_HEADER
#define _JET_LEXER_HEADER

#include "Token.h"
#include <map>
#include <vector>

namespace Jet
{
	bool IsLetter(char c);
	bool IsNumber(char c);

	extern std::map<TokenType,std::string> TokenToString; 
	class Lexer
	{
		unsigned int index;
		std::istream* stream;

		std::string text;

		std::map<std::string, TokenType> operators;
		std::map<std::string, TokenType> keywords;

		std::map<char, std::vector<std::pair<std::string, TokenType>>> operatorsearch;

		unsigned int linenumber;

	public:
		Lexer(std::istream* input, std::string filename);
		Lexer(std::string text, std::string filename);

		Token Next();

		std::string filename;

	private:
		void Init();

		char ConsumeChar();
		char MatchAndConsumeChar(char c);
		char PeekChar();
	};
}
#endif