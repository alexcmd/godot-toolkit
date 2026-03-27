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
