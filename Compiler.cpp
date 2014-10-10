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

std::vector<IntermediateInstruction> CompilerContext::Compile(BlockExpression* expr)
{
	expr->Compile(this);

	//add a return to signify end of global code
	this->Return();

	this->Compile();

	//printf("Compile Output:\n%s\n\n", this->output.c_str());

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
	this->scope->localvars.clear();

	for (auto ii: this->functions)
		delete ii.second;

	this->functions.clear();

	auto temp = std::move(this->out);
	this->out.clear();
	return std::move(temp);
}

bool CompilerContext::RegisterLocal(std::string name)
{
	for (unsigned int i = 0; i < this->scope->localvars.size(); i++)
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
		this->out.push_back(IntermediateInstruction(InstructionType::Add));
	else if (operation == TokenType::Asterisk || operation == TokenType::MultiplyAssign)
		this->out.push_back(IntermediateInstruction(InstructionType::Mul));
	else if (operation == TokenType::Minus || operation == TokenType::SubtractAssign)
		this->out.push_back(IntermediateInstruction(InstructionType::Sub));
	else if (operation == TokenType::Slash || operation == TokenType::DivideAssign)
		this->out.push_back(IntermediateInstruction(InstructionType::Div));
	else if (operation == TokenType::Modulo)
		this->out.push_back(IntermediateInstruction(InstructionType::Modulus));
	else if (operation == TokenType::Equals)
		this->out.push_back(IntermediateInstruction(InstructionType::Eq));
	else if (operation == TokenType::NotEqual)
		this->out.push_back(IntermediateInstruction(InstructionType::NotEq));
	else if (operation == TokenType::LessThan)
		this->out.push_back(IntermediateInstruction(InstructionType::Lt));
	else if (operation == TokenType::GreaterThan)
		this->out.push_back(IntermediateInstruction(InstructionType::Gt));
	else if (operation == TokenType::LessThanEqual)
		this->out.push_back(IntermediateInstruction(InstructionType::LtE));
	else if (operation == TokenType::GreaterThanEqual)
		this->out.push_back(IntermediateInstruction(InstructionType::GtE));
	else if (operation == TokenType::Or)
		this->out.push_back(IntermediateInstruction(InstructionType::BOr));
	else if (operation == TokenType::And)
		this->out.push_back(IntermediateInstruction(InstructionType::BAnd));
}

void CompilerContext::UnaryOperation(TokenType operation)
{
	if (operation == TokenType::Increment)
		this->out.push_back(IntermediateInstruction(InstructionType::Incr));
	else if (operation == TokenType::Decrement)
		this->out.push_back(IntermediateInstruction(InstructionType::Decr));
	else if (operation == TokenType::Minus)
		this->out.push_back(IntermediateInstruction(InstructionType::Negate));
}
