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
    bool WorldToScreen(JNIEnv* env, double x, double y, double z, float& sx, float& sy);
    std::vector<SkeletonData> m_RenderData;
};
