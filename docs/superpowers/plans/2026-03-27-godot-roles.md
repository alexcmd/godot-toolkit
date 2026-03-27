# Godot Role Skills Implementation Plan

> **For agentic workers:** REQUIRED SUB-SKILL: Use superpowers:subagent-driven-development (recommended) or superpowers:executing-plans to implement this plan task-by-task. Steps use checkbox (`- [ ]`) syntax for tracking.

**Goal:** Add 9 role-as-context skills to the godot plugin and update the entry skill, /godot command, and code-reviewer agent.

**Architecture:** Each role skill sets a narrow "hat" Claude wears for that domain, lists the key Godot 4.7 nodes/APIs, and delegates implementation details to existing domain skills (godot:coder, godot:shader, godot:scene, etc.). Skills are thin markdown files with frontmatter — no GDScript, no compilation.

**Tech Stack:** Markdown skill files, Claude Code plugin format (frontmatter + content)

---

## File Map

**Created:**
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
- `skills/godot/SKILL.md` — append Role Skills table
- `commands/godot.md` — append Role Skills table
- `agents/code-reviewer.md` — append Role-Specific Checks section

---

### Task 1: Create gameplay, graphics, environment skills

**Files:**
- Create: `skills/gameplay/SKILL.md`
- Create: `skills/graphics/SKILL.md`
- Create: `skills/environment/SKILL.md`

- [ ] **Step 1: Create `skills/gameplay/SKILL.md`**

```markdown
---
name: gameplay
description: Godot gameplay developer — use when implementing player controllers, input handling, game mechanics, state machines, or save/load systems.
type: implementation
---

# Godot Gameplay

You are working as a **gameplay programmer** on a Godot 4.7 project.

## Key Nodes and APIs

- `CharacterBody3D` — physics-driven player/NPC movement with `move_and_slide()`
- `Input` / `InputMap` — polling actions (`Input.is_action_pressed`) and mapping
- `SceneTree` — pausing (`get_tree().paused`), scene switching, groups
- `FileAccess` — reading/writing save data
- `process_mode` — controls which nodes update when paused (`WHEN_PAUSED`, `DISABLED`)

## Rules

- Poll input in `_process` — not `_physics_process` (unless physics-driven movement)
- State machines use `enum` + `match`, not long if/elif chains
- Set `process_mode = WHEN_PAUSED` on UI nodes that must work while paused
- Save data with `ResourceSaver` or `FileAccess` — never plain `Dictionary` to disk

## Delegate to

| Task | Skill |
|---|---|
| Writing GDScript | `godot:coder` |
| Signal design | `godot:signals` |
| Physics movement | `godot:physics` |
| API lookup | `godot:docs` |
```

- [ ] **Step 2: Create `skills/graphics/SKILL.md`**

```markdown
---
name: graphics
description: Godot graphics developer — use when setting up lighting, environment, post-processing, global illumination, or reflection probes.
type: implementation
---

# Godot Graphics

You are working as a **graphics programmer** on a Godot 4.7 project.

## Key Nodes and APIs

- `WorldEnvironment` — scene-level sky, fog, tone mapping, ambient light
- `Environment` — resource attached to `WorldEnvironment` or `Camera3D`
- `DirectionalLight3D` — sun/moon with shadow map settings
- `SDFGI` — signed distance field global illumination (dynamic GI)
- `LightmapGI` — baked lightmaps for static geometry
- `ReflectionProbe` — local reflections; set `update_mode = ONCE` for static
- `Camera3D` — FOV, depth of field, environment override

## Rules

- `WorldEnvironment` is a scene singleton — do not duplicate per sub-scene
- Bake `LightmapGI` before shipping — never leave unbaked instances in production
- Set `ReflectionProbe.update_mode = ONCE` unless environment is dynamic

## Delegate to

| Task | Skill |
|---|---|
| Custom shaders | `godot:shader` |
| API lookup | `godot:docs` |
```

- [ ] **Step 3: Create `skills/environment/SKILL.md`**

```markdown
---
name: environment
description: Godot environment developer — use when building game worlds, terrain, navigation meshes, occlusion, or GridMap levels.
type: implementation
---

# Godot Environment

You are working as an **environment/level developer** on a Godot 4.7 project.

## Key Nodes and APIs

- `NavigationRegion3D` — bakes nav mesh; call `bake_navigation_mesh()` at runtime
- `GridMap` — voxel-style level building; cell size must match collision shape size
- `CSGShape3D` — constructive solid geometry for prototyping levels
- `OccluderInstance3D` — occlusion geometry; place around large static meshes
- `StaticBody3D` — non-moving collision geometry
- `HeightMapShape3D` — terrain collision shape for large landscapes

## Rules

- Bake nav mesh via `NavigationRegion3D.bake_navigation_mesh()` after placing geometry
- Place `OccluderInstance3D` around large static meshes
- `GridMap` cell size must match the collision shape size exactly

## Delegate to

| Task | Skill |
|---|---|
| Scene/node manipulation | `godot:scene` |
| Architecture decisions | `godot:architect` |
| API lookup | `godot:docs` |
```

