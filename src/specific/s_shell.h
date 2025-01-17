#pragma once

#include <stdint.h>
#include <stdbool.h>

void S_Shell_ShowFatalError(const char *message);

void S_Shell_Shutdown();
void S_Shell_SeedRandom();
void S_Shell_SpinMessageLoop();
bool S_Shell_GetCommandLine(int *arg_count, char ***args);
void *S_Shell_GetWindowHandle();
void S_Shell_TerminateGame(int exit_code);
void S_Shell_ToggleFullscreen();
int S_Shell_GetCurrentDisplayWidth();
int S_Shell_GetCurrentDisplayHeight();
