#include "Parser.h"

std::map<int,std::string> TokenToString; 

bool IsLetter(char c)
{
	return (c >= 'a' && c <= 'z') || ( c >= 'A' && c <= 'Z');
}

bool IsNumber(char c)
{
	return (c >= '0' && c <= '9');
}

char* Operator(TokenType t)
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

Expression* AssignParselet::parse(Parser* parser, Expression* left, Token token)
{
	Expression* right = parser->parseExpression(/*assignment prcedence -1 */);

	if (dynamic_cast<NameExpression*>(left) == 0)
	{
		std::string str = "AssignParselet: Left hand side must be a storable location!";
		throw ParserException(token.filename, token.line, str);//printf("Consume: TokenType not as expected!\n");

		return 0;
	}

	std::string name = dynamic_cast<NameExpression*>(left)->GetName();
	return new AssignExpression(name, right);
}

Expression* OperatorAssignParselet::parse(Parser* parser, Expression* left, Token token)
{
	Expression* right = parser->parseExpression(/*assignment prcedence -1 */);

	if (dynamic_cast<NameExpression*>(left) == 0)
	{
		std::string str = "OperatorAssignParselet: Left hand side must be a storable location!";
		throw ParserException(token.filename, token.line, str);//printf("Consume: TokenType not as expected!\n");

		return 0;
	}

	std::string name = dynamic_cast<NameExpression*>(left)->GetName();
	return new OperatorAssignExpression(token, name, right);
}

Expression* SwapParselet::parse(Parser* parser, Expression* left, Token token)
{
	Expression* right = parser->parseExpression(/*assignment prcedence -1 */);

	if (dynamic_cast<NameExpression*>(left) == 0)
	{
		std::string str = "SwapParselet: Left hand side must be a storable location!";
		throw ParserException(token.filename, token.line, str);//printf("Consume: TokenType not as expected!\n");
		printf("left hand side must be a name\n");
		return 0;
	}

	if (dynamic_cast<NameExpression*>(right) == 0)
	{
		std::string str = "SwapParselet: Right hand side must be a storable location!";
		throw ParserException(token.filename, token.line, str);//printf("Consume: TokenType not as expected!\n");

		printf("right hand side must be a name\n");
		return 0;
	}

	std::string name = dynamic_cast<NameExpression*>(left)->GetName();
	return new SwapExpression(name, right);
}

Expression* PrefixOperatorParselet::parse(Parser* parser, Token token)
{
	Expression* right = parser->parseExpression(precedence);
	if (right == 0)
	{
		std::string str = "PrefixOperatorParselet: Right hand side missing!";
		throw ParserException(token.filename, token.line, str);//printf("Consume: TokenType not as expected!\n");
	}
	return new PrefixExpression(token.getType(), right);//::atof(token.getText().c_str()));
}

Expression* BinaryOperatorParselet::parse(Parser* parser, Expression* left, Token token)
{
	Expression* right = parser->parseExpression(precedence - (isRight ? 1 : 0));
	if (right == 0)
	{
		std::string str = "BinaryOperatorParselet: Right hand side missing!";
		throw ParserException(token.filename, token.line, str);//printf("Consume: TokenType not as expected!\n");
	}
	return new OperatorExpression(left, token.getType(), right);//::atof(token.getText().c_str()));
}

Expression* GroupParselet::parse(Parser* parser, Token token)
{
	Expression* exp = parser->parseExpression();
	parser->Consume(TokenType::RightParen);
	return exp;
}

Expression* WhileParselet::parse(Parser* parser, Token token)
{
	parser->Consume(TokenType::LeftParen);

	auto condition = parser->parseExpression();

	parser->Consume(TokenType::RightParen);

	auto block = new ScopeExpression(parser->parseBlock());
	return new WhileExpression(token, condition, block);
}

Expression* ForParselet::parse(Parser* parser, Token token)
{
	parser->Consume(TokenType::LeftParen);

	auto initial = parser->ParseStatement(true);

	auto condition = parser->ParseStatement(true);

	auto increment = parser->parseExpression();

	parser->Consume(TokenType::RightParen);

	auto block = new ScopeExpression(parser->parseBlock());
	return new ForExpression(token, initial, condition, increment, block);
}

Expression* IfParselet::parse(Parser* parser, Token token)
{
	auto branches = new std::vector<Branch*>;
	//todo
	//take parens
	parser->Consume(TokenType::LeftParen);
	Expression* condition = parser->parseExpression();
	parser->Consume(TokenType::RightParen);

	BlockExpression* block = parser->parseBlock(true);

	Branch* branch = new Branch;
	branch->condition = condition;
	branch->block = block;
	branches->push_back(branch);

	Branch* Else = 0;
	while(true)
	{
		//look for elses
		if (parser->MatchAndConsume(TokenType::ElseIf))
		{
			//keep going
			parser->Consume(TokenType::LeftParen);
			Expression* condition = parser->parseExpression();
			parser->Consume(TokenType::RightParen);

			BlockExpression* block = parser->parseBlock(true);

			Branch* branch2 = new Branch;
			branch2->condition = condition;
			branch2->block = block;
			branches->push_back(branch2);
		}
		else if (parser->Match(TokenType::Else))
		{
			//its an else
			parser->Consume();
			BlockExpression* block = parser->parseBlock(true);

			Branch* branch2 = new Branch;
			branch2->condition = 0;
			branch2->block = block;
			Else = branch2;
			break;
		}
		else
			break;//nothing else
	}

	return new IfExpression(token, branches, Else);
}

Expression* FunctionParselet::parse(Parser* parser, Token token)
{
	auto name = parser->parseExpression();
	if (dynamic_cast<CallExpression*>(name) == 0)
	{
		printf("ERROR: function expression not formatted correctly!\n");
		return 0;
	}

	//parser->Consume(TokenType::LeftParen);

	auto arguments = dynamic_cast<CallExpression*>(name)->args;
	name = dynamic_cast<CallExpression*>(name)->left;
	/*if (!parser->MatchAndConsume(TokenType::RightParen))
	{
	do
	{
	arguments->push_back(parser->ParseStatement(false));
	}
	while( parser->MatchAndConsume(TokenType::Comma));

	parser->Consume(TokenType::RightParen);
	}*/

	auto block = new ScopeExpression(parser->parseBlock());
	return new FunctionExpression(token, name, arguments, block);
}

Expression* CallParselet::parse(Parser* parser, Expression* left, Token token)
{
	auto arguments = new std::vector<Expression*>;

	if (!parser->MatchAndConsume(TokenType::RightParen))
	{
		do
		{
			arguments->push_back(parser->ParseStatement(false));
		}
		while( parser->MatchAndConsume(TokenType::Comma));

		parser->Consume(TokenType::RightParen);
	}

	return new CallExpression(token, left, arguments);
}

Expression* ReturnParselet::parse(Parser* parser, Token token)
{
	Expression* right = 0;
	if (parser->Match(TokenType::Semicolon) == false)
		right = parser->ParseStatement(false);
	//auto arguments = new std::vector<Expression*>;

	/*if (!parser->MatchAndConsume(TokenType::RightParen))
	{
	do
	{
	arguments->push_back(parser->ParseStatement(false));
	}
	while( parser->MatchAndConsume(TokenType::Comma));

	parser->Consume(TokenType::RightParen);
	}*/

	return new ReturnExpression(token, right);//CallExpression(token, left, arguments);
}