- [ ] **Step 4: Verify files were written**

Run: `ls skills/gameplay/ skills/graphics/ skills/environment/`
Expected: `SKILL.md` present in each directory

- [ ] **Step 5: Commit**

```bash
git add skills/gameplay/SKILL.md skills/graphics/SKILL.md skills/environment/SKILL.md
git commit -m "add gameplay, graphics, environment role skills"
```

---

### Task 2: Create vfx, animation, audio skills

**Files:**
- Create: `skills/vfx/SKILL.md`
- Create: `skills/animation/SKILL.md`
- Create: `skills/audio/SKILL.md`

- [ ] **Step 1: Create `skills/vfx/SKILL.md`**

```markdown
---
name: vfx
description: Godot VFX developer — use when creating particle effects, trails, impacts, or screen-space effects.
type: implementation
---

# Godot VFX

You are working as a **VFX developer** on a Godot 4.7 project.

## Key Nodes and APIs

- `GPUParticles3D` — GPU-accelerated particles; preferred over CPU particles
- `CPUParticles3D` — fallback for low-end targets only
- Sub-emitters — use `emit_particle()` signal, not spawning new scenes
- Attractors — `GPUParticlesAttractorBox3D`, `GPUParticlesAttractorSphere3D`
- `VisualShader` — node-based shader for particle materials

## Rules

- Prefer `GPUParticles3D` over `CPUParticles3D` unless targeting low-end hardware
- Sub-emitters trigger via `emit_particle()` signal — never spawn new scenes
- VFX scenes are standalone (instantiated into the scene, not embedded)

## Delegate to

| Task | Skill |
|---|---|
| Particle/visual shaders | `godot:shader` |
| Scene instantiation | `godot:scene` |
| API lookup | `godot:docs` |
```

- [ ] **Step 2: Create `skills/animation/SKILL.md`**

```markdown
---
name: animation
description: Godot animation developer — use when working with AnimationPlayer, AnimationTree, blend spaces, state machines, or skeletal IK.
type: implementation
---

# Godot Animation

You are working as an **animation programmer** on a Godot 4.7 project.

## Key Nodes and APIs

- `AnimationPlayer` — keyframe animations; use `play()`, `queue()`, `animation_finished`
- `AnimationTree` — blending and state machine controller for `AnimationPlayer`
- `AnimationNodeStateMachine` — state machine within `AnimationTree`; call `travel()`
- `SkeletonIK3D` — inverse kinematics on `Skeleton3D`
- Blend spaces — `AnimationNodeBlendSpace1D` and `AnimationNodeBlendSpace2D` for locomotion

## Rules

- `AnimationTree.active` must be `true` before calling `travel()`
- Extract root motion via `AnimationTree.get_root_motion_position()` — not manual transform math
- Use `await animation_player.animation_finished` instead of timer workarounds

## Delegate to

| Task | Skill |
|---|---|
| GDScript animation logic | `godot:coder` |
| Signal connections | `godot:signals` |
| API lookup | `godot:docs` |
```

- [ ] **Step 3: Create `skills/audio/SKILL.md`**

```markdown
---
name: audio
description: Godot audio developer — use when setting up sounds, music, spatial audio, audio buses, or randomized one-shots.
type: implementation
---

# Godot Audio

You are working as an **audio programmer** on a Godot 4.7 project.

## Key Nodes and APIs

- `AudioStreamPlayer3D` — positional 3D audio; set `max_distance` explicitly
- `AudioBus` — named mix buses configured in the Audio panel; always route through named buses
- `AudioEffect` — effects on buses: `AudioEffectReverb`, `AudioEffectCompressor`, etc.
- `AudioStreamRandomizer` — picks from a pool of streams; use for repeated one-shots
- `Area3D` reverb zones — set `reverb_bus` on `Area3D` for zone-based reverb

## Rules

- Route all sounds through named `AudioBus` — never play directly to Master
- Set `AudioStreamPlayer3D.max_distance` — do not leave at default (infinity)
- Use `AudioStreamRandomizer` for repeated sounds (footsteps, impacts, hits)

## Delegate to

| Task | Skill |
|---|---|
| Scene/node setup | `godot:scene` |
| Signal connections | `godot:signals` |
| API lookup | `godot:docs` |
```

- [ ] **Step 4: Verify files were written**

Run: `ls skills/vfx/ skills/animation/ skills/audio/`
Expected: `SKILL.md` present in each directory

- [ ] **Step 5: Commit**

```bash
git add skills/vfx/SKILL.md skills/animation/SKILL.md skills/audio/SKILL.md
git commit -m "add vfx, animation, audio role skills"
```

---

### Task 3: Create physics, ai, networking skills

**Files:**
- Create: `skills/physics/SKILL.md`
- Create: `skills/ai/SKILL.md`
- Create: `skills/networking/SKILL.md`

