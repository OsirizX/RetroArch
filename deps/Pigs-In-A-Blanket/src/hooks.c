/*****************************************************************************
 * 
 *  Copyright (c) 2020 by SonicMastr <sonicmastr@gmail.com>
 * 
 *  This file is part of Pigs In A Blanket
 * 
 *  This program is free software: you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation, either version 3 of the License, or
 *  (at your option) any later version.
 * 
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 * 
 *  You should have received a copy of the GNU General Public License
 *  along with this program.  If not, see <https://www.gnu.org/licenses/>.
 * 
 ****************************************************************************/

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <psp2/types.h>
#include "../include/shacccgpatch.h"
#include "../include/hooks.h"
#include "../include/patches.h"
#include "../include/debug.h"

tai_hook_ref_t hookRef[NUM_HOOKS];
SceUID hook[NUM_HOOKS];
int customResolutionMode;
int pibHooksInitialized = 0;

void loadHooks(PibOptions options)
{
    tai_module_info_t info;

    info.size = sizeof(info);
    taiGetModuleInfo("libScePiglet", &info);
    if (options & PIB_SHACCCG) {
        hook[0] = taiHookFunctionOffset(&hookRef[0], info.modid, 0, 0x32BB4, 1, pglPlatformShaderCompiler_CustomPatch);
        hook[1] = taiHookFunctionExport(&hookRef[1], info.name, 0xB4FE1ABB, 0x919FBCB7, glGetBooleanv_shaderCompilerPatch);
        LOG("ShaccCg Patch: 0x%08x\nEnabled Shader Compiler Response: 0x%08x\n", hook[0], hook[1]);
    }
    hook[2] = taiHookFunctionOffset(&hookRef[2], info.modid, 0, 0x39770, 1, _pglPlatformTextureUploadParams_patch);
    hook[3] = taiHookFunctionExport(&hookRef[3], info.name, 0xB4FE1ABB, 0x4B86317A, eglCreateWindowSurface_resolutionPatch);
    LOG("Texture Upload Params Patch: 0x%08x\n", hook[2]);
    LOG("Resolution Patch: 0x%08x\n", hook[3]);
    if (options & PIB_GET_PROC_ADDR_CORE) {
        hook[4] = taiHookFunctionExport(&hookRef[4], info.name, 0xB4FE1ABB, 0x249A431A, eglGetProcAddress_functionNamePatch);
        LOG("eglGetProcAddress Function Name Patch: 0x%08x\n", hook[4]);
    }
    pibHooksInitialized = 1;
}

void releaseHooks(void)
{
    if (pibHooksInitialized) {
        for (int i = 0; i < NUM_HOOKS; i++) {
            taiHookRelease(hook[i], hookRef[i]);
        }
        pibHooksInitialized = 0;
    }
}