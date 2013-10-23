#ifndef BINDINGS_MANAGER_H_
#define BINDINGS_MANAGER_H_

#include <map>

class BindingsManager
{
	static std::map<int32, KeyType> ScanFunction;
public:
	static void Initialize();
	static KeyType TranslateKey(int32 Scan);
};

#endif