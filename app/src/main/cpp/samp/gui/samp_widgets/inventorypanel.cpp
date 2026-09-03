#include "../../main.h"
#include "../gui.h"
#include "inventorypanel.h"

namespace
{
    constexpr int kColumns = 5;
    constexpr int kRows = 4;
    constexpr int kSlotCount = kColumns * kRows;
}

InventoryPanel::InventoryPanel()
{
    // Local preview data only. It will be replaced by server data later.
    m_previewItems[0] = {
        "BAG",
        "Personal storage",
        1,
        10.0f,
        PreviewIcon::bag
    };
    m_previewItems[1] = {
        "PISTOL",
        "Weapon preview",
        24,
        1.0f,
        PreviewIcon::weapon
    };
    m_previewItems[2] = {
        "MEDKIT",
        "Medical supplies",
        2,
        0.5f,
        PreviewIcon::medkit
    };
}

void InventoryPanel::toggle()
{
    setVisible(!visible());
    m_pressedSlot = -1;
    m_pressedClose = false;
    m_pressedAction = Action::none;
}

void InventoryPanel::close()
{
    setVisible(false);
    m_pressedSlot = -1;
    m_pressedClose = false;
    m_pressedAction = Action::none;
}

bool InventoryPanel::closeButtonContains(const ImVec2& pos) const
{
    const ImVec2 panelPos = absolutePosition();
    const ImVec2 panelSize = size();
    const float buttonSize = panelSize.y * 0.14f;
    const ImVec2 buttonPos(
        panelPos.x + panelSize.x - buttonSize - panelSize.x * 0.035f,
        panelPos.y + panelSize.y * 0.035f
    );

    return pos.x >= buttonPos.x &&
           pos.x <= buttonPos.x + buttonSize &&
           pos.y >= buttonPos.y &&
           pos.y <= buttonPos.y + buttonSize;
}

int InventoryPanel::slotAt(const ImVec2& pos) const
{
    const ImVec2 panelPos = absolutePosition();
    const ImVec2 panelSize = size();
    const float paddingX = panelSize.x * 0.055f;
    const float gridTop = panelPos.y + panelSize.y * 0.23f;
    const float gap = panelSize.x * 0.018f;
    const float slotWidth =
        (panelSize.x - paddingX * 2.0f - gap * (kColumns - 1)) / kColumns;
    const float slotHeight =
        (panelSize.y * 0.58f - gap * (kRows - 1)) / kRows;
    const float slotSize = slotWidth < slotHeight ? slotWidth : slotHeight;
    const float gridWidth = slotSize * kColumns + gap * (kColumns - 1);
    const float gridLeft = panelPos.x + (panelSize.x - gridWidth) * 0.5f;

    if (slotSize <= 0.0f) return -1;

    for (int row = 0; row < kRows; ++row)
    {
        for (int column = 0; column < kColumns; ++column)
        {
            const float left = gridLeft + column * (slotSize + gap);
            const float top = gridTop + row * (slotSize + gap);

            if (pos.x >= left &&
                pos.x <= left + slotSize &&
                pos.y >= top &&
                pos.y <= top + slotSize)
            {
                return row * kColumns + column;
            }
        }
    }

    return -1;
}

InventoryPanel::Action InventoryPanel::actionAt(const ImVec2& pos) const
{
    const ImVec2 panelPos = absolutePosition();
    const ImVec2 panelSize = size();
    const float buttonHeight = panelSize.y * 0.085f;
    const float buttonWidth = panelSize.x * 0.18f;
    const float gap = panelSize.x * 0.025f;
    const float top = panelPos.y + panelSize.y * 0.875f;
    const float right = panelPos.x + panelSize.x - panelSize.x * 0.055f;
    const float dropLeft = right - buttonWidth;
    const float useLeft = dropLeft - gap - buttonWidth;

    if (pos.y < top || pos.y > top + buttonHeight)
        return Action::none;
    if (pos.x >= useLeft && pos.x <= useLeft + buttonWidth)
        return Action::use;
    if (pos.x >= dropLeft && pos.x <= dropLeft + buttonWidth)
        return Action::drop;
    return Action::none;
}

int InventoryPanel::itemCount() const
{
    int count = 0;
    for (const PreviewItem& item : m_previewItems)
    {
        if (item.name != nullptr) ++count;
    }
    return count;
}

