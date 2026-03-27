#!/usr/bin/env bash
set -euo pipefail

GODOT_MODULES="D:/Projects/godot/modules/agent_bridge"
# NOTE: If running under WSL, convert to: /mnt/d/Projects/godot/modules/agent_bridge
SCRIPT_DIR="$(cd "$(dirname "$0")" && pwd)"
if [ ! -d "$SCRIPT_DIR/AgentBridge" ]; then
    echo "ERROR: AgentBridge/ not found at $SCRIPT_DIR/AgentBridge"
    exit 1
fi
TOOLKIT_DIR="$SCRIPT_DIR/AgentBridge"

echo "[AgentBridge] Linking module..."
if [ -L "$GODOT_MODULES" ]; then
    rm "$GODOT_MODULES"
elif [ -e "$GODOT_MODULES" ]; then
    echo "ERROR: $GODOT_MODULES exists and is not a symlink. Remove it manually first."
    exit 1
fi
ln -s "$TOOLKIT_DIR" "$GODOT_MODULES"
echo "[AgentBridge] Module linked: $GODOT_MODULES"

echo ""
echo "[godot plugin] Installing Claude Code plugin..."
if command -v claude &>/dev/null; then
    claude plugin install "$SCRIPT_DIR" --scope user
    echo "[godot plugin] Plugin installed (user scope)."
else
    echo "WARNING: 'claude' not found. Run manually:"
    echo "  claude plugin install $SCRIPT_DIR --scope user"
fi

echo ""
echo "[AgentBridge + godot plugin] Done. Next steps:"
echo "  1. Rebuild Godot: cd D:/Projects/godot && scons platform=windows target=editor -j32"
echo "  2. Open demo-host: D:/GodotProjects/demo-host"
echo "  3. Verify: claude plugin list"
