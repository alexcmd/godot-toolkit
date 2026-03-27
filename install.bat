@echo off
setlocal

set GODOT_MODULES=D:\Projects\godot\modules\agent_bridge
set TOOLKIT_DIR=%~dp0AgentBridge
if not exist "%TOOLKIT_DIR%" (
    echo ERROR: AgentBridge source not found: %TOOLKIT_DIR%
    exit /b 1
)

echo [AgentBridge] Linking module...
if exist "%GODOT_MODULES%" (
    rmdir "%GODOT_MODULES%"
    if %ERRORLEVEL% NEQ 0 (
        echo ERROR: "%GODOT_MODULES%" exists and is not a junction. Remove it manually first.
        exit /b 1
    )
)
mklink /J "%GODOT_MODULES%" "%TOOLKIT_DIR%"
if %ERRORLEVEL% NEQ 0 (
    echo ERROR: Failed to create junction. Run as Administrator.
    exit /b 1
)
echo [AgentBridge] Module linked: %GODOT_MODULES%

echo.
echo [godot plugin] Installing Claude Code plugin...
claude plugin install "%~dp0" --scope user
if %ERRORLEVEL% NEQ 0 (
    echo WARNING: Plugin install failed. Run manually:
    echo   claude plugin install %~dp0 --scope user
) else (
    echo [godot plugin] Plugin installed (user scope).
)

echo.
echo [AgentBridge + godot plugin] Done. Next steps:
echo   1. Rebuild Godot: cd D:\Projects\godot ^&^& scons platform=windows target=editor -j32
echo   2. Open demo-host: D:\GodotProjects\demo-host
echo   3. Verify: claude plugin list

endlocal
