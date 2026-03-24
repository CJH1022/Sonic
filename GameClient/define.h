#pragma once



// Singleton 매크로
#define SINGLE(Type) private:\
						Type();\
						Type(const Type& _Other) = delete;\
						~Type();\
						friend class singleton<Type>;

#define MAX_LAYER 32

#define GET(Type, MemName) Type Get##MemName(){return m_##MemName;}
#define SET(Type, MemName) void Set##MemName(Type _Data){ m_##MemName = _Data;}
#define GET_SET(Type, MemName) GET(Type, MemName) SET(Type, MemName)

#define CLONE(Type) public: Type* Clone() { return new Type(*this); }