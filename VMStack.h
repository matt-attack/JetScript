
#ifndef _VMSTACK
#define _VMSTACK

template<class T>
class VMStack
{
	unsigned int _size;
	
public:
	T mem[5000];
	VMStack()
	{
		_size = 0;
		//mem = new T[size];
	}

	VMStack(unsigned int size)
	{
		_size = 0;
		//mem = new T[size];
	}

	~VMStack()
	{
		//delete[] this->mem;
	}

	T Pop()
	{
		return mem[--_size];
	}

	void QuickPop(int times = 1)
	{
		_size -= times;
	}

	T Peek()
	{
		return mem[_size-1];
	}

	void Push(T item)
	{
		mem[_size++] = item;
	}

	unsigned int size()
	{
		return _size;
	}
};
#endif