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
| GDScript configuration | `godot:coder` |
| API lookup | `godot:docs` |
