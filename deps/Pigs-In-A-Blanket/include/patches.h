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

#ifndef PATCHES_H_
#define PATCHES_H_

#include <psp2/gxm.h>

void glGetBooleanv_shaderCompilerPatch(unsigned int pname, unsigned char *data);
void _pglPlatformTextureUploadParams_patch(int textureUploadParams);
int eglCreateWindowSurface_resolutionPatch(int dpy, int config, int win, int *attrib_list);
void *eglGetProcAddress_functionNamePatch(const char *procname);
#endif /* PATCHES_H_ */