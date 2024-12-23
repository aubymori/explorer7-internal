#pragma once
#include "common.h"
#include "dbgprint.h"
#include "OptionConfig.h"
#include "OSVersion.h"

// Ittr: Pattern imports are defined and rewritten here rather than dllmain. Feel free to further improve this.

// Find-by-string - allow 7 msstyles to load by removing animation map data
void RemoveLoadAnimationDataMap();
void RemoveGetClassIdForShellTarget();

// Fix authui.dll import for CLogOffOptions by replacing bytes
void FixAuthUI();

// Remove unwanted immersive shell interfaces, often by preventing them from running
void DisableImmersiveStart();
void DisableImmersiveSearch();
void DisableTaskView();
void DisableWin11AltTab();
void FixWin11SearchIcon();

// Main procedure we call from elsewhere
void ChangePatternImports();