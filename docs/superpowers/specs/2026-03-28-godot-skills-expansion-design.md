# Godot Skills Deep Expansion — Design Spec

**Date:** 2026-03-28
**Scope:** All 18 godot:* specialist skills
**Goal:** Expand each skill from ~50 lines to ~150–250 lines with broader topic coverage, expert-level depth, and external references.

---

## Shared Template Structure

Every skill is rewritten to follow this section layout:

```
## Overview
## Key Nodes & APIs
## Core Patterns
## Common Pitfalls
## Performance
## Anti-Patterns
## References
## Delegate to
```

- **Overview** — domain ownership, philosophy, when to reach for this skill
- **Key Nodes & APIs** — table of node/class → purpose + critical properties
- **Core Patterns** — 2–5 named patterns with annotated, runnable code examples
- **Common Pitfalls** — bulleted gotchas that silently break things or waste hours
- **Performance** — budgets, flags, what's expensive vs cheap in this domain
- **Anti-Patterns** — wrong code vs right code side-by-side
- **References** — links to docs.godotengine.org + community resources (GDQuest, official blog)
- **Delegate to** — existing table, updated

Frontmatter (name, description, type) is unchanged.

---

## Per-Skill Expansion Plan

### godot:architect
**New content:**
- Scene composition vs inheritance trade-offs
- Dependency injection via constructor arguments on Resources
- Service locator anti-pattern warning (autoloads ≠ DI)
- Resource pooling pattern (ObjectPool with `PackedScene`)
- Scene ownership and `owner` property deep-dive
- Circular autoload dependency detection
- Node count budgets (performance)
- When to use `Node` vs `RefCounted` vs `Resource`

### godot:coder
**New content:**
- Match expressions, lambdas (`func(x): return x * 2`), inner classes
- Static typing benefits: autocomplete, crash-on-error vs silent null
- Async patterns: `await signal`, coroutine chaining
- Property syntax with get/set blocks (replaces setget)
- Reference vs value types (Resource is reference, Vector3 is value)
- Freed object safety: `is_instance_valid()`
- Avoiding string-based node access (`$"../../../Player"` smell)
- GDScript performance: typed arrays, avoid `Dictionary` hot paths

### godot:shader
**New content:**
- Vertex shader patterns (wave deformation, billboard)
- Normal mapping, rim lighting, fresnel
- Dissolve / cutout effects with `discard`
- Outline shader via inverted hull
- Screen-space: `SCREEN_TEXTURE`, `DEPTH_TEXTURE`, `NORMAL_ROUGHNESS_TEXTURE`
- Particle shader: color over lifetime, size over lifetime curves
- Color space gotcha: `source_color` hint vs raw vec4
- Shader include files (`#include`)
- Precision: `mediump` vs `highp`

### godot:ui
**New content:**
- StyleBox types: `StyleBoxFlat`, `StyleBoxTexture`, `StyleBoxLine`
- Theme override hierarchy (node → type → project)
- Font override: `add_theme_font_override`
- Responsive layout: `AspectRatioContainer`, `SubViewportContainer`
- Focus management for gamepad/keyboard navigation (`grab_focus`, `focus_neighbor_*`)
- `TranslationServer` for localization
- Tween for UI animations (fade, slide)
- `z_index` vs `CanvasLayer` — when to use each
- Pitfall: `CanvasItem.clip_children` vs `ClipControl`

### godot:gameplay
**New content:**
- Input buffer pattern (queue recent inputs, replay on next valid frame)
- Coyote time + jump buffer implementation
- Push-down automaton state machine (stack-based for layered states)
- Screen shake via `Camera3D` transform noise
- Save slots: `DirAccess` + `ResourceSaver` pattern
- Delta-time normalization for framerate-independent movement
- `process_mode` matrix: PAUSABLE, WHEN_PAUSED, ALWAYS, DISABLED
- Action strength for analog input (`Input.get_action_strength`)

### godot:graphics
**New content:**
- SDFGI vs LightmapGI vs VoxelGI comparison table
- VolumetricFog setup (density, albedo, emission)
- Color grading via `Environment.adjustment_*`
- Shadow cascade configuration for `DirectionalLight3D`
- LOD via `VisibilityNotifier3D` and `GeometryInstance3D.lod_bias`
- Occlusion culling: `OccluderInstance3D` placement strategy
- HDR workflow: why tone mapping matters
- Draw call reduction: MultiMeshInstance3D for instancing

### godot:environment
**New content:**
- Large world: `WorldPartition` cell size and streaming distance
- `NavigationMesh` configuration: agent radius, max slope, cell size
- Terrain3D plugin workflow (community plugin)
- `TileMap` for 2D tile-based worlds
- Procedural placement via `MultiMeshInstance3D` + noise
- Occlusion strategy: manual vs automatic portal occlusion
- `Decal3D` for texture projection on surfaces
- Pitfall: CSG is prototype-only — never ship CSG geometry

### godot:animation
**New content:**
- Root motion full workflow: extract → apply to `CharacterBody3D.velocity`
- Procedural look-at with `Skeleton3D.look_at` / bone pose override
- `SpringBoneSimulator3D` for hair/cloth simulation
- `BoneAttachment3D` for attaching props to skeleton
- `AnimationMixer` (Godot 4.3+) vs `AnimationTree`
- `CallbackTrack` vs `MethodTrack` in AnimationPlayer
- Blend shape / morph target animation
- Pitfall: `advance_expression_base_node` in AnimationTree

