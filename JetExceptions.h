#ifndef _JET_EXCEPTIONS_HEADER
#define _JET_EXCEPTIONS_HEADER

#include <string>

namespace Jet
{
	//exceptions
	class CompilerException
	{
	public:
		unsigned int line;
		std::string file;
		std::string reason;

		CompilerException(std::string file, unsigned int line, std::string r)
		{
			this->line = line;
			this->file = file;
			reason = r;
		};

		const char *ShowReason() const { return reason.c_str(); }
	};

	class RuntimeException
	{
	public:
		std::string reason;
		RuntimeException(std::string reason)
		{
			this->reason = reason;
		}
	};
}
#endif