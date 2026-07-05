#pragma once
#include "../../modulebase.h"
#include "../../../java/java.h"
#include <vector>
#include <string>
#include <imgui.h>

struct ArmorPiece
{
    std::string displayName;
    int itemDamage;
};

class ArmorHUDModule : public ModuleBase
{
public:
    ArmorHUDModule();
    void OnEnable() override;
    void OnDisable() override;
    void OnRender() override;

private:
    std::vector<ArmorPiece> GetArmor(JNIEnv* env, jobject player);
};
