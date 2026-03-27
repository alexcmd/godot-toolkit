@echo off
dir /s /b /a-d project.godot 2>nul | findstr /i "project.godot" >nul
if %ERRORLEVEL% EQU 0 (
    type "%CLAUDE_PLUGIN_ROOT%\skills\godot\SKILL.md"
)
