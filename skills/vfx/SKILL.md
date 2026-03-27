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

- One-shot effects must set `one_shot = true` and call `restart()` to re-trigger — never re-instantiate the scene per trigger
- Sub-emitters trigger via `emit_particle()` signal — never spawn new scenes
- VFX scenes are standalone (instantiated into the scene, not embedded)

## Delegate to

| Task | Skill |
|---|---|
| Particle/visual shaders | `godot:shader` |
| Scene instantiation | `godot:scene` |
| GDScript particle control | `godot:coder` |
| API lookup | `godot:docs` |
