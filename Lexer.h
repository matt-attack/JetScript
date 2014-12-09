#ifndef _JET_LEXER_HEADER
#define _JET_LEXER_HEADER

#include "Token.h"
#include <map>
namespace Jet
{
	bool IsLetter(char c);
	bool IsNumber(char c);

	extern std::map<Jet::TokenType,std::string> TokenToString; 
	class Lexer
	{
		unsigned int index;
		std::istream* stream;

		std::string text;
		std::map<std::string, Jet::TokenType> operators;
		std::map<std::string, Jet::TokenType> keywords;

		unsigned int linenumber;
		std::string filename;

	public:
		Lexer(std::istream* input, std::string filename);
		Lexer(std::string text, std::string filename);

		Token Next();

	private:
		void Init();

		char ConsumeChar();
		char MatchAndConsumeChar(char c);
		char PeekChar();
	};
}
#endif