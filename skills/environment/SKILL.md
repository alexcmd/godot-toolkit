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

- Call `bake_navigation_mesh()` after every geometry change — not just at scene load
- Pair every large static mesh with an `OccluderInstance3D`; do not rely on engine auto-occlusion
- `GridMap` cell size must match the collision shape size exactly

## Delegate to

| Task | Skill |
|---|---|
| Scene/node manipulation | `godot:scene` |
| Architecture decisions | `godot:architect` |
| API lookup | `godot:docs` |
