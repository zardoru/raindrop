#pragma once

#include "ScreenGameplay.h"
#include "GuiTextPrompt.h"

class ScreenEdit : public ScreenGameplay
{
    enum
    {
        Playing,
        Editing
    }EditScreenState;

    GUI::TextPrompt OffsetPrompt, BPMPrompt;
    uint32_t CurrentFraction;
    uint32_t CurrentTotalFraction; // basically beat snap
    uint32_t savedMeasure;
    BitmapFont EditInfo;

    GameObject* HeldObject;

    void IncreaseTotalFraction();
    void DecreaseTotalFraction();

    GameObject &GetObject();

    float YLock;
    enum
    {
        Select,
        Normal,
        Hold
    }Mode;
    Sprite GhostObject;

    bool  GridEnabled;
    int32_t GridCellSize; // ScreenSize / GridCellSize
    // float GridOffset;

    void DecreaseCurrentFraction();
    void IncreaseCurrentFraction();
    void SaveChart();
    void SwitchPreviewMode();
    void InsertMeasure();

    void OnMousePress(KeyType tkey);
    void OnMouseRelease(KeyType tkey);

    void CalculateVerticalLock();
    void RunGhostObject();
    void DrawInformation();
public:
    ScreenEdit();
    void Init(dotcur::Song *Other);
    void StartPlaying(int32_t _Measure);
    bool HandleInput(int32_t key, KeyEventType code, bool isMouseInput) override;
    bool Run(double Delta) override;
    void Cleanup() override;
};