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
	switch (operation)
	{
	case TokenType::Plus:
	case TokenType::AddAssign:
		this->out.push_back(IntermediateInstruction(InstructionType::Add));
		break;	
	case TokenType::Asterisk:
	case TokenType::MultiplyAssign:
		this->out.push_back(IntermediateInstruction(InstructionType::Mul));
		break;	
	case TokenType::Minus:
	case TokenType::SubtractAssign:
		this->out.push_back(IntermediateInstruction(InstructionType::Sub));
		break;
	case TokenType::Slash:
	case TokenType::DivideAssign:
		this->out.push_back(IntermediateInstruction(InstructionType::Div));
		break;
	case TokenType::Modulo:
		this->out.push_back(IntermediateInstruction(InstructionType::Modulus));
		break;
	case TokenType::Equals:
		this->out.push_back(IntermediateInstruction(InstructionType::Eq));
		break;
	case TokenType::NotEqual:
		this->out.push_back(IntermediateInstruction(InstructionType::NotEq));
		break;
	case TokenType::LessThan:
		this->out.push_back(IntermediateInstruction(InstructionType::Lt));
		break;
	case TokenType::GreaterThan:
		this->out.push_back(IntermediateInstruction(InstructionType::Gt));
		break;
	case TokenType::LessThanEqual:
		this->out.push_back(IntermediateInstruction(InstructionType::LtE));
		break;
	case TokenType::GreaterThanEqual:
		this->out.push_back(IntermediateInstruction(InstructionType::GtE));
		break;
	case TokenType::Or:
		this->out.push_back(IntermediateInstruction(InstructionType::BOr));
		break;
	case TokenType::And:
		this->out.push_back(IntermediateInstruction(InstructionType::BAnd));
		break;
	}
}

void CompilerContext::UnaryOperation(TokenType operation)
{
	switch (operation)
	{
	case TokenType::Increment:
		this->out.push_back(IntermediateInstruction(InstructionType::Incr));
		break;
	case TokenType::Decrement:
		this->out.push_back(IntermediateInstruction(InstructionType::Decr));
		break;	
	case TokenType::Minus:
		this->out.push_back(IntermediateInstruction(InstructionType::Negate));
		break;
	}
}
