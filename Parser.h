
#ifndef _PARSER
#define _PARSER

#include <stdio.h>
#include <map>
#include <list>

#include "Token.h"
#include "Compiler.h"
#include "Expressions.h"

//exceptions
class ParserException
{
public:
	unsigned int line;
	std::string file;
	std::string reason;

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

extern std::map<int,std::string> TokenToString; 
class Lexer
{
	unsigned int index;
	std::string text;
	std::map<std::string, TokenType> operators;
	std::map<std::string, TokenType> keywords;

	unsigned int linenumber;
public:

	Lexer(std::string text)
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

		//grouping
		operators["("] = TokenType::LeftParen;
		operators[")"] = TokenType::RightParen;
		operators["{"] = TokenType::LeftBrace;
		operators["}"] = TokenType::RightBrace;

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

		for (auto ii = operators.begin(); ii != operators.end(); ii++)
		{
			TokenToString[ii->second] = ii->first;
		}
	}

	Token Next()
	{
		while (index < text.length())
		{
			char c = this->ConsumeChar();
			std::string str = text.substr(index-1, 1);
			bool found = false; int len = 0;
			for (auto ii: operators)
			{
				if (strncmp(ii.first.c_str(), &text.c_str()[index-1], ii.first.length()) == 0 && ii.first.length() > len)
				{
					len = ii.first.length();
					str = ii.first;
					found = true;
				}
			}

			if (found)
			{
				for (int i = 0; i < len-1; i++)
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
							//error
							throw ParserException(n.filename, n.line, "Missing end to comment block starting at:");
						}
						n = this->Next();
					}
					continue;
				}
				else if (operators[str] == TokenType::String)
				{
					//Token n;
					int start = index;// - 1;
					while (index < text.length())
					{
						if (text.at(index) == '"')
							break;

						index++;
					}

					std::string txt = text.substr(start, index-start);
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
				//char c = this->PeekChar();
				//if (c != ' ' && c != '\t' && c != '\n')
				//throw ParserException("test",this->linenumber, "Unknown Character:");
			}
		}
		return Token("test", linenumber, TokenType::EOF, "");
	}

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
	Expression* parse(Parser* parser, Token token)
	{
		return new NameExpression(token.getText());
	}
};

class NumberParselet: public PrefixParselet
{
public:
	Expression* parse(Parser* parser, Token token)
	{
		return new NumberExpression(::atof(token.getText().c_str()));
	}
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
	Expression* AssignParselet::parse(Parser* parser, Expression* left, Token token);
	/*{
	Expression* right = parser->parseExpression(assignment prcedence -1 );

	if (dynamic_cast<NameExpression*>(left) == 0)
	{
	printf("left hand side must be a name\n");
	return 0;
	}

	std::string name = dynamic_cast<NameExpression*>(left)->GetName();
	return new AssignExpression(name, right);
	}*/

	int getPrecedence()
	{
		return 1;
	}
};

class OperatorAssignParselet: public InfixParselet
{
public:

	Expression* OperatorAssignParselet::parse(Parser* parser, Expression* left, Token token);
	/*{
	Expression* right = parser->parseExpression(assignment prcedence -1 );

	if (dynamic_cast<NameExpression*>(left) == 0)
	{
	printf("left hand side must be a name\n");
	return 0;
	}

	std::string name = dynamic_cast<NameExpression*>(left)->GetName();
	return new AssignExpression(name, right);
	}*/

	int getPrecedence()
	{
		return 1;
	}
};

class SwapParselet: public InfixParselet
{
public:
	Expression* parse(Parser* parser, Expression* left, Token token);
	/*{
	Expression* right = parser->parseExpression(assignment prcedence -1 );

	if (dynamic_cast<NameExpression*>(left) == 0)
	{
	printf("left hand side must be a name\n");
	return 0;
	}

	std::string name = dynamic_cast<NameExpression*>(left)->GetName();
	return new AssignExpression(name, right);
	}*/

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
	/*{
	Expression* right = parser->parseExpression(precedence - (isRight ? 1 : 0));
	return new PostfixExpression(left, token.getType());//::atof(token.getText().c_str()));
	}*/

	int getPrecedence()
	{
		return precedence;
	}
};

class CallParselet: public InfixParselet
{
public:

	Expression* parse(Parser* parser, Expression* left, Token token);
	/*{
	Expression* right = parser->parseExpression(precedence - (isRight ? 1 : 0));
	return new PostfixExpression(left, token.getType());//::atof(token.getText().c_str()));
	}*/

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
	/*{
	parser->Consume(TokenType::LeftParen);

	auto condition = parser->parseExpression();

	parser->Consume(TokenType::RightParen);

	auto block = new ScopeExpression(parser->parseBlock());
	return new WhileExpression(token, condition, block);
	}*/
};

class FunctionParselet: public StatementParselet
{
public:
	FunctionParselet()
	{
		this->TrailingSemicolon = false;
	}

