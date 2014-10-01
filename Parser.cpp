#include "Parser.h"
#include "Token.h"
#undef EOF

using namespace Jet;

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
	this->Register(TokenType::LessThanEqual, new BinaryOperatorParselet(2, false));
	this->Register(TokenType::GreaterThanEqual, new BinaryOperatorParselet(2, false));

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

	this->Register(TokenType::Break, new BreakParselet());
	this->Register(TokenType::Continue, new ContinueParselet());
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

Token Parser::LookAhead(unsigned int num)
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

	return Token("test", 0, TokenType::EOF, "EOF");
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

