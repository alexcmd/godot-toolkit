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
| GDScript audio control | `godot:coder` |
| Signal connections | `godot:signals` |
| API lookup | `godot:docs` |
