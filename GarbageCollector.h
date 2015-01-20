#ifndef _JET_GC_HEADER
#define _JET_GC_HEADER

#include "Value.h"
#include "VMStack.h"
#include <vector>

namespace Jet
{
	class JetContext;

	class GarbageCollector
	{
		friend class Value;
		//finish moving stuff over to me
		//must free with GCFree, pointer is a bit offset to leave room for the flag
		template<class T> 
		T* GCAllocate()
		{
#undef new
			//need to call constructor
			char* buf = new char[sizeof(T)+1];
			this->gcObjects.push_back(buf);
			new (buf+1) T();
			return (T*)(buf+1);

#ifdef _DEBUG
#ifndef DBG_NEW      
#define DBG_NEW new ( _NORMAL_BLOCK , __FILE__ , __LINE__ )     
#define new DBG_NEW   
#endif
#endif
		}

		template<class T> 
		T* GCAllocate2(unsigned int size)
		{
			return (T*)((new char[sizeof(T)+1])+1);
		}

		char* GCAllocate(unsigned int size)
		{
			//this leads to less indirection, and simplified cleanup
			char* data = new char[size+1];//enough room for the flag
			this->gcObjects.push_back(data);
			return data+1;
		}

		//need to call destructor first
		template<class T>
		void GCFree(T* data)
		{
			data->~T();
			delete[] (((char*)data)-1);
		}

		void GCFree(char* data)
		{
			delete[] (data-1);
		}

		JetContext* context;
	public:
		//garbage collector stuff
		std::vector<JetArray*> arrays;
		std::vector<JetObject*> objects;
		std::vector<JetString*> strings;
		std::vector<JetUserdata*> userdata;

		std::vector<Closure*> closures;

		std::vector<char*> gcObjects;//not used atm

		std::vector<Value> nativeRefs;//a list of all objects stored natively to mark

		int allocationCounter;//used to determine when to run the GC
		int collectionCounter;
		VMStack<Value> greys;//stack of grey objects for processing

		GarbageCollector(JetContext* context);
		//~GarbageCollector(void);

		void Cleanup();

		void Run();
	private:
		void Mark();
		void Sweep();
	};
}
#endif
