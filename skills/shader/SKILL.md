---
name: shader
description: Write Godot Shading Language shaders — use when creating spatial, canvas_item, or particle shaders.
type: implementation
---

# Godot Shader

## Shader Types

- `shader_type spatial;` — 3D objects
- `shader_type canvas_item;` — 2D sprites, UI
- `shader_type particles;` — GPU particles

## Spatial Shader Template

```glsl
shader_type spatial;

uniform vec4 albedo : source_color = vec4(1.0);
uniform float roughness : hint_range(0.0, 1.0) = 0.5;

void fragment() {
    ALBEDO = albedo.rgb;
    ROUGHNESS = roughness;
}
```

## Canvas Item Template

```glsl
shader_type canvas_item;

uniform float speed : hint_range(0.0, 10.0) = 1.0;

void fragment() {
    vec2 uv = UV;
    uv.x += TIME * speed * 0.1;
    COLOR = texture(TEXTURE, uv);
}
```

## Particle Shader Template

```glsl
shader_type particles;

void process() {
    VELOCITY.y -= 9.8 * DELTA;
    TRANSFORM.origin += VELOCITY * DELTA;
}
```

## Uniforms

- `hint_range(min, max)` — slider in inspector
- `source_color` — color picker with sRGB
- `hint_default_white` / `hint_default_black` — texture defaults

## Docs

When GLSL built-ins or Godot shader globals are uncertain, use godot:docs (Context7) before writing code.