	Expression* parse(Parser* parser, Token token);
	/*{
	parser->Consume(TokenType::LeftParen);

	auto condition = parser->parseExpression();

	parser->Consume(TokenType::RightParen);

	auto block = new ScopeExpression(parser->parseBlock());
	return new WhileExpression(token, condition, block);
	}*/
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
	std::map<TokenType, InfixParselet*> mInfixParselets;
	std::map<TokenType, PrefixParselet*> mPrefixParselets;
	std::map<TokenType, StatementParselet*> mStatementParselets;
	std::list<Token> mRead;
public:
	Parser(Lexer* l)
	{
		this->lexer = l;

		this->Register(TokenType::Name, new NameParselet());
		this->Register(TokenType::Number, new NumberParselet());
		this->Register(TokenType::String, new StringParselet());
		this->Register(TokenType::Assign, new AssignParselet());
		this->Register(TokenType::LeftParen, new GroupParselet());

		this->Register(TokenType::Swap, new SwapParselet());

		//operator assign
		this->Register(TokenType::AddAssign, new OperatorAssignParselet());
		this->Register(TokenType::SubtractAssign, new OperatorAssignParselet());
		this->Register(TokenType::MultiplyAssign, new OperatorAssignParselet());
		this->Register(TokenType::DivideAssign, new OperatorAssignParselet());


		//prefix stuff
		this->Register(TokenType::Increment, new PrefixOperatorParselet(6));
		this->Register(TokenType::Decrement, new PrefixOperatorParselet(6));

		//postfix stuff
		this->Register(TokenType::Increment, new PostfixOperatorParselet(7));
		this->Register(TokenType::Decrement, new PostfixOperatorParselet(7));

		//boolean stuff
		this->Register(TokenType::Equals, new BinaryOperatorParselet(2, false));
		this->Register(TokenType::NotEqual, new BinaryOperatorParselet(2, false));
		this->Register(TokenType::LessThan, new BinaryOperatorParselet(2, false));
		this->Register(TokenType::GreaterThan, new BinaryOperatorParselet(2, false));

		this->Register(TokenType::Plus, new BinaryOperatorParselet(3, false));
		this->Register(TokenType::Minus, new BinaryOperatorParselet(3, false));
		this->Register(TokenType::Asterisk, new BinaryOperatorParselet(4, false));
		this->Register(TokenType::Slash, new BinaryOperatorParselet(4, false));
		this->Register(TokenType::Modulo, new BinaryOperatorParselet(4, false));

		//function stuff
		this->Register(TokenType::LeftParen, new CallParselet());

		//statements
		this->Register(TokenType::While, new WhileParselet()); 
		this->Register(TokenType::If, new IfParselet());
		this->Register(TokenType::Function, new FunctionParselet());
		this->Register(TokenType::Ret, new ReturnParselet());
		this->Register(TokenType::For, new ForParselet());
	}

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

	Expression* ParseStatement(bool takeTrailingSemicolon = true)//call this until out of tokens (hit EOF)
	{
		Token token = LookAhead();
		StatementParselet* statement = mStatementParselets[token.getType()];

		Expression* result;
		if (statement == 0)
		{
			result = parseExpression();

			if (takeTrailingSemicolon)
				Consume(TokenType::Semicolon);

			return result;
		}

		token = Consume();
		result = statement->parse(this, token);

		if (takeTrailingSemicolon && statement->TrailingSemicolon)
			Consume(TokenType::Semicolon);

		return result;
	}

	BlockExpression* parseBlock(bool allowsingle = true)
	{
		std::vector<Expression*>* statements = new std::vector<Expression*>;

		if (allowsingle && !Match(TokenType::LeftBrace))
		{
			statements->push_back(this->ParseStatement());
			return new BlockExpression(statements);
		}

		Consume(TokenType::LeftBrace);

		while (!Match(TokenType::RightBrace))
		{
			statements->push_back(this->ParseStatement());
		}

		Consume(TokenType::RightBrace);
		return new BlockExpression(statements);
	}

	BlockExpression* parseAll()
	{
		auto statements = new std::vector<Expression*>;
		while (!Match(TokenType::EOF))
		{
			statements->push_back(this->ParseStatement());
		}
		auto n = new BlockExpression(statements);
		n->SetParent(0);//go through and setup parents
		return n;
	}

	int getPrecedence() {
		InfixParselet* parser = mInfixParselets[LookAhead(0).getType()];
		if (parser != 0) 
			return parser->getPrecedence();

		return 0;
	}

	Token Consume()
	{
		LookAhead();

		auto temp = mRead.front();
		mRead.pop_front();
		return temp;//mRead.remove(0);
	}

	Token Consume(TokenType expected)
	{
		LookAhead();
		auto temp = mRead.front();
		if (temp.getType() != expected)
		{
			std::string str = "Consume: TokenType not as expected! Expected: " + TokenToString[expected] + " Got: " + temp.text;
			throw ParserException(temp.filename, temp.line, str);//printf("Consume: TokenType not as expected!\n");
		}
		mRead.pop_front();
		return temp;
	}

	Token LookAhead(int num = 0)
	{
		while (num >= mRead.size())
		{
			mRead.push_back(lexer->Next());
		}

		int c = 0;
		for (auto ii: mRead)
		{
			if (c++ == num)
			{
				return ii;
			}
		}

		//return mRead.get(num);
	}

	bool Match(TokenType expected)
	{
		Token token = LookAhead();
		if (token.getType() != expected)
		{
			return false;
		}

		//Consume();
		return true;
	}

	bool MatchAndConsume(TokenType expected)
	{
		Token token = LookAhead();
		if (token.getType() != expected)
		{
			return false;
		}

		Consume();
		return true;
	}

	void Register(TokenType token, InfixParselet* parselet)
	{
		this->mInfixParselets[token] = parselet;
	}

	void Register(TokenType token, PrefixParselet* parselet)
	{
		this->mPrefixParselets[token] = parselet;
	}

	void Register(TokenType token, StatementParselet* parselet)
	{
		this->mStatementParselets[token] = parselet;
	}
};

#define EOF -1


#endif