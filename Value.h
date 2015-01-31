#ifndef _VALUE_HEADER
#define _VALUE_HEADER

#include "JetExceptions.h"

#include <map>
#include <functional>
#include <string>
#include <memory>
#include <vector>
#include <unordered_map>

namespace Jet
{
	class JetContext;

	template<class t>
	struct GCVal
	{
		bool mark;
		bool grey;
		unsigned char refcount;//used for native functions
		t ptr;

		GCVal() { }

		GCVal(t tt)
		{
			ptr = tt;
		}
	};

	struct Value;

	enum class ValueType
	{
		//keep all garbage collectable types towards the end after NativeFunction
		//this is used for the GC being able to tell what it is quickly
		Null = 0,
		Number,
		NativeFunction,
		String,
		Object,
		Array,
		Function,
		Userdata,
	};

	static const char* ValueTypes[] = { "Null", "Number", "NativeFunction", "String" , "Object", "Array", "Function", "Userdata"};

	struct String
	{
		bool mark, grey;
		unsigned char refcount;

		unsigned int length;
		unsigned int hash;
		//string data would be stored after this point
	};

	class JetObject;
	typedef std::vector<Value> _JetArrayBacking;
	typedef GCVal<_JetArrayBacking* > JetArray;
	typedef _JetArrayBacking::iterator _JetArrayIterator;

	typedef GCVal<std::pair<void*,JetObject*> > JetUserdata;
	typedef GCVal<char*> JetString;

	typedef void _JetFunction;
	typedef Value(*_JetNativeFunc)(JetContext*,Value*, int);

	class JetContext;

	struct Function
	{
		unsigned int ptr;
		unsigned int args, locals, upvals;
		bool vararg;
		std::string name;
	};

	struct Closure
	{
		bool mark;
		bool grey;
		unsigned char refcount;

		bool closed;//marks if the closure has gone out of its parent scope and was closed
		unsigned char numupvals;

		Function* prototype;

		union
		{
			Value* cupvals;//captured values used while closed
			Value** upvals;//captured values used while not closed
		};

		Closure* prev;//parent closure
	};

	struct _JetObject;
	struct Value
	{
		ValueType type;
		union
		{
			double value;
			//this is the main struct
			struct
			{
				union
				{
					JetString* _string;
					JetObject* _object;
					JetArray* _array;
					JetUserdata* _userdata;
				};
				union
				{
					unsigned int length;//used for strings
				};
			};

			Closure* _function;//jet function
			_JetNativeFunc func;//native func
		};

		Value();

		Value(JetString* str);
		Value(JetObject* obj);
		Value(JetArray* arr);

		Value(double val);
		Value(int val);

		Value(_JetNativeFunc a);
		Value(Closure* func);

		explicit Value(JetUserdata* userdata, JetObject* prototype);

		Value& operator= (const _JetNativeFunc& func)
		{
			return *this = Value(func);
		}

		void SetPrototype(JetObject* obj);

		std::string ToString(int depth = 0) const;

		template<class T>
		inline T*& GetUserdata()
		{
			return (T*&)this->_userdata->ptr.first;
		}

		const char* Type()
		{
			return ValueTypes[(int)this->type];
		}

		operator int()
		{
			if (type == ValueType::Number)
				return (int)value;

			throw RuntimeException("Cannot convert type " + (std::string)ValueTypes[(int)this->type] + " to int!");
		};

		operator double()
		{
			if (type == ValueType::Number)
				return value;

			throw RuntimeException("Cannot convert type " + (std::string)ValueTypes[(int)this->type] + " to double!");
		}

		JetObject* GetPrototype();

		//reference counting stuff
		void AddRef();
		void Release();

		/*template<typename First, typename... Types>
		Value operator() (First f, Types... args)
		{
		print("hi");
		//this->CallHelper(args...);
		}*/
		/*template<typename First, typename Second>
		Value operator() (JetContext* context, First first, First second)
		{

		}

		template<typename First>
		Value operator() (JetContext* context, First first)
		{

		}

		Value operator() (JetContext* context)
		{

		}*/

		/*template<typename First>
		Value operator() (First f)
		{
		this->CallHelper(f);
		}*/

		/*template<typename First, typename... Types>
		void CallHelper(First t)
		{
		//actually call
		//this->CallHelper<
		}

		template<typename First>
		void CallHelper(First t)
		{
		//actually call
		}*/

