#include <3ds.h>
#include <malloc.h>
#include <stdlib.h>
#include <string>
#include "network_client.h"
#include "ui_handler.h"

// soc:u service buffer backing the BSD socket layer.
// Must be 0x1000-aligned and a multiple of 0x1000 bytes.
#define SOC_ALIGN      0x1000
#define SOC_BUFFERSIZE 0x100000

int main(int argc, char* argv[]) {
    gfxInitDefault();

    UIHandler ui;
    if (!ui.Init()) {
        gfxExit();
        return 1;
    }

    ui.AddMessage(".:eLoRa.:.3Ds:. ONLINE", UIHandler::MsgOrigin::SYSTEM);

    u32* socBuffer = (u32*)memalign(SOC_ALIGN, SOC_BUFFERSIZE);
    bool socReady = (socBuffer != NULL) && R_SUCCEEDED(socInit(socBuffer, SOC_BUFFERSIZE));
    if (!socReady) {
        ui.AddMessage("ERROR: socInit failed", UIHandler::MsgOrigin::ERROR);
    }

    NetworkClient netClient;

    if (socReady) {
        ui.SetConnState(UIHandler::ConnState::CONNECTING);
        if (netClient.Connect()) {
            ui.SetConnState(UIHandler::ConnState::CONNECTED,
                std::string(NetworkClient::TARGET_IP) + ":" +
                std::to_string(NetworkClient::TARGET_PORT));
        } else {
            ui.AddMessage("Connection failed", UIHandler::MsgOrigin::ERROR);
            ui.SetConnState(UIHandler::ConnState::DISCONNECTED);
        }
    }

    ui.SetInputBuffer("");

    std::string inputBuffer;
    bool wasConnected = netClient.IsConnected();

    int cursorTimer = 0;
    bool cursorOn = true;

    while (aptMainLoop()) {
        hidScanInput();
        u32 keyDown = hidKeysDown();

        if (++cursorTimer >= 30) {
            cursorTimer = 0;
            cursorOn = !cursorOn;
            ui.SetCursorVisible(cursorOn);
        }

        if (keyDown & KEY_START) break;

        // Poll for incoming messages
        std::string incoming;
        while (netClient.PollMessage(incoming)) {
            ui.AddMessage(incoming, UIHandler::MsgOrigin::RECEIVED);
        }

        if (wasConnected && !netClient.IsConnected()) {
            ui.AddMessage("Connection lost", UIHandler::MsgOrigin::WARNING);
            ui.SetConnState(UIHandler::ConnState::LOST);
        }
        wasConnected = netClient.IsConnected();

        // Handle keyboard input
        if (keyDown & KEY_A) {
            SwkbdState swkbd;
            char inputText[256] = {0};
            swkbdInit(&swkbd, SWKBD_TYPE_NORMAL, 2, sizeof(inputText) - 1);
            swkbdSetHintText(&swkbd, "Type a message...");
            SwkbdButton btn = swkbdInputText(&swkbd, inputText, sizeof(inputText));
            if (btn == SWKBD_BUTTON_CONFIRM && inputText[0] != '\0') {
                inputBuffer = inputText;
                ui.AddMessage("You: " + inputBuffer, UIHandler::MsgOrigin::SELF);
                ui.SetInputBuffer(inputBuffer);
            }
        }

        if (keyDown & KEY_X) {
            if (!inputBuffer.empty()) {
                if (netClient.SendText(inputBuffer)) {
                    ui.AddMessage("Sent: " + inputBuffer, UIHandler::MsgOrigin::SELF);
                    ui.SetInputBuffer("");
                    inputBuffer.clear();
                } else {
                    ui.AddMessage("Send failed", UIHandler::MsgOrigin::ERROR);
                }
            }
        }

        if (keyDown & KEY_B) {
            if (!inputBuffer.empty()) {
                inputBuffer.pop_back();
                ui.SetInputBuffer(inputBuffer);
            }
        }

        if (keyDown & KEY_Y) {
            if (socReady && !netClient.IsConnected()) {
                ui.SetConnState(UIHandler::ConnState::CONNECTING);
                netClient.Disconnect();
                if (netClient.Connect()) {
                    ui.SetConnState(UIHandler::ConnState::CONNECTED,
                        std::string(NetworkClient::TARGET_IP) + ":" +
                        std::to_string(NetworkClient::TARGET_PORT));
                } else {
                    ui.AddMessage("Reconnect failed", UIHandler::MsgOrigin::WARNING);
                    ui.SetConnState(UIHandler::ConnState::DISCONNECTED);
                }
                wasConnected = netClient.IsConnected();
            }
        }

        ui.Render();
    }

    netClient.Disconnect();
    if (socReady) socExit();
    free(socBuffer);
    ui.Shutdown();
    gfxExit();
    return 0;
}
