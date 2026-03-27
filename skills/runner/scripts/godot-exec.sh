#!/usr/bin/env bash
# godot-exec.sh — AgentBridge REST client for Godot 4.x
#
# Communicates with a running Godot editor via REST API over HTTP.
# Port is auto-detected from (in order):
#   1. AGENT_BRIDGE_PORT env var
#   2. <project>/.godot/agentbridge.port
#
# Never use raw curl in skills. Always call this script instead.

set -euo pipefail

SCRIPT_NAME="$(basename "${BASH_SOURCE[0]}")"
HOST="${AGENT_BRIDGE_HOST:-127.0.0.1}"
_PORT="${AGENT_BRIDGE_PORT:-}"

# ── REST API ──────────────────────────────────────────────────────────────────
#
#   GET  /agent/health
#   POST /agent/execute        {"code": "..."}
#   POST /agent/log            {"lines": N}  (optional, default 100)
#   POST /agent/play           {"scene": "res://..."}  (optional)
#   POST /agent/stop
#   POST /agent/save           {"path": "res://..."}  (optional)
#   POST /agent/save-all
#   POST /agent/open           {"path": "res://..."}
#   POST /agent/setting/get    {"key": "..."}
#   POST /agent/setting/set    {"key": "...", "value": ...}
#   POST /agent/resource/delete     {"path": "res://..."}
#   POST /agent/resource/duplicate  {"src": "res://...", "dst": "res://..."}
#   POST /agent/resource/ensure     {"path": "res://...", "type": "GDScript"}
#   POST /agent/batch          {"calls": [{"tool": "...", "args": {...}}, ...]}

