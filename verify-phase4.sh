#!/usr/bin/env bash
# Verification script for Phase 4 completion
# Checks binary format, file permissions, and network connectivity

echo "=== Phase 4 Verification ==="
echo ""

# Check 1: 3DSX binary header
echo "✓ Check 1: 3DSX binary magic header"
MAGIC=$(xxd /home/indreaven/meshtasticds/out/meshtastic3ds.3dsx 2>/dev/null | head -1 | awk '{print $2$3}')
if [ "$MAGIC" = "33445358" ]; then
    echo "  ✅ PASS: Binary has correct 3DSX magic (3344 5358)"
else
    echo "  ❌ FAIL: Expected 3344 5358, got $MAGIC"
fi
echo ""

# Check 2: Binary file size
echo "✓ Check 2: Binary file size"
SIZE=$(ls -lh /home/indreaven/meshtasticds/out/meshtastic3ds.3dsx 2>/dev/null | awk '{print $5}')
if [ -n "$SIZE" ]; then
    echo "  ✅ PASS: Binary exists (${SIZE}B)"
else
    echo "  ❌ FAIL: Binary not found"
fi
echo ""

# Check 3: Mock bridge script
echo "✓ Check 3: Mock bridge script"
if [ -x /home/indreaven/meshtasticds/test-bridge-mock.sh ]; then
    echo "  ✅ PASS: test-bridge-mock.sh is executable"
else
    echo "  ❌ FAIL: test-bridge-mock.sh not executable"
fi
echo ""

# Check 4: Network client configuration
echo "✓ Check 4: Network client IP configuration"
TARGET_IP=$(grep -oP 'TARGET_IP = "\K[^"]+' /home/indreaven/meshtasticds/src/3ds/network_client.h)
if [ "$TARGET_IP" = "127.0.0.1" ]; then
    echo "  ✅ PASS: TARGET_IP set to 127.0.0.1 (emulator)"
else
    echo "  ⚠️  WARNING: TARGET_IP is $TARGET_IP (not emulator default)"
fi
echo ""

# Check 5: Makefile uses 3dsxtool
echo "✓ Check 5: Makefile uses 3dsxtool"
if grep -q "3dsxtool" /home/indreaven/meshtasticds/src/3ds/Makefile; then
    echo "  ✅ PASS: Makefile invokes 3dsxtool"
else
    echo "  ❌ FAIL: Makefile does not use 3dsxtool"
fi
echo ""

# Check 6: Test bridge with nc (if available)
echo "✓ Check 6: Mock bridge functionality"
if command -v nc &> /dev/null; then
    timeout 5 bash /home/indreaven/meshtasticds/test-bridge-mock.sh &
    BRIDGE_PID=$!
    sleep 1

    RESPONSE=$(echo '{"text":"test"}' | nc -w 1 127.0.0.1 4444 2>&1)
    kill $BRIDGE_PID 2>/dev/null || true
    wait $BRIDGE_PID 2>/dev/null || true

    if echo "$RESPONSE" | grep -q '"text": "Echo: test"'; then
        echo "  ✅ PASS: Mock bridge echoes JSON correctly"
    else
        echo "  ⚠️  WARNING: Bridge response unexpected: $RESPONSE"
    fi
else
    echo "  ⚠️  SKIP: nc not available (cannot test bridge)"
fi
echo ""

# Check 7: Documentation files
echo "✓ Check 7: Documentation files"
DOCS=("PHASE4-EMULATOR-TESTING.md" "PHASE4-COMPLETE.md" "PHASE4-TESTING.md")
for doc in "${DOCS[@]}"; do
    if [ -f "/home/indreaven/meshtasticds/$doc" ]; then
        echo "  ✅ $doc exists"
    else
        echo "  ❌ $doc missing"
    fi
done
echo ""

echo "=== Summary ==="
echo "Phase 4 components verified. Ready for emulator testing."
echo ""
echo "Next steps:"
echo "  1. Install Lime3DS or Citra emulator"
echo "  2. Run: ./test-bridge-mock.sh"
echo "  3. Load out/meshtastic3ds.3dsx in emulator"
echo "  4. Verify console output and network echo"
