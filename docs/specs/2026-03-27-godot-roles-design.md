# Godot Role Skills Design

**Date:** 2026-03-27
**Goal:** Add 9 role-as-context skills to the godot plugin covering gameplay, graphics, environment, vfx, animation, audio, physics, ai, and networking. Update the entry skill, /godot command, and code-reviewer agent.

---

## Approach

Role-as-context (Option A): each skill sets the "hat" Claude wears for that role, lists key Godot nodes/APIs, and delegates implementation details to existing domain skills (godot:coder, godot:shader, godot:scene, etc.). Skills are thin and focused — no duplication of existing content.

---

## New Skills

| Skill | Key Godot APIs | Delegates to |
|---|---|---|
| `godot:gameplay` | `CharacterBody3D`, `Input`, `InputMap`, `SceneTree`, `FileAccess`, `process_mode` | `godot:coder`, `godot:signals`, `godot:physics` |
| `godot:graphics` | `WorldEnvironment`, `Environment`, `DirectionalLight3D`, `SDFGI`, `LightmapGI`, `ReflectionProbe`, `Camera3D` | `godot:shader`, `godot:docs` |
| `godot:environment` | `NavigationRegion3D`, `GridMap`, `CSGShape3D`, `OccluderInstance3D`, `StaticBody3D`, `HeightMapShape3D` | `godot:scene`, `godot:architect` |
| `godot:vfx` | `GPUParticles3D`, `CPUParticles3D`, sub-emitters, attractors, `VisualShader` | `godot:shader`, `godot:scene` |
| `godot:animation` | `AnimationPlayer`, `AnimationTree`, `AnimationNodeStateMachine`, `SkeletonIK3D`, blend spaces | `godot:coder`, `godot:signals` |
| `godot:audio` | `AudioStreamPlayer3D`, `AudioBus`, `AudioEffect`, `AudioStreamRandomizer`, `Area3D` reverb zones | `godot:scene`, `godot:signals` |
| `godot:physics` | `RigidBody3D`, `Area3D`, `ShapeCast3D`, `RayCast3D`, collision layers/masks, joints, `PhysicsServer3D` | `godot:coder`, `godot:docs` |
| `godot:ai` | `NavigationAgent3D`, avoidance, enum state machines, `Area3D` detection, groups | `godot:coder`, `godot:signals` |
| `godot:networking` | `ENetMultiplayerPeer`, `@rpc`, `MultiplayerSynchronizer`, `MultiplayerSpawner`, authority patterns | `godot:coder`, `godot:architect` |

---

## Entry Skill + Command Updates

`skills/godot/SKILL.md` and `commands/godot.md` both get a new **Role Skills** table appended after the Process Skills table:

```markdown
## Role Skills — When to Use Each

| Skill | Use when |
|---|---|
| `godot:gameplay` | Implementing player, input, game mechanics, state machines, save/load |
| `godot:graphics` | Setting up lighting, environment, post-processing, GI, reflection probes |
| `godot:environment` | Building game worlds — terrain, navigation mesh, occlusion, GridMap |
| `godot:vfx` | Creating particle effects, trails, impacts, screen effects |
| `godot:animation` | Working with AnimationPlayer, AnimationTree, blend spaces, IK |
| `godot:audio` | Setting up sounds, music, spatial audio, audio buses |
| `godot:physics` | Collision layers, joints, raycasts, physics bodies, PhysicsServer3D |
| `godot:ai` | NPC pathfinding, NavigationAgent3D, behavior state machines |
| `godot:networking` | Multiplayer, RPC, scene replication, lobby systems |
```

---

## code-reviewer Agent Update

Append a **Role-Specific Checks** section to `agents/code-reviewer.md`:

```markdown
## Role-Specific Checks

Apply the relevant section based on what was implemented:

**Gameplay**
- Input polling in `_process` only — not `_physics_process` (unless physics-driven)
- State machines use `enum` + `match`, not long if/elif chains
- `process_mode = WHEN_PAUSED` set correctly on UI nodes
- Save data uses `ResourceSaver` or `FileAccess`, never plain `Dictionary` to disk

**Graphics**
- `WorldEnvironment` is a scene singleton, not duplicated per scene
- `LightmapGI` baked before shipping — no unbaked instances in production
- `ReflectionProbe` update mode set to `ONCE` unless dynamic environment

**Environment**
- Navigation mesh baked via `NavigationRegion3D.bake_navigation_mesh()`
- `OccluderInstance3D` placed around large static meshes
- `GridMap` cell size matches collision shape size

**VFX**
- `GPUParticles3D` preferred over `CPUParticles3D` unless targeting low-end
- Sub-emitters use `emit_particle()` signal, not spawning new scenes
- VFX scenes are standalone (instanced, not embedded)

**Animation**
- `AnimationTree` active before calling `travel()`
- Root motion extracted via `root_motion_position` not manual transform
- `await animation_finished` used instead of timer hacks

**Audio**
- All sounds routed through named `AudioBus` (not Master directly)
- `AudioStreamPlayer3D` max_distance set — not left at default infinity
- `AudioStreamRandomizer` used for repeated one-shots (footsteps, hits)

**Physics**
- Collision layers documented (comment in scene or autoload)
- Physics logic in `_physics_process`, never `_process`
- `ShapeCast3D` / `RayCast3D` preferred over `move_and_collide` for queries

**AI**
- `NavigationAgent3D.velocity_computed` signal used for avoidance
- State machine transitions guarded — no direct state assignment from outside
- Detection uses `Area3D` body_entered, not per-frame distance checks

**Networking**
- `@rpc("any_peer")` avoided — prefer `@rpc("authority")`
- `MultiplayerSynchronizer` used for state, not manual RPC spam
- Authority checked before mutating game state: `if not is_multiplayer_authority(): return`
```

---

## Files Changed

**Created (9 new skills):**
- `skills/gameplay/SKILL.md`
- `skills/graphics/SKILL.md`
- `skills/environment/SKILL.md`
- `skills/vfx/SKILL.md`
- `skills/animation/SKILL.md`
- `skills/audio/SKILL.md`
- `skills/physics/SKILL.md`
- `skills/ai/SKILL.md`
- `skills/networking/SKILL.md`

**Modified:**
- `skills/godot/SKILL.md` — add Role Skills table
- `commands/godot.md` — add Role Skills table
- `agents/code-reviewer.md` — add Role-Specific Checks section