		//this massively redundant case is only here because
		//c++ operator overloading resolution is dumb
		//and wants to do integer[pointer-to-object]
		//rather than value[(implicit value)const char*]
		Value& operator[] (int key);
		Value& operator[] (const char* key);
		Value& operator[] (const Value& key);

		bool operator==(const Value& other) const;

		Value operator+( const Value &other );

		Value operator-( const Value &other );

		Value operator*( const Value &other );

		Value operator/( const Value &other );

		Value operator%( const Value &other );

		Value operator|( const Value &other );

		Value operator&( const Value &other );

		Value operator^( const Value &other );

		Value operator<<( const Value &other );

		Value operator>>( const Value &other );

		Value operator~();

		Value operator-();

	private:
		Value CallMetamethod(const char* name, const Value* other);
	};


	struct ObjNode
	{
		Value first;
		Value second;

		ObjNode* next;
	};

	template <class T>
	class ObjIterator
	{
		typedef ObjNode Node;
		typedef ObjIterator<T> Iterator;
		Node* ptr;
		JetObject* parent;
	public:
		ObjIterator()
		{
			this->parent = 0;
			this->ptr = 0;
		}

		ObjIterator(JetObject* p)
		{
			this->parent = p;
			this->ptr = 0;
		}

		ObjIterator(JetObject* p, Node* node)
		{
			this->parent = p;
			this->ptr = node;
		}

		bool operator==(const Iterator& other)
		{
			return ptr == other.ptr;
		}

		bool operator!=(const Iterator& other)
		{
			return this->ptr != other.ptr;
		}

		Iterator& operator++()
		{
			if (ptr && (this->ptr-this->parent->nodes) < (this->parent->nodecount-1))
			{
				do
				{
					this->ptr++;
					if (ptr->first.type != ValueType::Null)
						return *this;
				}
				while ((this->ptr-this->parent->nodes) < (this->parent->nodecount-1));
			}
			this->ptr = 0;

			return *this;
		};

		Node*& operator->() const
		{	// return pointer to class object
			return (Node*&)this->ptr;//this does pointer magic and gives a reference to the pair containing the key and value
		}

		Node& operator*()
		{
			return *this->ptr;
		}
	};

	class JetContext;
	class JetObject
	{
		friend class ObjIterator<Value>;
		friend class Value;
		friend class GarbageCollector;
		friend class JetContext;

		bool mark, grey;
		unsigned char refcount;

		JetContext* context;
		ObjNode* nodes;
		JetObject* prototype;

		unsigned int Size;
		unsigned int nodecount;
	public:
		typedef ObjIterator<Value> Iterator;

		JetObject(JetContext* context);
		~JetObject();

		std::size_t key(const Value* v) const;

		Iterator find(const Value& key)
		{
			ObjNode* node = this->findNode(&key);
			return Iterator(this, node);
		}

		Iterator find(const char* key)
		{
			ObjNode* node = this->findNode(key);
			return Iterator(this, node);
		}

		//just looks for a node
		ObjNode* findNode(const Value* key);
		ObjNode* findNode(const char* key);

		//finds node for key or creates one if doesnt exist
		ObjNode* getNode(const Value* key);
		ObjNode* getNode(const char* key);

		//try not to use these in the vm
		Value& operator [](const Value& key);

		//special operator for strings to deal with insertions
		Value& operator [](const char* key);

		Iterator end()
		{
			return Iterator(this);
		}

		Iterator begin()
		{
			for (int i = 0; i < this->nodecount; i++)
				if (this->nodes[i].first.type != ValueType::Null)
					return Iterator(this, &nodes[i]);
			return end();
		}

		inline size_t size()
		{
			return this->Size;
		}

		void DebugPrint();

	private:
		//finds an open node in the table for inserting
		ObjNode* getFreePosition();

		//increases the size to fit new keys
		void resize();

		//memory barrier
		void Barrier();
	};

	//basically a unique_ptr for values
	class ValueRef
	{
		Value v;
	public:
		ValueRef(const Value& value)
		{
			this->v = value;
			this->v.AddRef();
		}

		ValueRef(ValueRef&& other)
		{
			this->v = other.v;
			other.v = Value();
		}

		~ValueRef()
		{
			this->v.Release();
		}

		inline operator Value()
		{
			return this->v;
		}

		inline Value* operator ->()
		{
			return &this->v;
		}

		inline Value& operator [](const char* c)
		{
			return this->v[c];
		}

		inline Value& operator [](int i)
		{
			return this->v[i];
		}

		inline Value& operator [](const Value& c)
		{
			return this->v[c];
		}
	};
}

#endif