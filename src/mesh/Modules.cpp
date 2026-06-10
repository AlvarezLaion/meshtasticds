#include "mesh/MeshModule.h"
#include "modules/BridgeModule.h"

// Fix #20: store the pointer so the module is not leaked. BridgeModule's base-class
// constructor self-registers with the module list; keeping the pointer here ensures
// the destructor is reachable and prevents tooling warnings about unused allocations.
static BridgeModule *bridgeModule = nullptr;

void setupModules() {
    bridgeModule = new BridgeModule();
}
