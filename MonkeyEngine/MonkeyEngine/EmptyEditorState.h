#pragma once

#include "SceneManager.h"

namespace Engine
{
    class EmptyEditorState : public Scene
    {
    public:
        void load()   override {}
        void init()   override {}
        void update() override {}
        void draw()   override {}
        void free()   override {}
        void unload() override {}
    };
}
