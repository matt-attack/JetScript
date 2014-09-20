#include "Parser.h"
#include "Token.h"
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

char* Jet::Operator(TokenType t)
{
	if (t == TokenType::Plus)
		return "+";
	else if (t == TokenType::Minus)
		return "-";
	else if (t == TokenType::Asterisk)
		return "*";
	else if (t == TokenType::Slash)
		return "/";
	else if (t == TokenType::Modulo)
		return "%";
	else if (t == TokenType::Comma)
		return ",";
	else if (t == TokenType::Increment)
		return "++";
	else if (t == TokenType::Decrement)
		return "--";
	else if (t == TokenType::Equals)
		return "==";
	else if (t == TokenType::NotEqual)
		return "!=";
	else if (t == TokenType::Semicolon)
		return ";";
	else if (t == TokenType::RightBrace)
		return "}";
	return "";
}

Parser::Parser(Lexer* l)
{
	this->lexer = l;

	this->Register(TokenType::Name, new NameParselet());
	this->Register(TokenType::Number, new NumberParselet());
	this->Register(TokenType::String, new StringParselet());
	this->Register(TokenType::Assign, new AssignParselet());

	this->Register(TokenType::LeftParen, new GroupParselet());

	this->Register(TokenType::Swap, new SwapParselet());

	this->Register(TokenType::Dot, new MemberParselet());
	this->Register(TokenType::LeftBrace, new ObjectParselet());

	//array/index stuffs
	this->Register(TokenType::LeftBracket, new ArrayParselet());
	this->Register(TokenType::LeftBracket, new IndexParselet());//postfix

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

	//math
	this->Register(TokenType::Plus, new BinaryOperatorParselet(4, false));
	this->Register(TokenType::Minus, new BinaryOperatorParselet(4, false));
	this->Register(TokenType::Asterisk, new BinaryOperatorParselet(5, false));
	this->Register(TokenType::Slash, new BinaryOperatorParselet(5, false));
	this->Register(TokenType::Modulo, new BinaryOperatorParselet(5, false));
	this->Register(TokenType::Or, new BinaryOperatorParselet(3, false));//or
	this->Register(TokenType::And, new BinaryOperatorParselet(3, false));//and


	//function stuff
	this->Register(TokenType::LeftParen, new CallParselet());

	//lambda
	this->Register(TokenType::Function, new LambdaParselet());

	//statements
	this->Register(TokenType::While, new WhileParselet()); 
	this->Register(TokenType::If, new IfParselet());
	this->Register(TokenType::Function, new FunctionParselet());
	this->Register(TokenType::Ret, new ReturnParselet());
	this->Register(TokenType::For, new ForParselet());
	this->Register(TokenType::Local, new LocalParselet());
}

Parser::~Parser()
{
	for (auto ii: this->mInfixParselets)
		delete ii.second;

	for (auto ii: this->mPrefixParselets)
		delete ii.second;

	for (auto ii: this->mStatementParselets)
		delete ii.second;
};

Expression* Parser::ParseStatement(bool takeTrailingSemicolon)//call this until out of tokens (hit EOF)
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

BlockExpression* Parser::parseBlock(bool allowsingle)
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

BlockExpression* Parser::parseAll()
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

Token Parser::Consume()
{
	LookAhead();

	auto temp = mRead.front();
	mRead.pop_front();
	return temp;//mRead.remove(0);
}

Token Parser::Consume(TokenType expected)
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

Token Parser::LookAhead(int num)
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

bool Parser::Match(TokenType expected)
{
	Token token = LookAhead();
	if (token.getType() != expected)
	{
		return false;
	}

	return true;
}

bool Parser::MatchAndConsume(TokenType expected)
{
	Token token = LookAhead();
	if (token.getType() != expected)
	{
		return false;
	}

	Consume();
	return true;
}

void Parser::Register(TokenType token, InfixParselet* parselet)
{
	this->mInfixParselets[token] = parselet;
}

void Parser::Register(TokenType token, PrefixParselet* parselet)
{
	this->mPrefixParselets[token] = parselet;
}

void Parser::Register(TokenType token, StatementParselet* parselet)
{
	this->mStatementParselets[token] = parselet;
}


Token Lexer::Next()
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
						throw ParserException(n.filename, n.line, "Missing end to comment block starting at:");
					}
					n = this->Next();
				}
				continue;
			}
			else if (operators[str] == TokenType::String)
			{
				//Token n;
				int start = index;
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
		}
	}
	return Token("test", linenumber, TokenType::EOF, "");
}

