#pragma once

class UIRendering
{
public:
    static UIRendering& GetInstance();

    void Initialize();
    void Render();
    void Shutdown();

private:
    UIRendering() = default;
    ~UIRendering() = default;
    UIRendering(const UIRendering&) = delete;
    UIRendering& operator=(const UIRendering&) = delete;

    bool m_Initialized = false;
};
