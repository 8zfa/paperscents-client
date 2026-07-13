#pragma once
#include "../../modulebase.h"
#include <vector>

struct BackTrackPos
{
    double x, y, z;
    int tick;
};

class BackTrackModule : public ModuleBase
{
public:
    BackTrackModule();
    void OnEnable() override;
    void OnDisable() override;
    void OnUpdate() override;

    std::vector<BackTrackPos>& GetPositions() { return m_Positions; }

private:
    std::vector<BackTrackPos> m_Positions;
    int m_TickCounter;
    int m_UpdateInterval = 3;
    int m_FrameCounter = 0;
};
