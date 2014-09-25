#ifndef DBG_NEW      
#define DBG_NEW new ( _NORMAL_BLOCK , __FILE__ , __LINE__ )     
#define new DBG_NEW   
#endif

#define _CRTDBG_MAP_ALLOC
#include <crtdbg.h>

#include "Compiler.h"
#include "Parser.h"

using namespace Jet;

CompilerContext::CompilerContext(void)
{
	uuid = 0;
	this->scope = new CompilerContext::Scope;
	this->scope->level = 0;
	this->scope->previous = this->scope->next = 0;
}

CompilerContext::~CompilerContext(void)
{
	if (this->scope)
	{
		auto next = this->scope->next;
		while (next)
		{
			auto tmp = next->next;
			delete next;
			next = tmp;
		}
	}
	delete this->scope;

	//delete functions
	for (auto ii: this->functions)
		delete ii.second;
}

std::string CompilerContext::Compile(BlockExpression* expr)
{
	expr->Compile(this);

	//add a return to signify end of global code
	this->Return();

	this->Compile();

	//printf("Compile Output:\n%s\n\n", this->output.c_str());

	std::string tmp = std::move(this->output);
	this->output = "";

	if (this->scope)
	{
		auto next = this->scope->next;
		this->scope->next = 0;
		while (next)
		{
			auto tmp = next->next;
			delete next;
			next = tmp;
		}
	}

	for (auto ii: this->functions)
		delete ii.second;

	this->functions.clear();

	return tmp;//this->output.c_str();
}

bool CompilerContext::RegisterLocal(std::string name)
{
	for (int i = 0; i < this->scope->localvars.size(); i++)
	{
		if (this->scope->localvars[i] == name)
			return false;
	}
	this->scope->localvars.push_back(name);
	return true;
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
	else if (operation == TokenType::Or)
		output += "BOR;\n";
	else if (operation == TokenType::And)
		output += "BAND;\n";
}

void CompilerContext::UnaryOperation(TokenType operation)
{
	if (operation == TokenType::Increment)
		output += "Incr;\n";
	else if (operation == TokenType::Decrement)
		output += "Decr;\n";
}
