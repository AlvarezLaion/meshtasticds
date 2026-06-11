#include "ui_handler.h"

// ── Constructor / Destructor ──────────────────────────────────────────────────

UIHandler::UIHandler()
    : m_topTarget(nullptr), m_botTarget(nullptr), m_textBuf(nullptr)
    , m_cursorVisible(true), m_initialized(false)
    , m_connLabel("[DISCONNECTED]")
    , m_keyHints("[A]Type [X]Send [B]Del [Y]Reconn")
{}

UIHandler::~UIHandler() { Shutdown(); }

// ── Init ──────────────────────────────────────────────────────────────────────

bool UIHandler::Init() {
    if (!C3D_Init(C3D_DEFAULT_CMDBUF_SIZE)) return false;

    if (!C2D_Init(C2D_DEFAULT_MAX_OBJECTS)) {
        C3D_Fini();
        return false;
    }

    C2D_Prepare();

    m_topTarget = C2D_CreateScreenTarget(GFX_TOP, GFX_LEFT);
    if (!m_topTarget) {
        C2D_Fini();
        C3D_Fini();
        return false;
    }

    m_botTarget = C2D_CreateScreenTarget(GFX_BOTTOM, GFX_LEFT);
    if (!m_botTarget) {
        C2D_Fini();
        C3D_Fini();
        return false;
    }

    m_textBuf = C2D_TextBufNew(2048);
    if (!m_textBuf) {
        C2D_Fini();
        C3D_Fini();
        return false;
    }

    m_initialized = true;
    return true;
}

// ── Shutdown ──────────────────────────────────────────────────────────────────

void UIHandler::Shutdown() {
    if (!m_initialized) return;

    C2D_TextBufDelete(m_textBuf);
    m_textBuf = nullptr;

    C2D_Fini();
    C3D_Fini();

    m_initialized = false;
}

// ── AddMessage ────────────────────────────────────────────────────────────────

void UIHandler::AddMessage(const std::string& text, MsgOrigin origin) {
    if (m_messages.size() >= MAX_MESSAGES) {
        m_messages.erase(m_messages.begin());
    }
    m_messages.push_back({text, origin});
}

// ── SetInputBuffer ────────────────────────────────────────────────────────────

void UIHandler::SetInputBuffer(const std::string& text) {
    m_inputBuf = text;
}

// ── SetCursorVisible ──────────────────────────────────────────────────────────

void UIHandler::SetCursorVisible(bool visible) {
    m_cursorVisible = visible;
}

// ── SetConnState ──────────────────────────────────────────────────────────────

void UIHandler::SetConnState(ConnState state, const std::string& extraInfo) {
    m_connLabel = FormatConnLabel(state, extraInfo);
}

// ── FormatConnLabel (static) ──────────────────────────────────────────────────

std::string UIHandler::FormatConnLabel(ConnState state, const std::string& extra) {
    switch (state) {
        case ConnState::DISCONNECTED: return "[DISCONNECTED]";
        case ConnState::CONNECTING:   return "[CONNECTING...]";
        case ConnState::CONNECTED:    return "[CONNECTED :: " + extra + "]";
        case ConnState::LOST:         return "[CONNECTION LOST]";
        default:                      return "[DISCONNECTED]";
    }
}

// ── Render ────────────────────────────────────────────────────────────────────

void UIHandler::Render() {
    C2D_TextBufClear(m_textBuf);   // MUST be before C3D_FrameBegin

    C3D_FrameBegin(C3D_FRAME_SYNCDRAW);

    C2D_TargetClear(m_topTarget, COL_BG);
    C2D_SceneBegin(m_topTarget);
    DrawTopScreen();

    C2D_TargetClear(m_botTarget, COL_BG);
    C2D_SceneBegin(m_botTarget);
    DrawBottomScreen();

    C3D_FrameEnd(0);
}

// ── DrawTopScreen ─────────────────────────────────────────────────────────────

