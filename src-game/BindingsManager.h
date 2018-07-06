#pragma once

class BindingsManager
{
    static std::map<int32_t, KeyType> ScanFunction;

    // These are used only when translating 7K mode's bindings.
    static std::map<int32_t, int32_t> ScanFunction7K;
public:
    static void Initialize();
    static KeyType TranslateKey(int32_t Scan);
    static int32_t TranslateKey7K(int32_t Scan);
};