### godot:audio
**New content:**
- Bus architecture blueprint: Master → Music / SFX → Voice / Ambience
- `AudioEffectCapture` for runtime audio analysis
- Polyphony: `AudioStreamPolyphonic` (Godot 4.3+)
- HRTF spatial audio: `AudioStreamPlayer3D.panning_strength`
- Procedural audio: `AudioStreamGenerator` + `AudioStreamGeneratorPlayback`
- Music crossfade pattern using two `AudioStreamPlayer` nodes
- `AudioServer.get_bus_peak_volume_left_db()` for VU meters
- Pitfall: `play()` resets position — use `stream_paused` to pause

### godot:vfx
**New content:**
- `Decal3D` for bullet holes, blood, scorch marks
- `Trail3D` (MeshTrail) ribbon effects
- Shader-based VFX: hit flash via `canvas_item` `modulate`, dissolve
- Particle LOD: disable `GPUParticles3D` beyond distance threshold
- `GPUParticlesCollision3D` for particles reacting to geometry
- Screen-space distortion shader pattern
- Pooling one-shot VFX: pre-instantiate and reuse via `restart()`
- Pitfall: `amount` × `lifetime` = memory cost, not just `amount`

### godot:networking
**New content:**
- Client-side prediction: apply input locally, reconcile with server state
- Lag compensation: server rewinds state N ms for hit detection
- Lobby pattern: `ENetMultiplayerPeer` host + `WebRTCMultiplayerPeer` for P2P
- `SceneReplicationConfig` property-level sync settings
- `MultiplayerSynchronizer` tick rate tuning
- Interest management: `set_visibility_for(peer_id, visible)`
- RPCMode matrix: `authority`, `any_peer`, `puppet` use cases
- Pitfall: `change_scene_to_file` on non-authority causes desync

### godot:ai
**New content:**
- Behavior tree in GDScript: `BTNode` base class, `Sequence`, `Selector`, `Leaf`
- Perception system: vision cone (dot product + raycast), sound radius (`Area3D`)
- Steering behaviors: seek, flee, arrive, wander
- Group coordination via `SceneTree.get_nodes_in_group()`
- `NavigationServer3D.map_get_path()` for offline path queries
- Influence maps for tactical AI
- `NavigationAgent3D.avoidance_priority` for crowd pushing
- Pitfall: setting `target_position` only once for a moving target

### godot:physics
**New content:**
- CCD (continuous collision detection): `RigidBody3D.continuous_cd`
- Collision layer design: Layer 1=World, 2=Player, 3=Enemy, 4=Projectile convention
- Custom physics: `_integrate_forces(state: PhysicsDirectBodyState3D)`
- `PhysicsServer3D` direct API for spawning thousands of bodies
- Swept collision via `PhysicsDirectSpaceState3D.cast_motion()`
- Sleep mode: `RigidBody3D.can_sleep` and wakeup conditions
- Soft body with `SoftBody3D`
- Pitfall: `move_and_collide` in `_process` instead of `_physics_process`

### godot:debugger
**New content:**
- Remote debugger connection: `--remote-debug` flag
- Profiler script: measure function execution time via `Time.get_ticks_usec()`
- Memory leak detection: `Performance.get_monitor(Performance.OBJECT_COUNT)`
- `assert()` driven development — crash early in debug builds
- `OS.dump_memory_to_file()` for memory snapshot
- Breakpoint injection via `breakpoint` keyword
- Error categories: script errors vs engine errors vs resource errors
- Pitfall: `print()` in `_physics_process` causes log flooding

### godot:scene
**New content:**
- Bulk node property setter via `get_children()` loop
- Load and add a scene via `load()` + `instantiate()`
- Set exported resource on a node via AgentBridge
- `EditorInterface.save_scene()` vs `--save-all`
- `EditorInspector.edit()` to select a node in the Inspector
- `EditorUndoRedoManager` for undoable operations
- Query node metadata: `node.get_meta_list()`
- Pitfall: `queue_free()` is deferred — check `is_queued_for_deletion()`

### godot:signals
**New content:**
- `connect(..., CONNECT_ONE_SHOT)` for fire-once listeners
- `connect(..., CONNECT_DEFERRED)` to defer to end-of-frame
- Signal chaining: `signal_a.connect(signal_b.emit)`
- Typed signals enforce argument contracts at editor time
- Disconnecting: `disconnect()` vs `CONNECT_ONE_SHOT`
- Performance: signal emission cost vs polling cost
- `is_connected()` guard before connecting to prevent duplicates
- Pitfall: connecting to a freed object crashes — use `is_instance_valid()`

### godot:runner
**New content:**
- GUT (Godot Unit Testing) integration: `gut` autoload, `test_*.gd` convention
- Headless CI run: `godot --headless --script test_runner.gd`
- Scene benchmark: run N frames, measure avg frame time
- `DisplayServer.window_set_mode(DisplayServer.WINDOW_MODE_MINIMIZED)` for background runs
- `SceneTree.quit()` with exit code for CI pass/fail
- `--export-debug` for profiled production builds
- Pitfall: `sleep` timing in runner is machine-dependent — use signal-based sync instead

---

## Execution Plan

1. Define template as above (this spec)
2. Dispatch all 18 skills in parallel to subagents — each agent:
   - Reads current SKILL.md
   - Rewrites using the template + per-skill plan above
   - Adds external references (docs.godotengine.org URLs, GDQuest links)
3. Review output for consistency and commit

---

## Quality Bar

- Every code example must be valid GDScript 4.x / Godot 4.7 syntax
- Every reference URL must point to docs.godotengine.org or a known stable resource
- No placeholder text ("TBD", "TODO") in final skills
- "Common Pitfalls" must have at least 3 entries per skill
- "Core Patterns" must have at least 2 code-backed examples per skill
