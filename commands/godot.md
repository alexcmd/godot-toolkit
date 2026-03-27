---
name: godot
description: Load the Godot development context — use at the start of a session or when switching to Godot work mid-session.
---

# Godot Development Context

You are working on a **Godot 4.7** project with the **AgentBridge** module active.

## AgentBridge REST Tools

| Tool | Purpose |
|---|---|
| `execute(code)` | Run GDScript in the live editor |
| `get_log(max_lines)` | Read editor output log |
| `save_all()` | Save all open scenes and scripts |
| `play_scene(scene)` | Start playing a scene |
| `stop_scene()` | Stop the playing scene |

## Domain Skills — When to Use Each

| Skill | Use when |
|---|---|
| `godot:coder` | Writing any GDScript — classes, autoloads, methods |
| `godot:architect` | Designing scene hierarchy, autoload structure, resource patterns |
| `godot:debugger` | Encountering errors, crashes, or unexpected behavior |
| `godot:runner` | Running, testing, or profiling scenes |
| `godot:scene` | Adding, removing, or inspecting nodes programmatically |
| `godot:shader` | Writing spatial, canvas_item, or particle shaders |
| `godot:signals` | Designing event systems or connecting signals |
| `godot:ui` | Building Control-based UI, HUDs, menus |
| `godot:docs` | Looking up Godot 4.x API docs via Context7 |

## Process Skills — When to Use Each

| Skill | Use when |
|---|---|
| `godot:brainstorming` | Before implementing any new feature |
| `godot:writing-plans` | After brainstorming, before writing code |
| `godot:executing-plans` | Executing a written implementation plan |
| `godot:subagent-driven-development` | Independent subtasks suitable for parallel agents |
| `godot:systematic-debugging` | Any debugging — always use instead of guessing |
| `godot:verification-before-completion` | Before claiming any work is done |
| `godot:requesting-code-review` | After completing a plan step |
| `godot:receiving-code-review` | Processing code review feedback |
| `godot:dispatching-parallel-agents` | Launching parallel agents for independent work |
| `godot:using-git-worktrees` | Starting feature work that needs branch isolation |
| `godot:finishing-a-development-branch` | When implementation is complete and ready to integrate |
| `godot:writing-skills` | Creating new godot:* skills |

## Role Skills — When to Use Each

| Skill | Use when |
|---|---|
| `godot:gameplay` | Implementing player, input, game mechanics, state machines, save/load |
| `godot:graphics` | Setting up lighting, WorldEnvironment, post-processing, GI, reflection probes |
| `godot:environment` | Building game worlds — terrain, navigation mesh, occlusion, GridMap |
| `godot:vfx` | Creating particle effects, trails, impacts, screen effects |
| `godot:animation` | Working with AnimationPlayer, AnimationTree, blend spaces, IK |
| `godot:audio` | Setting up sounds, music, spatial audio, audio buses |
| `godot:physics` | Collision layers, joints, raycasts, physics bodies, PhysicsServer3D |
| `godot:ai` | NPC pathfinding, NavigationAgent3D, NPC behavior state machines |
| `godot:networking` | Multiplayer, RPC, scene replication, lobby systems |
