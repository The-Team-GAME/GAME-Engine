#pragma once
#include "pch.h"

class EmptyState : public Engine::Scene
{
public:
    void on_enter() override
    {
        Log::info("[EmptyState] Entered");
    }

    void on_update([[maybe_unused]] float dt) override {}

    void on_exit() override
    {
        Log::info("[EmptyState] Exited");
    }
};
