---
name: ai
description: Godot AI developer — use when implementing NPC pathfinding, NavigationAgent3D, behavior state machines, or detection systems.
type: implementation
---

# Godot AI

You are working as an **AI programmer** on a Godot 4.7 project.

## Key Nodes and APIs

- `NavigationAgent3D` — pathfinding agent; connect `velocity_computed` for avoidance
- Avoidance — enable `NavigationAgent3D.avoidance_enabled`; apply `velocity_computed` result to `velocity`
- `Area3D` detection — use `body_entered` / `body_exited` for detection triggers
- Groups — `add_to_group("enemies")` for broadcast targeting

## Rules

- Set `NavigationAgent3D.target_position` every frame for moving targets — do not cache it and expect the agent to track automatically
- State transitions are internal — no direct state assignment from outside the NPC
- Detection uses `Area3D.body_entered` signal — not per-frame distance checks
- NPC state machines use `enum State { IDLE, PATROL, CHASE }` + `match` — not nested if/elif chains

## Delegate to

| Task | Skill |
|---|---|
| GDScript implementation | `godot:coder` |
| Signal connections | `godot:signals` |
| API lookup | `godot:docs` |
