#pragma once

#include <3ds.h>
#include <citro2d.h>
#include <string>
#include <vector>

// ── Cyberpunk / Pip-Boy / Matrix palette ─────────────────────────────────
#define COL_BG         C2D_Color32(0x00, 0x00, 0x00, 0xFF)
#define COL_GREEN      C2D_Color32(0x00, 0xFF, 0x00, 0xFF)
#define COL_GREEN_SELF C2D_Color32(0x66, 0xFF, 0x66, 0xFF)
#define COL_AMBER      C2D_Color32(0xFF, 0xB0, 0x00, 0xFF)
#define COL_ORANGE     C2D_Color32(0xFF, 0x80, 0x00, 0xFF)
#define COL_WHITE      C2D_Color32(0xFF, 0xFF, 0xFF, 0xFF)
#define COL_DIM_GREEN  C2D_Color32(0x00, 0x60, 0x00, 0xFF)
#define COL_SCANLINE   C2D_Color32(0x00, 0x00, 0x00, 0x60)

// ── Layout ────────────────────────────────────────────────────────────────
#define TOP_W           400
#define TOP_H           240
#define BOT_W           320
#define BOT_H           240
#define HEADER_H        20
#define FOOTER_H        16
#define BORDER_THICK    2
#define MSG_FONT_SCALE  0.42f
#define UI_FONT_SCALE   0.45f
#define MSG_LINE_H      16
#define MAX_MESSAGES    12
#define MSG_AREA_PAD    4

class UIHandler {
public:
    enum class MsgOrigin { RECEIVED, SELF, SYSTEM, WARNING, ERROR };
    enum class ConnState { DISCONNECTED, CONNECTING, CONNECTED, LOST };

    UIHandler();
    ~UIHandler();

    // Call once after gfxInitDefault(), before the main loop.
    bool Init();

    // Call once per frame — replaces gfxFlushBuffers/gfxSwapBuffers/gspWaitForVBlank.
    void Render();

    // Call at program exit before gfxExit(). Safe to call multiple times.
    void Shutdown();

    // Add a message to the scrolling log (oldest evicted when full).
    void AddMessage(const std::string& text, MsgOrigin origin = MsgOrigin::RECEIVED);

    // Update the live input preview on the bottom screen.
    void SetInputBuffer(const std::string& text);

    // Toggle the blinking cursor; call on a ~30-frame interval.
    void SetCursorVisible(bool visible);

    // Update the status bar (bottom of top screen).
    void SetConnState(ConnState state, const std::string& extraInfo = "");

private:
    C3D_RenderTarget* m_topTarget;
    C3D_RenderTarget* m_botTarget;
    C2D_TextBuf       m_textBuf;

    struct Message { std::string text; MsgOrigin origin; };
    std::vector<Message> m_messages;

    std::string m_inputBuf;
    bool        m_cursorVisible;
    bool        m_initialized;
    std::string m_connLabel;
    std::string m_keyHints;

    void DrawTopScreen();
    void DrawBottomScreen();
    void DrawBorder(float x, float y, float w, float h, u32 color);
    void DrawScanlines(float screenW, float screenH);
    void DrawText(const std::string& str, float x, float y, float z,
                  float scale, u32 color);
    static std::string FormatConnLabel(ConnState state, const std::string& extra);
};
