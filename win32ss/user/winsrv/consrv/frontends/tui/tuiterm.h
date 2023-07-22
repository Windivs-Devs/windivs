/*
 * COPYRIGHT:       See COPYING in the top level directory
 * PROJECT:         Windivs Console Server DLL
 * FILE:            win32ss/user/winsrv/consrv/frontends/tui/tuiterm.h
 * PURPOSE:         TUI Terminal Front-End
 * PROGRAMMERS:     David Welch
 *                  G� van Geldorp
 *                  Jeffrey Morlan
 *                  Hermes Belusca-Maito (hermes.belusca@sfr.fr)
 */

#pragma once

NTSTATUS TuiInitConsole(PCONSOLE Console,
                        /*IN*/ PCONSOLE_START_INFO ConsoleStartInfo,
                        PCONSOLE_INFO ConsoleInfo,
                        DWORD ProcessId);

/* EOF */
