#include "Parselets.h"
#include "Expressions.h"
#include "Parser.h"
#include <string>

using namespace Jet;

Expression* NameParselet::parse(Parser* parser, Token token)
{
	if (parser->LookAhead().getType() == TokenType::LeftBracket)
	{
		//array index
		parser->Consume();
		Expression* index = parser->parseExpression();
		parser->Consume(TokenType::RightBracket);

		return new IndexExpression(new NameExpression(token.getText()), index);
	}
	else
		return new NameExpression(token.getText());
}

Expression* AssignParselet::parse(Parser* parser, Expression* left, Token token)
{
	Expression* right = parser->parseExpression(/*assignment prcedence -1 */);

	if (dynamic_cast<IStorableExpression*>(left) == 0)
	{
		std::string str = "AssignParselet: Left hand side must be a storable location!";

		throw ParserException(token.filename, token.line, str);//printf("Consume: TokenType not as expected!\n");
	}

	return new AssignExpression(left, right);
}

Expression* OperatorAssignParselet::parse(Parser* parser, Expression* left, Token token)
{
	Expression* right = parser->parseExpression(/*assignment prcedence -1 */);

	if (dynamic_cast<IStorableExpression*>(left) == 0)
	{
		std::string str = "OperatorAssignParselet: Left hand side must be a storable location!";
		throw ParserException(token.filename, token.line, str);//printf("Consume: TokenType not as expected!\n");

		return 0;
	}

	return new OperatorAssignExpression(token, left, right);
}

Expression* SwapParselet::parse(Parser* parser, Expression* left, Token token)
{
	Expression* right = parser->parseExpression(/*assignment prcedence -1 */);

	if (dynamic_cast<IStorableExpression*>(left) == 0)
	{
		std::string str = "SwapParselet: Left hand side must be a storable location!";
		throw ParserException(token.filename, token.line, str);//printf("Consume: TokenType not as expected!\n");
	}

	if (dynamic_cast<IStorableExpression*>(right) == 0)
	{
		std::string str = "SwapParselet: Right hand side must be a storable location!";
		throw ParserException(token.filename, token.line, str);//printf("Consume: TokenType not as expected!\n");
	}

	//std::string name = dynamic_cast<NameExpression*>(left)->GetName();
	return new SwapExpression(left, right);
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
	auto name = new NameExpression(parser->Consume(TokenType::Name).getText());
	auto arguments = new std::vector<Expression*>;

	parser->Consume(TokenType::LeftParen);

	if (!parser->MatchAndConsume(TokenType::RightParen))
	{
		do
		{
			arguments->push_back(parser->ParseStatement(false));
		}
		while( parser->MatchAndConsume(TokenType::Comma));

		parser->Consume(TokenType::RightParen);
	}

	auto block = new ScopeExpression(parser->parseBlock());
	return new FunctionExpression(token, name, arguments, block);
}

Expression* LambdaParselet::parse(Parser* parser, Token token)
{
	parser->Consume(TokenType::LeftParen);

	auto arguments = new std::vector<Expression*>;
	while (true)
	{
		Token name = parser->Consume(TokenType::Name);

		arguments->push_back(new NameExpression(name.getText()));

		if (!parser->MatchAndConsume(TokenType::Comma))
			break;
	}

	parser->Consume(TokenType::RightParen);

	//if (parser->MatchAndConsume(TokenType::
	//if we match shorthand notation, dont parse a scope expression, and just parse the rest of this line
	//leak regarding scope expression somehow
	auto block = new ScopeExpression(parser->parseBlock());
	return new FunctionExpression(token, 0, arguments, block);
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

	return new ReturnExpression(token, right);//CallExpression(token, left, arguments);
}

Expression* LocalParselet::parse(Parser* parser, Token token)
{
	Token name = parser->Consume(TokenType::Name);

	parser->Consume(TokenType::Assign);//its possible this wont be here and it may just be a mentioning, but no assignment

	//todo code me
	Expression* right = parser->parseExpression(/*assignment prcedence -1 */);

	parser->Consume(TokenType::Semicolon);
	//do stuff with this and store and what not
	//need to add this variable to this's block expression

	return new LocalExpression(name, right);
}

Expression* ArrayParselet::parse(Parser* parser, Token token)
{
	parser->Consume(TokenType::RightBracket);
	return new ArrayExpression(token, 0);
}

Expression* IndexParselet::parse(Parser* parser, Expression* left, Token token)
{
	//parser->Consume();
	Expression* index = parser->parseExpression();
	parser->Consume(TokenType::RightBracket);

	return new IndexExpression(left, index);//::atof(token.getText().c_str()));
}

Expression* MemberParselet::parse(Parser* parser, Expression* left, Token token)
{
	//this is for const members
	Expression* member = parser->parseExpression(1);
	NameExpression* name = dynamic_cast<NameExpression*>(member);
	if (name == 0)
		throw ParserException(token.filename, token.line, "Cannot access member name that is not a string");
	auto ret = new IndexExpression(left, new StringExpression(name->GetName()));
	delete name;

	return ret;
}

Expression* ObjectParselet::parse(Parser* parser, Token token)
{
	if (parser->LookAhead().getType() == TokenType::RightBrace)
	{
		parser->Consume();//take the right brace
		//we are done, return null object
		return new ObjectExpression(0);
	}
	//parse initial values
	while(parser->LookAhead().getType() == TokenType::Name)
	{
		Token name = parser->Consume();
		if (parser->MatchAndConsume(TokenType::Comma))
		{
			//keep going
		}
		else
			break;//we are done
	}
	parser->Consume(TokenType::RightBrace);//end part
	return new ObjectExpression(0);
};