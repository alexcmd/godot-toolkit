---
name: ui
description: Build Godot UI with Control nodes, themes, and anchors — use when creating menus, HUDs, or any CanvasLayer UI.
type: implementation
---

# Godot UI

## Key Principles

- Use `Control` as base for all UI. `CanvasLayer` for HUDs that overlay the 3D world.
- Set anchors via `set_anchor_and_offset` or the Layout menu in editor.
- Use `Theme` resources to style multiple controls at once — never style individual controls in code.
- `MarginContainer` + `VBoxContainer`/`HBoxContainer` for most layouts.
- `%NodeName` unique names for controls you reference in scripts.

## Common Patterns

**Anchoring a panel to bottom-right:**
```gdscript
var panel: Panel = $Panel
panel.set_anchor_and_offset(SIDE_LEFT, 1.0, -200)
panel.set_anchor_and_offset(SIDE_TOP, 1.0, -100)
panel.set_anchor_and_offset(SIDE_RIGHT, 1.0, 0)
panel.set_anchor_and_offset(SIDE_BOTTOM, 1.0, 0)
```

**Connecting a Button signal:**
```gdscript
%MyButton.pressed.connect(_on_my_button_pressed)
```

**Showing/hiding UI:**
```gdscript
%HUD.visible = true
%PauseMenu.visible = false
```

**Theme application:**
```gdscript
var theme = preload("res://resources/ui_theme.tres")
%MyPanel.theme = theme
```

## CanvasLayer for HUD

```gdscript
# hud.gd
extends CanvasLayer

@onready var health_label: Label = %HealthLabel

func update_health(value: int) -> void:
    health_label.text = "HP: %d" % value
```

## Docs

When Control node properties or theme overrides are uncertain, use godot:docs (Context7) before writing code.
