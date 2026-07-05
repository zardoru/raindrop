#pragma once

class BindingsManager
{
    static std::map<int32_t, KeyType> ScanFunction;

    // These are used only when translating 7K mode's bindings.
    static std::map<int32_t, int32_t> ScanFunction7K;
public:
    static void initialize();
    static KeyType translate_key(int32_t scan);
    static int32_t translate_key_game(int32_t scan);
};