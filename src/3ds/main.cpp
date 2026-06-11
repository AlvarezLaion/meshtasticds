#include <3ds.h>
#include <malloc.h>
#include <stdio.h>
#include <stdlib.h>
#include <string>
#include "network_client.h"

// soc:u service buffer backing the BSD socket layer.
// Must be 0x1000-aligned and a multiple of 0x1000 bytes.
#define SOC_ALIGN      0x1000
#define SOC_BUFFERSIZE 0x100000

int main(int argc, char* argv[]) {
    gfxInitDefault();
    consoleInit(GFX_TOP, NULL);

    printf("\n=== Meshtastic 3DS Console ===\n");
    printf("Initializing network...\n");

    u32* socBuffer = (u32*)memalign(SOC_ALIGN, SOC_BUFFERSIZE);
    bool socReady = (socBuffer != NULL) && R_SUCCEEDED(socInit(socBuffer, SOC_BUFFERSIZE));
    if (!socReady) {
        printf("ERROR: socInit failed; networking unavailable.\n");
    }

    NetworkClient netClient;

    if (socReady) {
        printf("Connecting to bridge at %s:%u ...\n",
               NetworkClient::TARGET_IP, NetworkClient::TARGET_PORT);
        if (netClient.Connect()) {
            printf("Connected to bridge!\n");
        } else {
            printf("ERROR: Connection to bridge failed.\n");
            printf("Make sure:\n");
            printf("  1. Heltec is running with BridgeModule\n");
            printf("  2. Wi-Fi AP 'meshtastic-bridge' is available\n");
            printf("  3. 3DS is connected to that AP\n");
            printf("Press Y to retry the connection.\n");
        }
    }

    printf("\nControls:\n");
    printf("  START: Exit\n");
    printf("  A: Type message\n");
    printf("  X: Send\n");
    printf("  B: Backspace\n");
    printf("  Y: Reconnect\n");

    std::string inputBuffer;
    bool wasConnected = netClient.IsConnected();

    while (aptMainLoop()) {
        hidScanInput();
        u32 keyDown = hidKeysDown();

        if (keyDown & KEY_START) break;

        // Poll for incoming messages
        std::string incoming;
        while (netClient.PollMessage(incoming)) {
            printf("[Received] %s\n", incoming.c_str());
        }

        if (wasConnected && !netClient.IsConnected()) {
            printf("[Warn] Connection lost. Press Y to reconnect.\n");
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
                printf("[You] %s\n", inputBuffer.c_str());
            }
        }

        if (keyDown & KEY_X) {
            if (!inputBuffer.empty()) {
                if (netClient.SendText(inputBuffer)) {
                    printf("[Sent] %s\n", inputBuffer.c_str());
                    inputBuffer.clear();
                } else {
                    printf("[Error] Failed to send message\n");
                }
            }
        }

        if (keyDown & KEY_B) {
            if (!inputBuffer.empty()) {
                inputBuffer.pop_back();
                printf("[Input] %s\n", inputBuffer.c_str());
            }
        }

        if (keyDown & KEY_Y) {
            if (socReady && !netClient.IsConnected()) {
                printf("Reconnecting to %s:%u ...\n",
                       NetworkClient::TARGET_IP, NetworkClient::TARGET_PORT);
                netClient.Disconnect();
                if (netClient.Connect()) {
                    printf("Connected to bridge!\n");
                } else {
                    printf("Reconnect failed. Press Y to retry.\n");
                }
                wasConnected = netClient.IsConnected();
            }
        }

        gfxFlushBuffers();
        gfxSwapBuffers();
        gspWaitForVBlank();
    }

    netClient.Disconnect();
    if (socReady) socExit();
    free(socBuffer);
    gfxExit();
    return 0;
}
