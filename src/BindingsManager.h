#ifndef BINDINGS_MANAGER_H_
#define BINDINGS_MANAGER_H_

#include <map>

class BindingsManager
{
	static std::map<int32, KeyType> ScanFunction;

	// These are used only when translating 7K mode's bindings.
	static std::map<int32, int32> ScanFunction7K; 
public:
	static void Initialize();
	static KeyType TranslateKey(int32 Scan);
	static int32 TranslateKey7K(int32 Scan);
};

#endif