void UIHandler::DrawTopScreen() {
    // 1. Amber header fill
    C2D_DrawRectSolid(0, 0, 0.1f, TOP_W, HEADER_H, COL_AMBER);

    // 2. Outer border
    DrawBorder(1, 1, TOP_W - 2, TOP_H - 2, COL_AMBER);

    // 3. Header divider
    C2D_DrawRectSolid(0, HEADER_H, 0.1f, TOP_W, BORDER_THICK, COL_AMBER);

    // 4. Status bar divider
    C2D_DrawRectSolid(0, TOP_H - FOOTER_H - BORDER_THICK, 0.1f, TOP_W, BORDER_THICK, COL_AMBER);

    // 5. Header text LEFT — verbatim string required
    DrawText(".:eLoRa.:.3Ds:.", 4, 2, 0.5f, UI_FONT_SCALE, COL_BG);

    // 6. Header text RIGHT
    DrawText("[MESH CHAT]", 280, 2, 0.5f, UI_FONT_SCALE, COL_BG);

    // 7. Message area
    float msgAreaBottom = TOP_H - FOOTER_H - BORDER_THICK;
    for (int i = 0; i < (int)m_messages.size(); ++i) {
        float y = HEADER_H + BORDER_THICK + MSG_AREA_PAD + i * MSG_LINE_H;
        if (y + MSG_LINE_H > msgAreaBottom) break;

        const Message& msg = m_messages[i];
        std::string prefix;
        u32 color;

        switch (msg.origin) {
            case MsgOrigin::RECEIVED:
                prefix = "> ";
                color  = COL_GREEN;
                break;
            case MsgOrigin::SELF:
                prefix = "> You: ";
                color  = COL_GREEN_SELF;
                break;
            case MsgOrigin::SYSTEM:
                prefix = "  ";
                color  = COL_AMBER;
                break;
            case MsgOrigin::WARNING:
                prefix = "! ";
                color  = COL_ORANGE;
                break;
            case MsgOrigin::ERROR:
                prefix = "X ";
                color  = COL_WHITE;
                break;
            default:
                prefix = "> ";
                color  = COL_GREEN;
                break;
        }

        DrawText(prefix + msg.text, MSG_AREA_PAD, y, 0.5f, MSG_FONT_SCALE, color);
    }

    // 8. Status bar
    DrawText(m_connLabel, MSG_AREA_PAD, TOP_H - FOOTER_H + 2, 0.5f, UI_FONT_SCALE, COL_AMBER);

    // 9. Scanlines (topmost layer)
    DrawScanlines(TOP_W, TOP_H);
}

// ── DrawBottomScreen ──────────────────────────────────────────────────────────

void UIHandler::DrawBottomScreen() {
    // 1. Outer border
    DrawBorder(1, 1, BOT_W - 2, BOT_H - 2, COL_AMBER);

    // 2. "INPUT:" label
    DrawText("INPUT:", 8, 4, 0.5f, UI_FONT_SCALE, COL_AMBER);

    // 3. Divider under label
    C2D_DrawRectSolid(0, HEADER_H, 0.1f, BOT_W, BORDER_THICK, COL_AMBER);

    // 4. Input text with cursor
    std::string inputDisplay = "> " + m_inputBuf + (m_cursorVisible ? "_" : " ");
    DrawText(inputDisplay, 8, HEADER_H + BORDER_THICK + 4, 0.5f, MSG_FONT_SCALE, COL_GREEN);

    // 5. Key hints divider
    C2D_DrawRectSolid(0, BOT_H - FOOTER_H - BORDER_THICK, 0.1f, BOT_W, BORDER_THICK, COL_AMBER);

    // 6. Key hints text
    DrawText(m_keyHints, 8, BOT_H - FOOTER_H + 2, 0.5f, MSG_FONT_SCALE * 0.9f, COL_AMBER);

    // 7. Scanlines (topmost layer)
    DrawScanlines(BOT_W, BOT_H);
}

// ── DrawBorder ────────────────────────────────────────────────────────────────

void UIHandler::DrawBorder(float x, float y, float w, float h, u32 color) {
    // Top
    C2D_DrawRectSolid(x, y, 0.1f, w, BORDER_THICK, color);
    // Bottom
    C2D_DrawRectSolid(x, y + h - BORDER_THICK, 0.1f, w, BORDER_THICK, color);
    // Left
    C2D_DrawRectSolid(x, y, 0.1f, BORDER_THICK, h, color);
    // Right
    C2D_DrawRectSolid(x + w - BORDER_THICK, y, 0.1f, BORDER_THICK, h, color);
}

// ── DrawScanlines ─────────────────────────────────────────────────────────────

void UIHandler::DrawScanlines(float screenW, float screenH) {
    for (float sy = 0.0f; sy < screenH; sy += 4.0f) {
        C2D_DrawRectSolid(0.0f, sy, 0.9f, screenW, 2.0f, COL_SCANLINE);
    }
}

// ── DrawText ──────────────────────────────────────────────────────────────────

void UIHandler::DrawText(const std::string& str, float x, float y, float z,
                         float scale, u32 color) {
    C2D_Text t;
    C2D_TextParse(&t, m_textBuf, str.c_str());
    C2D_TextOptimize(&t);
    C2D_DrawText(&t, C2D_WithColor, x, y, z, scale, scale, color);
}