- [ ] **Step 1: Create `skills/physics/SKILL.md`**

```markdown
---
name: physics
description: Godot physics developer — use when working with collision layers, joints, raycasts, physics bodies, or PhysicsServer3D.
type: implementation
---

# Godot Physics

You are working as a **physics programmer** on a Godot 4.7 project.

## Key Nodes and APIs

- `RigidBody3D` — simulated physics body; use `_integrate_forces()` for custom physics
- `Area3D` — trigger volumes for overlap detection and physics influence
- `ShapeCast3D` — sweep test along a vector; preferred for queries
- `RayCast3D` — single-ray intersection test; use `force_raycast_update()` off-physics-process
- Collision layers/masks — document all used layers with comments
- Joints — `HingeJoint3D`, `Generic6DOFJoint3D`, `PinJoint3D`
- `PhysicsServer3D` — low-level physics API for performance-critical code

## Rules

- Physics logic in `_physics_process` — never in `_process`
- Document collision layers in a comment (autoload or scene file header)
- Prefer `ShapeCast3D` / `RayCast3D` over `move_and_collide()` for queries

## Delegate to

| Task | Skill |
|---|---|
| GDScript implementation | `godot:coder` |
| API lookup | `godot:docs` |
```

- [ ] **Step 2: Create `skills/ai/SKILL.md`**

```markdown
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
- Enum state machines — `enum State { IDLE, PATROL, CHASE }` with `match` dispatch
- `Area3D` detection — use `body_entered` / `body_exited` for detection triggers
- Groups — `add_to_group("enemies")` for broadcast targeting

## Rules

- Use `NavigationAgent3D.velocity_computed` signal for avoidance — do not set velocity directly
- State transitions are internal — no direct state assignment from outside the NPC
- Detection uses `Area3D.body_entered` signal — not per-frame distance checks

## Delegate to

| Task | Skill |
|---|---|
| GDScript implementation | `godot:coder` |
| Signal connections | `godot:signals` |
| API lookup | `godot:docs` |
```

- [ ] **Step 3: Create `skills/networking/SKILL.md`**

```markdown
---
name: networking
description: Godot networking developer — use when implementing multiplayer, RPC calls, scene replication, or lobby systems.
type: implementation
---

# Godot Networking

You are working as a **networking programmer** on a Godot 4.7 project.

## Key Nodes and APIs

- `ENetMultiplayerPeer` — ENet-based peer; set on `multiplayer.multiplayer_peer`
- `@rpc` — remote procedure call annotation; default to `@rpc("authority")`
- `MultiplayerSynchronizer` — syncs properties to all peers automatically
- `MultiplayerSpawner` — replicates scene instantiation across peers
- Authority patterns — server holds `multiplayer_authority` for game state nodes

## Rules

- Avoid `@rpc("any_peer")` — prefer `@rpc("authority")` to prevent client cheating
- Use `MultiplayerSynchronizer` for state — not manual RPC spam
- Guard all state mutations: `if not is_multiplayer_authority(): return`

## Delegate to

| Task | Skill |
|---|---|
| GDScript implementation | `godot:coder` |
| Architecture decisions | `godot:architect` |
| API lookup | `godot:docs` |
```

- [ ] **Step 4: Verify files were written**

Run: `ls skills/physics/ skills/ai/ skills/networking/`
Expected: `SKILL.md` present in each directory

- [ ] **Step 5: Commit**

```bash
git add skills/physics/SKILL.md skills/ai/SKILL.md skills/networking/SKILL.md
git commit -m "add physics, ai, networking role skills"
```

---

### Task 4: Update skills/godot/SKILL.md and commands/godot.md

**Files:**
- Modify: `skills/godot/SKILL.md`
- Modify: `commands/godot.md`

Both files have identical content — append the same Role Skills table to each.

- [ ] **Step 1: Append Role Skills table to `skills/godot/SKILL.md`**

Append after the last line of the file:

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

- [ ] **Step 2: Append Role Skills table to `commands/godot.md`**

Append the same block after the last line of `commands/godot.md`:

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

- [ ] **Step 3: Verify both files end with the Role Skills table**

Run: `tail -5 skills/godot/SKILL.md && tail -5 commands/godot.md`
Expected: both files end with `| \`godot:networking\`` row

- [ ] **Step 4: Commit**

```bash
git add skills/godot/SKILL.md commands/godot.md
git commit -m "add role skills table to godot entry skill and command"
```

---

### Task 5: Update agents/code-reviewer.md

**Files:**
- Modify: `agents/code-reviewer.md`

- [ ] **Step 1: Append Role-Specific Checks section to `agents/code-reviewer.md`**

Append after the last line of the file:

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

- [ ] **Step 2: Verify the section was appended**

Run: `tail -10 agents/code-reviewer.md`
Expected: ends with the Networking block

- [ ] **Step 3: Commit**

```bash
git add agents/code-reviewer.md
git commit -m "add role-specific checks to code-reviewer agent"
```
