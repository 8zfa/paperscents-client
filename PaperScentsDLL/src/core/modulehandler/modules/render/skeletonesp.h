#pragma once
#include "../../modulebase.h"
#include <vector>
#include <imgui.h>
#include <jni.h>

struct SkeletonBone {
    double x, y, z;
};

struct SkeletonData {
    double posX, posY, posZ;
    float limbSwing;
    float limbSwingAmount;
    float height;
};

class SkeletonESPModule : public ModuleBase
{
public:
    SkeletonESPModule();
    void OnEnable() override;
    void OnDisable() override;
    void OnUpdate() override;
    void OnRender() override;

private:
    std::vector<SkeletonData> m_RenderData;
    int m_UpdateInterval = 3;
    int m_FrameCounter = 0;
};
