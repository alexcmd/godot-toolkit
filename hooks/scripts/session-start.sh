#!/usr/bin/env bash
# Detect if the current working directory contains a Godot project.
# Searches up to 2 levels deep for project.godot.
# If found, outputs the godot:godot skill content as session context.

if find . -maxdepth 2 -name "project.godot" 2>/dev/null | grep -q .; then
    cat "$CLAUDE_PLUGIN_ROOT/skills/godot/SKILL.md"
fi
