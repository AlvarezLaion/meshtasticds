#include <3ds.h>
#include <stdio.h>
#include <string.h>
#include "network_client.h"

int main(int argc, char* argv[]) {
    consoleClear();
    printf("\n=== Meshtastic 3DS Console ===\n");
    printf("Initializing...\n");

    NetworkClient netClient;

    if (!netClient.Connect()) {
        printf("ERROR: Connection to bridge failed.\n");
        printf("Make sure:\n");
        printf("  1. Heltec is running with BridgeModule\n");
        printf("  2. Wi-Fi AP 'meshtastic-bridge' is available\n");
        printf("  3. 3DS is connected to the AP at 192.168.4.1:4444\n");
    } else {
        printf("Connected to bridge!\n");
    }

    printf("\nControls:\n");
    printf("  START: Exit\n");
    printf("  A: Type message\n");
    printf("  X: Send\n");
    printf("  B: Backspace\n");

    std::string inputBuffer;

    while (aptMainLoop()) {
        hidScanInput();
        u32 keyDown = hidKeysDown();

        if (keyDown & KEY_START) break;

        // Poll for incoming messages
        std::string incoming;
        while (netClient.PollMessage(incoming)) {
            printf("[Received] %s\n", incoming.c_str());
        }

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
            }
        }

        svcSleepThread(50000000);  // Sleep 50ms to prevent busy-loop
    }

    netClient.Disconnect();
    return 0;
}
