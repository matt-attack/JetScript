#ifndef _JET_EXCEPTIONS_HEADER
#define _JET_EXCEPTIONS_HEADER

namespace Jet
{
	//exceptions
	class ParserException
	{
	public:
		unsigned int line;
		::std::string file;
		::std::string reason;

		ParserException(std::string file, unsigned int line, std::string r)
		{
			this->line = line;
			this->file = file;
			reason = r;
		};
		~ParserException() {};

		const char *ShowReason() const { return reason.c_str(); }
	};

	class CompilerException
	{
	public:
		unsigned int line;
		::std::string file;
		::std::string reason;

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