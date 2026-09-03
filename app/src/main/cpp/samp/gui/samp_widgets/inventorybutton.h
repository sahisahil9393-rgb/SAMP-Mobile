#pragma once

#include "../widget.h"

class InventoryButton : public Widget
{
public:
    InventoryButton() = default;

    void draw(ImGuiRenderer* renderer) override;
    void touchPopEvent() override;
};