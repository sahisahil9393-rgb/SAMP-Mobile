#include "../../main.h"
#include "../gui.h"
#include "inventorybutton.h"

extern UI* pUI;

void InventoryButton::draw(ImGuiRenderer* renderer)
{
    const ImVec2 topLeft = absolutePosition();
    const ImVec2 bottomRight = topLeft + size();

    // Small dark translucent button, matching the mobile HUD controls.
    renderer->drawRect(
        topLeft,
        bottomRight,
        focused()
            ? ImColor(0.28f, 0.40f, 0.75f, 0.95f)
            : ImColor(0.05f, 0.05f, 0.07f, 0.86f),
        true
    );

    renderer->drawRect(
        topLeft + ImVec2(2.0f, 2.0f),
        bottomRight - ImVec2(2.0f, 2.0f),
        ImColor(0.80f, 0.84f, 0.92f, 0.85f),
        false,
        2.0f
    );

    // Simple vector suitcase/bag icon. It does not depend on the game cache.
    const float width = size().x;
    const float height = size().y;
    const float left = topLeft.x + width * 0.25f;
    const float right = topLeft.x + width * 0.75f;
    const float top = topLeft.y + height * 0.34f;
    const float bottom = topLeft.y + height * 0.76f;
    const float handleY = topLeft.y + height * 0.25f;
    const ImColor iconColor(0.92f, 0.94f, 0.98f, 1.0f);

    renderer->drawLine(
        ImVec2(topLeft.x + width * 0.40f, handleY),
        ImVec2(topLeft.x + width * 0.60f, handleY),
        iconColor,
        3.0f
    );
    renderer->drawLine(
        ImVec2(topLeft.x + width * 0.40f, handleY),
        ImVec2(topLeft.x + width * 0.36f, top),
        iconColor,
        3.0f
    );
    renderer->drawLine(
        ImVec2(topLeft.x + width * 0.60f, handleY),
        ImVec2(topLeft.x + width * 0.64f, top),
        iconColor,
        3.0f
    );

    renderer->drawRect(
        ImVec2(left, top),
        ImVec2(right, bottom),
        ImColor(0.12f, 0.13f, 0.17f, 1.0f),
        true
    );
    renderer->drawRect(
        ImVec2(left, top),
        ImVec2(right, bottom),
        iconColor,
        false,
        2.5f
    );
    renderer->drawLine(
        ImVec2(left, top + height * 0.13f),
        ImVec2(right, top + height * 0.13f),
        iconColor,
        2.0f
    );

    Widget::draw(renderer);
}

void InventoryButton::touchPopEvent()
{
    if (pUI && pUI->inventorypanel())
        pUI->inventorypanel()->toggle();
}