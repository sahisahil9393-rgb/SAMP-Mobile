#pragma once

#include "../widget.h"
#include <array>
#include <string>

class InventoryPanel : public Widget
{
public:
    InventoryPanel();

    void toggle();
    void close();

    void draw(ImGuiRenderer* renderer) override;
    void touchEvent(const ImVec2& pos, TouchType type) override;

private:
    enum class PreviewIcon
    {
        bag,
        weapon,
        medkit
    };

    struct PreviewItem
    {
        const char* name = nullptr;
        const char* description = nullptr;
        int quantity = 0;
        float weight = 0.0f;
        PreviewIcon icon = PreviewIcon::bag;
    };

    enum class Action
    {
        none,
        use,
        drop
    };

    int slotAt(const ImVec2& pos) const;
    bool closeButtonContains(const ImVec2& pos) const;
    Action actionAt(const ImVec2& pos) const;
    int itemCount() const;
    float totalWeight() const;

    void drawPreviewIcon(
        ImGuiRenderer* renderer,
        PreviewIcon icon,
        const ImVec2& slotPos,
        float slotSize
    ) const;

    static constexpr int kSlotCount = 20;
    std::array<PreviewItem, kSlotCount> m_previewItems{};
    int m_pressedSlot = -1;
    bool m_pressedClose = false;
    int m_selectedSlot = -1;
    Action m_pressedAction = Action::none;
    std::string m_status;
};