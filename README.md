# Godot Plugin for Claude Code

A Claude Code plugin that turns Claude into a Godot 4.7 co-developer. It knows the engine, speaks GDScript, and can actually run code in your live editor through AgentBridge.

---

## What it does

When you open a Godot project, Claude automatically loads Godot context — the right nodes, APIs, patterns, and rules for whatever you're working on. You don't have to explain what `CharacterBody3D` is or remind Claude that physics logic belongs in `_physics_process`. It already knows.

The plugin also gives Claude a live connection to your running editor through **AgentBridge**. It can execute GDScript, read the output log, save scenes, and start or stop playback — all without you leaving your editor.

---

## Requirements

- Claude Code
- Godot 4.7 with the [AgentBridge](https://github.com/alexcmd/AgentBridge) module installed *(required for live editor tools)*

---

## Installation

```bash
claude plugin install godot@claude-plugins-official
```

Or clone and install locally:

```bash
git clone https://github.com/alexcmd/godot-toolkit
cd godot-toolkit
./install.sh   # Mac/Linux
install.bat    # Windows
```

Once installed, open any Godot project — Claude will pick up the context automatically on session start.

---

## Skills

The plugin organizes its skills into three groups.

### Domain skills — the core toolkit

These are the workhorses. Use them whenever you need to write code, design systems, or inspect the editor.

| Skill | What it's for |
|---|---|
| `godot:coder` | Writing GDScript — classes, autoloads, methods |
| `godot:architect` | Scene hierarchy, autoload structure, resource patterns |
| `godot:scene` | Adding, moving, or inspecting nodes via the live editor |
| `godot:shader` | Spatial, canvas_item, and particle shaders |
| `godot:signals` | Signal architecture and connections |
| `godot:ui` | Control nodes, themes, anchors, HUDs |
| `godot:debugger` | Errors, crashes, and unexpected behavior |
| `godot:runner` | Running and profiling scenes |
| `godot:docs` | Looking up Godot 4.x API docs via Context7 |

### Role skills — put Claude in the right headspace

Sometimes you need Claude thinking like a specific kind of developer. These skills set the context for a domain and point to the right tools for that work.

| Skill | When to use it |
|---|---|
| `godot:gameplay` | Player controllers, input, state machines, save/load |
| `godot:graphics` | Lighting, WorldEnvironment, GI, post-processing |
| `godot:environment` | Terrain, navigation mesh, occlusion, GridMap |
| `godot:vfx` | Particles, trails, impacts, screen effects |
| `godot:animation` | AnimationPlayer, AnimationTree, blend spaces, IK |
| `godot:audio` | Spatial audio, buses, randomized one-shots |
| `godot:physics` | Collision layers, joints, raycasts, physics bodies |
| `godot:ai` | NPC pathfinding, behavior state machines, detection |
| `godot:networking` | Multiplayer, RPC, scene replication |

### Process skills — how the work gets done

These enforce good habits: plan before coding, verify before calling anything done, review before moving on.

| Skill | When to use it |
|---|---|
| `godot:brainstorming` | Before starting any new feature |
| `godot:writing-plans` | After brainstorming, before writing code |
| `godot:executing-plans` | Working through a written plan step by step |
| `godot:subagent-driven-development` | Parallel work with independent subagents |
| `godot:systematic-debugging` | Any bug — always use this instead of guessing |
| `godot:verification-before-completion` | Before saying anything is done |
| `godot:requesting-code-review` | After finishing a plan step |
| `godot:receiving-code-review` | Processing review feedback |
| `godot:finishing-a-development-branch` | When implementation is complete |

---

## The code reviewer

The plugin includes a `code-reviewer` agent that knows Godot — not just generic code quality. It checks GDScript idioms (typed variables, `@onready`, signal connection style), scene structure, AgentBridge usage, performance patterns, and role-specific rules for each domain.

It gets invoked automatically as part of the workflow after you finish a plan step.

---

## AgentBridge

AgentBridge is a Godot module that exposes your running editor as a REST API. With it active, Claude can:

- Run GDScript in the live editor and read the result
- Check the output log for errors
- Save all open scenes and scripts
- Start and stop scene playback

This makes the difference between Claude guessing at your project and Claude actually seeing it. Install it from the [AgentBridge repo](https://github.com/alexcmd/AgentBridge) and follow the setup instructions there.

---

## How it works in practice

Open a Godot project and start Claude Code. The plugin detects `project.godot` and loads automatically — you'll have Godot context from your first message.

For anything non-trivial, the workflow goes: brainstorm the feature → write a plan → execute it step by step → verify → review → commit. The process skills handle each of those stages and make sure Claude doesn't skip steps or declare victory prematurely.

For quick tasks — fixing a bug, writing a shader, wiring up signals — just use the relevant skill directly.

---

## License

MIT
