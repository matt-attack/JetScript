#include "Compiler.h"
#include "Parser.h"


CompilerContext::CompilerContext(void)
{
	uuid = 0;
}


CompilerContext::~CompilerContext(void)
{
}

std::string CompilerContext::Compile(BlockExpression* expr)
{
	expr->Compile(this);

	//add a return to signify end of global code
	this->Return();
	
	this->Compile();

	printf("Compile Output:\n%s\n\n", this->output.c_str());
	
	std::string tmp = this->output;
	this->output = "";
	return tmp;//this->output.c_str();
}

void CompilerContext::BinaryOperation(TokenType operation)
{
	if (operation == TokenType::Plus || operation == TokenType::AddAssign)
		output += "Add;\n";
	else if (operation == TokenType::Asterisk || operation == TokenType::MultiplyAssign)
		output += "Mul;\n";
	else if (operation == TokenType::Minus || operation == TokenType::SubtractAssign)
		output += "Sub;\n";
	else if (operation == TokenType::Slash || operation == TokenType::DivideAssign)
		output += "Div;\n";
	else if (operation == TokenType::Modulo)
		output += "Mod;\n";
	else if (operation == TokenType::Equals)
		output += "Eq;\n";
	else if (operation == TokenType::NotEqual)
		output += "NotEq;\n";
	else if (operation == TokenType::LessThan)
		output += "Lt;\n";
	else if (operation == TokenType::GreaterThan)
		output += "Gt;\n";
}

void CompilerContext::UnaryOperation(TokenType operation)
{
	if (operation == TokenType::Increment)
		output += "Incr;\n";
	else if (operation == TokenType::Decrement)
		output += "Decr;\n";
}
