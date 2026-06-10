#!/bin/bash
# MeshtasticDS Phase 1: Podman Environment Setup
# Run this script to prepare your system for the build

set -e

echo "========== MeshtasticDS Phase 1: Podman Setup =========="
echo ""

# Step 1: Install Podman
echo "[1/4] Installing Podman..."
if command -v podman &> /dev/null; then
    echo "✓ Podman already installed: $(podman --version)"
else
    echo "Installing podman package..."
    sudo apt-get update && sudo apt-get install -y podman
    echo "✓ Podman installed"
fi

# Step 2: Create nodocker file
echo ""
echo "[2/4] Configuring nodocker file..."
if [ -f /etc/containers/nodocker ]; then
    echo "✓ /etc/containers/nodocker already exists"
else
    echo "Creating /etc/containers/nodocker..."
    sudo mkdir -p /etc/containers
    sudo touch /etc/containers/nodocker
    echo "✓ Created /etc/containers/nodocker"
fi

# Step 3: Configure registries.conf
echo ""
echo "[3/4] Configuring registries.conf..."
if [ -f /etc/containers/registries.conf ]; then
    if grep -q "docker.io" /etc/containers/registries.conf; then
        echo "✓ registries.conf already configured"
    else
        echo "Updating registries.conf..."
        sudo tee /etc/containers/registries.conf > /dev/null <<'EOF'
[[registry]]
prefix = ""
location = "docker.io"

[registries.search]
registries = ["docker.io"]
EOF
        echo "✓ Updated /etc/containers/registries.conf"
    fi
else
    echo "Creating registries.conf..."
    sudo mkdir -p /etc/containers
    sudo tee /etc/containers/registries.conf > /dev/null <<'EOF'
[[registry]]
prefix = ""
location = "docker.io"

[registries.search]
registries = ["docker.io"]
EOF
    echo "✓ Created /etc/containers/registries.conf"
fi

# Step 4: Verify installation
echo ""
echo "[4/4] Verifying Podman installation..."
podman --version
echo ""
echo "Running verification test: podman run hello-world"
podman pull alpine >/dev/null 2>&1 && podman run alpine echo "✓ Podman works!" || echo "⚠ Warning: test failed, but Podman may still work"

echo ""
echo "========== Phase 1 Complete =========="
echo "Next steps:"
echo "1. Run Phase 2: podman build -t meshtastic-3ds ."
echo "2. Run Phase 3: mkdir -p out && podman run --rm -v \"\$(pwd)/out:/out\" meshtastic-3ds"
echo ""