float InventoryPanel::totalWeight() const
{
    float weight = 0.0f;
    for (const PreviewItem& item : m_previewItems)
    {
        if (item.name != nullptr) weight += item.weight;
    }
    return weight;
}

void InventoryPanel::draw(ImGuiRenderer* renderer)
{
    const ImVec2 panelPos = absolutePosition();
    const ImVec2 panelSize = size();
    const ImVec2 panelEnd = panelPos + panelSize;
    const ImColor background(0.035f, 0.045f, 0.065f, 0.96f);
    const ImColor header(0.10f, 0.14f, 0.23f, 0.98f);
    const ImColor border(0.48f, 0.58f, 0.80f, 0.92f);
    const ImColor slot(0.015f, 0.025f, 0.045f, 0.96f);
    const ImColor selected(0.20f, 0.38f, 0.72f, 0.98f);
    const ImColor white(0.92f, 0.95f, 1.0f, 1.0f);
    const ImColor secondary(0.72f, 0.78f, 0.90f, 1.0f);
    const ImColor disabled(0.36f, 0.40f, 0.48f, 0.95f);

    renderer->drawRect(panelPos, panelEnd, background, true);
    renderer->drawRect(panelPos, panelEnd, border, false, 3.0f);

    const float headerHeight = panelSize.y * 0.19f;
    renderer->drawRect(
        panelPos,
        ImVec2(panelEnd.x, panelPos.y + headerHeight),
        header,
        true
    );

    renderer->drawText(
        panelPos + ImVec2(panelSize.x * 0.055f, panelSize.y * 0.055f),
        white,
        "INVENTORY",
        true,
        UISettings::fontSize() * 0.72f
    );

    renderer->drawText(
        ImVec2(
            panelPos.x + panelSize.x * 0.72f,
            panelPos.y + panelSize.y * 0.065f
        ),
        secondary,
        [&]() {
            static char countText[24];
            snprintf(countText, sizeof(countText), "%d / 20", itemCount());
            return std::string(countText);
        }(),
        false,
        UISettings::fontSize() * 0.48f
    );

    const float closeSize = panelSize.y * 0.14f;
    const ImVec2 closePos(
        panelEnd.x - closeSize - panelSize.x * 0.035f,
        panelPos.y + panelSize.y * 0.035f
    );
    const ImColor closeColor =
        m_pressedClose ? ImColor(0.86f, 0.30f, 0.32f, 1.0f) : white;

    renderer->drawLine(
        closePos + ImVec2(closeSize * 0.25f, closeSize * 0.25f),
        closePos + ImVec2(closeSize * 0.75f, closeSize * 0.75f),
        closeColor,
        3.0f
    );
    renderer->drawLine(
        closePos + ImVec2(closeSize * 0.75f, closeSize * 0.25f),
        closePos + ImVec2(closeSize * 0.25f, closeSize * 0.75f),
        closeColor,
        3.0f
    );

    const float paddingX = panelSize.x * 0.055f;
    const float gridTop = panelPos.y + panelSize.y * 0.23f;
    const float gap = panelSize.x * 0.018f;
    const float slotWidth =
        (panelSize.x - paddingX * 2.0f - gap * (kColumns - 1)) / kColumns;
    const float slotHeight =
        (panelSize.y * 0.58f - gap * (kRows - 1)) / kRows;
    const float slotSize = slotWidth < slotHeight ? slotWidth : slotHeight;
    const float gridWidth = slotSize * kColumns + gap * (kColumns - 1);
    const float gridLeft = panelPos.x + (panelSize.x - gridWidth) * 0.5f;

    for (int row = 0; row < kRows; ++row)
    {
        for (int column = 0; column < kColumns; ++column)
        {
            const int index = row * kColumns + column;
            const ImVec2 slotPos(
                gridLeft + column * (slotSize + gap),
                gridTop + row * (slotSize + gap)
            );
            const ImVec2 slotEnd = slotPos + ImVec2(slotSize, slotSize);

            renderer->drawRect(
                slotPos,
                slotEnd,
                index == m_selectedSlot ? selected : slot,
                true
            );
            renderer->drawRect(
                slotPos,
                slotEnd,
                index == m_selectedSlot ? white : ImColor(0.18f, 0.23f, 0.34f, 1.0f),
                false,
                index == m_selectedSlot ? 2.5f : 1.5f
            );

            if (m_previewItems[index].name != nullptr)
            {
                drawPreviewIcon(
                    renderer,
                    m_previewItems[index].icon,
                    slotPos,
                    slotSize
                );

                const float badgeSize = slotSize * 0.28f;
                const ImVec2 badgePos(
                    slotEnd.x - badgeSize * 1.12f,
                    slotEnd.y - badgeSize * 1.12f
                );
                renderer->drawRect(
                    badgePos,
                    badgePos + ImVec2(badgeSize, badgeSize),
                    ImColor(0.02f, 0.025f, 0.04f, 0.94f),
                    true
                );

                char quantity[12];
                snprintf(
                    quantity,
                    sizeof(quantity),
                    "%d",
                    m_previewItems[index].quantity
                );
                renderer->drawText(
                    badgePos + ImVec2(badgeSize * 0.24f, badgeSize * 0.05f),
                    white,
                    quantity,
                    true,
                    badgeSize * 0.62f
                );
            }
        }
    }

    if (m_selectedSlot >= 0 &&
        m_selectedSlot < kSlotCount &&
        m_previewItems[m_selectedSlot].name != nullptr)
    {
        char details[128];
        snprintf(
            details,
            sizeof(details),
            "%s  |  %s  |  %.1f KG",
            m_previewItems[m_selectedSlot].name,
            m_previewItems[m_selectedSlot].description,
            m_previewItems[m_selectedSlot].weight
        );
        renderer->drawText(
            panelPos + ImVec2(panelSize.x * 0.055f, panelEnd.y - panelSize.y * 0.16f),
            secondary,
            details,
            false,
            UISettings::fontSize() * 0.42f
        );
    }

    char weightText[48];
    snprintf(
        weightText,
        sizeof(weightText),
        "%.1f / 100 KG",
        totalWeight()
    );
    renderer->drawText(
        panelPos + ImVec2(panelSize.x * 0.055f, panelEnd.y - panelSize.y * 0.075f),
        secondary,
        weightText,
        false,
        UISettings::fontSize() * 0.42f
    );

    const float buttonHeight = panelSize.y * 0.085f;
    const float buttonWidth = panelSize.x * 0.18f;
    const float buttonGap = panelSize.x * 0.025f;
    const float buttonTop = panelPos.y + panelSize.y * 0.875f;
    const float right = panelEnd.x - panelSize.x * 0.055f;
    const ImVec2 dropPos(
        right - buttonWidth,
        buttonTop
    );
    const ImVec2 usePos(
        dropPos.x - buttonGap - buttonWidth,
        buttonTop
    );

    renderer->drawRect(
        usePos,
        usePos + ImVec2(buttonWidth, buttonHeight),
        disabled,
        true
    );
    renderer->drawRect(
        usePos,
        usePos + ImVec2(buttonWidth, buttonHeight),
        ImColor(0.58f, 0.63f, 0.72f, 0.90f),
        false,
        1.5f
    );
    renderer->drawText(
        usePos + ImVec2(buttonWidth * 0.22f, buttonHeight * 0.10f),
        white,
        "USE",
        false,
        UISettings::fontSize() * 0.38f
    );

    renderer->drawRect(
        dropPos,
        dropPos + ImVec2(buttonWidth, buttonHeight),
        disabled,
        true
    );
    renderer->drawRect(
        dropPos,
        dropPos + ImVec2(buttonWidth, buttonHeight),
        ImColor(0.58f, 0.63f, 0.72f, 0.90f),
        false,
        1.5f
    );
    renderer->drawText(
        dropPos + ImVec2(buttonWidth * 0.14f, buttonHeight * 0.10f),
        white,
        "DROP",
        false,
        UISettings::fontSize() * 0.38f
    );

    if (!m_status.empty())
    {
        renderer->drawText(
            panelPos + ImVec2(panelSize.x * 0.055f, panelEnd.y - panelSize.y * 0.22f),
            ImColor(0.95f, 0.70f, 0.30f, 1.0f),
            m_status,
            false,
            UISettings::fontSize() * 0.36f
        );
    }
}