# ── Usage ─────────────────────────────────────────────────────────────────────
usage() {
  cat >&2 <<EOF
Usage: $SCRIPT_NAME [OPTIONS]

  --health              Check editor connection and version
  --execute CODE        Run GDScript code (use '-' to read from stdin)
  --file PATH           Run GDScript from a file
  --log [N]             Fetch last N editor log lines (default: 100)
  --save-all            Save all open scenes and scripts
  --play [SCENE]        Play scene; omit SCENE to play main scene (res:// path)
  --stop                Stop the currently playing scene
  --open SCENE          Open a scene in the editor (res:// path)
  --port N              Override port (or set AGENT_BRIDGE_PORT env var)
  -h, --help            Show this help

Examples:
  $SCRIPT_NAME --health
  $SCRIPT_NAME --execute 'print(OS.get_name())'
  $SCRIPT_NAME --execute - <<'GD'
    var root = EditorInterface.get_edited_scene_root()
    print(root.name, " — ", root.get_child_count(), " children")
  GD
  $SCRIPT_NAME --file /tmp/check.gd
  $SCRIPT_NAME --log 200
  $SCRIPT_NAME --save-all
  $SCRIPT_NAME --play res://scenes/main.tscn
  $SCRIPT_NAME --stop
  $SCRIPT_NAME --open res://scenes/player.tscn
EOF
  exit 1
}

# ── Port discovery ─────────────────────────────────────────────────────────────
find_port() {
  [[ -n "$_PORT" ]] && { echo "$_PORT"; return 0; }

  local dir="${PWD}"
  while true; do
    # Current path (new binary)
    if [[ -f "${dir}/.godot/agentbridge.port" ]]; then
      local p
      p=$(tr -d '[:space:]' < "${dir}/.godot/agentbridge.port")
      if [[ "$p" =~ ^[0-9]+$ ]]; then echo "$p"; return 0; fi
    fi
[[ -f "${dir}/project.godot" ]] && break
    local parent; parent=$(dirname "$dir")
    [[ "$parent" == "$dir" ]] && break
    dir="$parent"
  done

  cat >&2 <<'EOF'
ERROR: AgentBridge port not found.
  Is the Godot editor running with the AgentBridge module compiled in?
  Expected: <project>/.godot/agentbridge.port
  Override: export AGENT_BRIDGE_PORT=<port>
EOF
  exit 1
}

# ── REST call ──────────────────────────────────────────────────────────────────
rest_call() {
  local method="$1" path="$2" body="${3:-}" timeout="${4:-30}"
  local port; port=$(find_port)

  local tmp_resp; tmp_resp=$(mktemp)
  local http_code

  if [[ -n "$body" ]]; then
    http_code=$(curl -s -o "$tmp_resp" -w '%{http_code}' \
      --max-time "$timeout" \
      -X "$method" "http://${HOST}:${port}${path}" \
      -H "Content-Type: application/json" \
      -d "$body" 2>/dev/null) || http_code="000"
  else
    http_code=$(curl -s -o "$tmp_resp" -w '%{http_code}' \
      --max-time "$timeout" \
      -X "$method" "http://${HOST}:${port}${path}" 2>/dev/null) || http_code="000"
  fi

  if [[ "$http_code" == "000" ]]; then
    rm -f "$tmp_resp"
    echo "ERROR: Cannot connect to AgentBridge on port ${port}. Is the editor running?" >&2
    exit 1
  fi

  python3 - "$tmp_resp" <<'PYEOF'
import json, sys, os
path = sys.argv[1]
with open(path) as f: raw = f.read()
os.unlink(path)
try:
    r = json.loads(raw)
except Exception as e:
    print(f"ERROR: Invalid JSON from AgentBridge: {e}", file=sys.stderr)
    sys.exit(1)
if "error" in r:
    print(f"ERROR: {r['error']}", file=sys.stderr)
    sys.exit(1)
if "output" in r:
    text = r["output"]
    sys.stdout.write(text + ("" if text.endswith("\n") else "\n"))
elif "ok" in r:
    pass  # silent success
else:
    # structured response (health, setting/get, batch, etc.) — print as JSON
    print(json.dumps(r, indent=2))
PYEOF
}

# ── Main ───────────────────────────────────────────────────────────────────────
main() {
  local cmd="" code="" scene="" log_lines=100

  [[ $# -eq 0 ]] && usage

  while [[ $# -gt 0 ]]; do
    case "$1" in
      --health)
        cmd="health"; shift ;;
      --save-all)
        cmd="save_all"; shift ;;
      --stop)
        cmd="stop"; shift ;;
      --execute)
        cmd="execute"
        if [[ "${2:-}" == "-" ]]; then
          code=$(cat); shift 2
        else
          code="${2:?'--execute requires CODE or - for stdin'}"; shift 2
        fi ;;
      --file)
        cmd="execute"
        code=$(cat "${2:?'--file requires a PATH'}"); shift 2 ;;
      --log)
        cmd="log"
        if [[ -n "${2:-}" && "${2:-}" =~ ^[0-9]+$ ]]; then
          log_lines="$2"; shift
        fi
        shift ;;
      --play)
        cmd="play"
        if [[ -n "${2:-}" && "${2:-}" != --* ]]; then
          scene="$2"; shift
        fi
        shift ;;
      --open)
        cmd="open"
        scene="${2:?'--open requires a SCENE path'}"; shift 2 ;;
      --port)
        _PORT="${2:?'--port requires N'}"; shift 2 ;;
      -h|--help) usage ;;
      *) echo "ERROR: Unknown option: $1" >&2; usage ;;
    esac
  done

  case "$cmd" in
    health)
      rest_call GET /agent/health ;;

    execute)
      local body
      body=$(printf '%s' "$code" | python3 -c "import json,sys; print(json.dumps({'code':sys.stdin.read()}))")
      rest_call POST /agent/execute "$body" 60 ;;

    log)
      rest_call POST /agent/log "{\"lines\":${log_lines}}" ;;

    save_all)
      rest_call POST /agent/save-all ;;

    play)
      if [[ -n "$scene" ]]; then
        local body
        body=$(printf '%s' "$scene" | python3 -c "import json,sys; print(json.dumps({'scene':sys.stdin.read()}))")
        rest_call POST /agent/play "$body"
      else
        rest_call POST /agent/play
      fi ;;

    stop)
      rest_call POST /agent/stop ;;

    open)
      local body
      body=$(printf '%s' "$scene" | python3 -c "import json,sys; print(json.dumps({'path':sys.stdin.read()}))")
      rest_call POST /agent/open "$body" ;;

    *)
      usage ;;
  esac
}

main "$@"
