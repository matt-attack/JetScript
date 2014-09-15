#include "JetContext.h"

using namespace Jet;

const char* JetContext::Compile(const char* code)
{
	INT64 start, rate, end;
	QueryPerformanceFrequency( (LARGE_INTEGER *)&rate );
	QueryPerformanceCounter( (LARGE_INTEGER *)&start );

	Lexer lexer = Lexer(code);
	Parser parser = Parser(&lexer);

	//printf("In: %s\n\nResult:\n", code);
	BlockExpression* result = parser.parseAll();
	//result->print();
	//printf("\n\n");

	std::string out = compiler.Compile(result);

	delete result;

	QueryPerformanceCounter( (LARGE_INTEGER *)&end );

	INT64 diff = end - start;
	double dt = ((double)diff)/((double)rate);

	printf("Took %lf seconds to compile\n\n", dt);

	char* nout = new char[out.size()+1];
	out.copy(nout, out.size(), 0);
	nout[out.size()] = 0;
	return nout;
}