void InventoryPanel::drawPreviewIcon(
    ImGuiRenderer* renderer,
    PreviewIcon icon,
    const ImVec2& slotPos,
    float slotSize
) const
{
    const ImColor iconColor(0.86f, 0.90f, 0.98f, 1.0f);
    const ImColor iconDark(0.10f, 0.14f, 0.22f, 1.0f);
    const ImVec2 center = slotPos + ImVec2(slotSize * 0.5f, slotSize * 0.5f);

    if (icon == PreviewIcon::bag)
    {
        const float left = center.x - slotSize * 0.19f;
        const float right = center.x + slotSize * 0.19f;
        const float top = center.y - slotSize * 0.13f;
        const float bottom = center.y + slotSize * 0.22f;
        const float handleY = center.y - slotSize * 0.24f;

        renderer->drawLine(
            ImVec2(center.x - slotSize * 0.10f, handleY),
            ImVec2(center.x + slotSize * 0.10f, handleY),
            iconColor,
            2.0f
        );
        renderer->drawLine(
            ImVec2(center.x - slotSize * 0.10f, handleY),
            ImVec2(center.x - slotSize * 0.14f, top),
            iconColor,
            2.0f
        );
        renderer->drawLine(
            ImVec2(center.x + slotSize * 0.10f, handleY),
            ImVec2(center.x + slotSize * 0.14f, top),
            iconColor,
            2.0f
        );
        renderer->drawRect(
            ImVec2(left, top),
            ImVec2(right, bottom),
            iconDark,
            true
        );
        renderer->drawRect(
            ImVec2(left, top),
            ImVec2(right, bottom),
            iconColor,
            false,
            2.0f
        );
        renderer->drawLine(
            ImVec2(left, top + slotSize * 0.09f),
            ImVec2(right, top + slotSize * 0.09f),
            iconColor,
            1.5f
        );
        return;
    }

    if (icon == PreviewIcon::weapon)
    {
        const ImVec2 bodyPos(
            center.x - slotSize * 0.23f,
            center.y - slotSize * 0.07f
        );
        const ImVec2 bodyEnd(
            center.x + slotSize * 0.17f,
            center.y + slotSize * 0.08f
        );

        renderer->drawRect(bodyPos, bodyEnd, iconColor, true);
        renderer->drawLine(
            ImVec2(bodyEnd.x, center.y),
            ImVec2(center.x + slotSize * 0.31f, center.y),
            iconColor,
            3.0f
        );
        renderer->drawLine(
            ImVec2(center.x - slotSize * 0.05f, bodyEnd.y),
            ImVec2(center.x - slotSize * 0.13f, center.y + slotSize * 0.27f),
            iconColor,
            3.0f
        );
        renderer->drawLine(
            ImVec2(center.x - slotSize * 0.17f, center.y - slotSize * 0.07f),
            ImVec2(center.x - slotSize * 0.03f, center.y - slotSize * 0.21f),
            iconColor,
            2.0f
        );
        return;
    }

    // Medkit icon.
    const ImVec2 kitPos(
        center.x - slotSize * 0.22f,
        center.y - slotSize * 0.16f
    );
    const ImVec2 kitEnd(
        center.x + slotSize * 0.22f,
        center.y + slotSize * 0.20f
    );
    renderer->drawRect(kitPos, kitEnd, iconColor, true);
    renderer->drawRect(
        ImVec2(center.x - slotSize * 0.11f, center.y - slotSize * 0.24f),
        ImVec2(center.x + slotSize * 0.11f, center.y - slotSize * 0.13f),
        iconColor,
        true
    );
    renderer->drawRect(
        ImVec2(center.x - slotSize * 0.05f, center.y - slotSize * 0.08f),
        ImVec2(center.x + slotSize * 0.05f, center.y + slotSize * 0.12f),
        iconDark,
        true
    );
    renderer->drawRect(
        ImVec2(center.x - slotSize * 0.12f, center.y - slotSize * 0.01f),
        ImVec2(center.x + slotSize * 0.12f, center.y + slotSize * 0.05f),
        iconDark,
        true
    );
}

void InventoryPanel::touchEvent(const ImVec2& pos, TouchType type)
{
    if (!visible()) return;

    if (type == TouchType::push)
    {
        m_pressedClose = closeButtonContains(pos);
        m_pressedSlot = slotAt(pos);
        m_pressedAction = actionAt(pos);
        return;
    }

    if (type == TouchType::pop)
    {
        if (m_pressedClose && closeButtonContains(pos))
        {
            close();
        }
        else if (m_pressedAction != Action::none &&
                 m_pressedAction == actionAt(pos))
        {
            m_status = "DEMO ONLY - SERVER LINK REQUIRED";
        }
        else if (m_pressedSlot >= 0 && m_pressedSlot == slotAt(pos))
        {
            m_selectedSlot = m_pressedSlot;
            m_status.clear();
        }

        m_pressedSlot = -1;
        m_pressedClose = false;
        m_pressedAction = Action::none;
        return;
    }
}