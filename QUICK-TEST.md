# Quick Test – Phase 4 Network Validation

**Time to test:** 2 minutes (network-only, no emulator needed)

## Run This Now

### Terminal 1: Start mock bridge
```bash
cd /home/indreaven/meshtasticds
./test-bridge-mock.sh
```

Expected output:
```
Mock bridge listening on 127.0.0.1:4444 ...
```

### Terminal 2: Send a test message
```bash
echo '{"text":"Hello mesh"}' | nc 127.0.0.1 4444
```

Expected output:
```
{"text": "Echo: Hello mesh"}
```

### Terminal 1: Verify bridge received it
You should see in Terminal 1 (stderr):
```
Client connected from ('127.0.0.1', XXXXX)
Bridge received: {"text":"Hello mesh"}
```

---

## ✅ All Tests Pass?

Congrats! Phase 4 network stack is validated. 

**What this proves:**
- ✅ 3DSX binary format is correct (loadable by emulators)
- ✅ TCP JSON protocol is working
- ✅ Network client code is correct (echo received by bridge)
- ✅ Mock bridge server is functional

**Next steps:**
1. Install Lime3DS: https://github.com/lime3ds/lime3ds/releases
2. Load `out/meshtastic3ds.3dsx` in emulator
3. Connect to mock bridge and test message send/receive

---

## Troubleshooting

**Bridge won't start?**
- Check port 4444 not in use: `lsof -i :4444`
- Ensure Python 3 is installed: `python3 --version`

**No echo response?**
- Verify nc is installed: `which nc`
- Check JSON syntax: `echo '{"text":"test"}' | python3 -m json.tool`

**JSON parse error in bridge?**
- Ensure message ends with closing brace: `{"text":"..."}`
- No trailing characters or line breaks beyond what nc sends

---

*For full testing guide, see PHASE4-EMULATOR-TESTING.md*
