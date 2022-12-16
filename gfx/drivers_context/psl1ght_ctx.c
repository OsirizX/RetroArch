/*  RetroArch - A frontend for libretro.
 *  Copyright (C) 2010-2014 - Hans-Kristian Arntzen
 *  Copyright (C) 2011-2017 - Daniel De Matteis
 *
 *  RetroArch is free software: you can redistribute it and/or modify it under the terms
 *  of the GNU General Public License as published by the Free Software Found-
 *  ation, either version 3 of the License, or (at your option) any later version.
 *
 *  RetroArch is distributed in the hope that it will be useful, but WITHOUT ANY WARRANTY;
 *  without even the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
 *  PURPOSE.  See the GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License along with RetroArch.
 *  If not, see <http://www.gnu.org/licenses/>.
 */

#include <stdint.h>

#ifdef HAVE_CONFIG_H
#include "../../config.h"
#endif

#include <compat/strl.h>

#include "../../configuration.h"
#include "../../retroarch.h"
#include "../../verbosity.h"
#include <defines/ps3_defines.h>
#include "../../frontend/frontend_driver.h"
#include <rsx/rsx.h>

#if defined(HAVE_RSXGL)
#include "../common/gl_common.h"
#include "../common/gl2_common.h"
#if defined(HAVE_EGL)
#include "../common/egl_common.h"
#endif
#endif

typedef struct gfx_ctx_psl1ght_data
{
#if defined(HAVE_PSGL)
   PSGLdevice* gl_device;
   PSGLcontext* gl_context;
#elif defined(HAVE_EGL)
   egl_ctx_data_t egl;
   bool resize;
   unsigned width, height;
   float refresh_rate;
#else
   gcmContextData *rx_context;
#endif
} gfx_ctx_psl1ght_data_t;

/* TODO/FIXME - static global */
#if defined(HAVE_RSXGL)
static enum gfx_ctx_api ps3_api = GFX_CTX_OPENGL_API;
#else
static enum gfx_ctx_api ps3_api = GFX_CTX_RSX_API;
#endif

static void gfx_ctx_psl1ght_destroy(void *data);

static void gfx_ctx_psl1ght_get_resolution(unsigned idx,
      unsigned *width, unsigned *height)
{
   CellVideoOutResolution resolution;
   cellVideoOutGetResolution(idx, &resolution);

   *width  = resolution.width;
   *height = resolution.height;
}

static float gfx_ctx_psl1ght_get_aspect_ratio(void *data)
{
   CellVideoOutState videoState;

   cellVideoOutGetState(CELL_VIDEO_OUT_PRIMARY, 0, &videoState);

   switch (videoState.displayMode.aspect)
   {
      case CELL_VIDEO_OUT_ASPECT_4_3:
         return 4.0f/3.0f;
      case CELL_VIDEO_OUT_ASPECT_16_9:
         break;
   }

   return 16.0f/9.0f;
}

static void gfx_ctx_psl1ght_get_available_resolutions(void)
{
   unsigned i;
   uint32_t videomode[] = {
      CELL_VIDEO_OUT_RESOLUTION_480,
      CELL_VIDEO_OUT_RESOLUTION_576,
      CELL_VIDEO_OUT_RESOLUTION_960x1080,
      CELL_VIDEO_OUT_RESOLUTION_720,
      CELL_VIDEO_OUT_RESOLUTION_1280x1080,
      CELL_VIDEO_OUT_RESOLUTION_1440x1080,
      CELL_VIDEO_OUT_RESOLUTION_1600x1080,
      CELL_VIDEO_OUT_RESOLUTION_1080
   };
   uint32_t resolution_count = 0;
   bool defaultresolution    = true;
   uint16_t num_videomodes   = sizeof(videomode) / sizeof(uint32_t);
   global_t       *global    = global_get_ptr();

   if (global->console.screen.resolutions.check)
      return;

   for (i = 0; i < num_videomodes; i++)
   {
      if (cellVideoOutGetResolutionAvailability(
               CELL_VIDEO_OUT_PRIMARY, videomode[i],
               CELL_VIDEO_OUT_ASPECT_AUTO, 0))
         resolution_count++;
   }

   global->console.screen.resolutions.count = 0;
   global->console.screen.resolutions.list  =
      malloc(resolution_count * sizeof(uint32_t));

   for (i = 0; i < num_videomodes; i++)
   {
      if (cellVideoOutGetResolutionAvailability(
               CELL_VIDEO_OUT_PRIMARY,
               videomode[i],
               CELL_VIDEO_OUT_ASPECT_AUTO, 0))
      {
         global->console.screen.resolutions.list[
            global->console.screen.resolutions.count++] = videomode[i];
         global->console.screen.resolutions.initial.id = videomode[i];

         if (global->console.screen.resolutions.current.id == videomode[i])
         {
            defaultresolution = false;
            global->console.screen.resolutions.current.idx =
               global->console.screen.resolutions.count-1;
         }
      }
   }

   /* In case we didn't specify a resolution -
    * make the last resolution
      that was added to the list (the highest resolution)
      the default resolution */
   if (global->console.screen.resolutions.current.id > num_videomodes || defaultresolution)
    {
      global->console.screen.resolutions.current.idx = resolution_count - 1;
      global->console.screen.resolutions.current.id = global->console.screen.resolutions.list[global->console.screen.resolutions.current.idx];
    }

   global->console.screen.resolutions.check = true;
}

static void gfx_ctx_psl1ght_set_swap_interval(void *data, int interval)
{
#if defined(HAVE_PSGL)
   if (interval == 1)
      glEnable(GL_VSYNC_SCE);
   else
      glDisable(GL_VSYNC_SCE);
#elif defined(HAVE_EGL)
   gfx_ctx_psl1ght_data_t *psl1ght = (gfx_ctx_psl1ght_data_t*)data;
   egl_set_swap_interval(&psl1ght->egl, 0);
#endif
}

static void gfx_ctx_psl1ght_check_window(void *data, bool *quit,
      bool *resize, unsigned *width, unsigned *height)
{
   
}

static bool gfx_ctx_psl1ght_has_focus(void *data) { return true; }
static bool gfx_ctx_psl1ght_suppress_screensaver(void *data, bool enable) { return false; }

static void gfx_ctx_psl1ght_swap_buffers(void *data)
{
#ifdef HAVE_PSGL
   psglSwap();
#elif defined(HAVE_EGL)
   gfx_ctx_psl1ght_data_t *psl1ght = (gfx_ctx_psl1ght_data_t *)data;
   egl_swap_buffers(&psl1ght->egl);
#endif
#ifdef HAVE_SYSUTILS
   cellSysutilCheckCallback();
#endif
}

static void gfx_ctx_psl1ght_get_video_size(void *data,
      unsigned *width, unsigned *height)
{
#if defined(HAVE_PSGL) || defined(HAVE_EGL)
   gfx_ctx_psl1ght_data_t *psl1ght = (gfx_ctx_psl1ght_data_t*)data;
   if (psl1ght)
#if defined(HAVE_PSGL)
      psglGetDeviceDimensions(psl1ght->gl_device, width, height);
#elif defined(HAVE_EGL)
      egl_get_video_size(&psl1ght->egl, width, height);
#endif
#endif
}

static void *gfx_ctx_psl1ght_init(void *video_driver)
{
   global_t *global = global_get_ptr();
   gfx_ctx_psl1ght_data_t *psl1ght = (gfx_ctx_psl1ght_data_t*)
      calloc(1, sizeof(gfx_ctx_psl1ght_data_t));

   if (!psl1ght)
      return NULL;

#if defined(HAVE_EGL)
   EGLint n;
   EGLint major, minor;
   static const EGLint attribs[] = {
#if 1
      EGL_CONFIG_ID, 1,
#else
      EGL_BUFFER_SIZE, 32,
      EGL_RED_SIZE, 8,
      EGL_BLUE_SIZE, 8,
      EGL_GREEN_SIZE, 8,
      EGL_ALPHA_SIZE, 8,
      EGL_DEPTH_SIZE, 24,
      EGL_STENCIL_SIZE, 8,
#endif
      EGL_NONE
   };

   if (!egl_init_context(&psl1ght->egl, EGL_NONE, EGL_DEFAULT_DISPLAY,
                         &major, &minor, &n, attribs, NULL))
   {
      egl_report_error();
      printf("[PSL1GHT Context]: EGL error: 0x%x.\n", eglGetError());
      return NULL;
   }

#endif

   global->console.screen.pal_enable =
      cellVideoOutGetResolutionAvailability(
            CELL_VIDEO_OUT_PRIMARY, CELL_VIDEO_OUT_RESOLUTION_576,
            CELL_VIDEO_OUT_ASPECT_AUTO, 0);

   gfx_ctx_psl1ght_get_available_resolutions();

   return psl1ght;
}

static bool gfx_ctx_psl1ght_set_video_mode(void *data,
      unsigned width, unsigned height,
      bool fullscreen)
{
#if defined(HAVE_EGL)
   static const EGLint contextAttributeList[] =
   {
      EGL_CONTEXT_CLIENT_VERSION, 2,
      EGL_NONE
   };

   gfx_ctx_psl1ght_data_t *psl1ght = (gfx_ctx_psl1ght_data_t *)data;

   psl1ght->refresh_rate = 60;

   if (!egl_create_context(&psl1ght->egl, 0)) {
      egl_report_error();
      gfx_ctx_psl1ght_destroy(data);
      return false;
   }

   if (!egl_create_surface(&psl1ght->egl, 0)) {
      egl_report_error();
      gfx_ctx_psl1ght_destroy(data);
      return false;
   }
#endif
  return true;
}

static void gfx_ctx_psl1ght_destroy_resources(gfx_ctx_psl1ght_data_t *ps3)
{
   if (!ps3)
      return;

#if defined(HAVE_PSGL)
   psglDestroyContext(ps3->gl_context);
   psglDestroyDevice(ps3->gl_device);

   psglExit();
#elif defined(HAVE_EGL)
   egl_destroy(&ps3->egl);
#endif
}

static void gfx_ctx_psl1ght_destroy(void *data)
{
   gfx_ctx_psl1ght_data_t *ps3 = (gfx_ctx_psl1ght_data_t*)data;

   if (!ps3)
      return;

   gfx_ctx_psl1ght_destroy_resources(ps3);
   free(data);
}

static void gfx_ctx_psl1ght_input_driver(void *data,
      const char *joypad_name,
      input_driver_t **input, void **input_data)
{
   void *ps3input       = input_driver_init_wrap(&input_ps3, joypad_name);

   *input               = ps3input ? &input_ps3 : NULL;
   *input_data          = ps3input;
}

#if defined(HAVE_EGL)
static void gfx_ctx_psl1ght_bind_hw_render(void *data, bool enable)
{
   gfx_ctx_psl1ght_data_t *psl1ght = (gfx_ctx_psl1ght_data_t *)data;
   egl_bind_hw_render(&psl1ght->egl, enable);
}
#endif

static enum gfx_ctx_api gfx_ctx_psl1ght_get_api(void *data) { return ps3_api; }

static bool gfx_ctx_psl1ght_bind_api(void *data,
      enum gfx_ctx_api api, unsigned major, unsigned minor)
{
   ps3_api = api;

#if defined(HAVE_RSXGL)
   if (
      api == GFX_CTX_OPENGL_API ||
      api == GFX_CTX_OPENGL_ES_API
   )
      return true;
#else
   if (api == GFX_CTX_RSX_API)
      return true;
#endif

   return false;
}

static void gfx_ctx_psl1ght_get_video_output_size(void *data,
      unsigned *width, unsigned *height, char *desc, size_t desc_len)
{
   global_t *global = global_get_ptr();

   if (!global)
      return;

   gfx_ctx_psl1ght_get_resolution(global->console.screen.resolutions.current.id,
         width, height);

   if (*width == 720 && *height == 576)
   {
      if (global->console.screen.pal_enable)
         global->console.screen.pal60_enable = true;
   }
   else
   {
      global->console.screen.pal_enable = false;
      global->console.screen.pal60_enable = false;
   }
}

static void gfx_ctx_psl1ght_get_video_output_prev(void *data)
{
   global_t *global = global_get_ptr();

   if (!global)
      return;

   if (global->console.screen.resolutions.current.idx)
   {
      global->console.screen.resolutions.current.idx--;
      global->console.screen.resolutions.current.id =
         global->console.screen.resolutions.list
         [global->console.screen.resolutions.current.idx];
   }
}

static void gfx_ctx_psl1ght_get_video_output_next(void *data)
{
   global_t *global = global_get_ptr();

   if (!global)
      return;

   if (global->console.screen.resolutions.current.idx + 1 <
         global->console.screen.resolutions.count)
   {
      global->console.screen.resolutions.current.idx++;
      global->console.screen.resolutions.current.id =
         global->console.screen.resolutions.list
         [global->console.screen.resolutions.current.idx];
   }
}

static uint32_t gfx_ctx_psl1ght_get_flags(void *data)
{
   uint32_t flags = 0;

#ifdef HAVE_CG
   BIT32_SET(flags, GFX_CTX_FLAGS_SHADERS_CG);
#endif

#if defined(HAVE_EGL)
   if (string_is_equal(video_driver_get_ident(), "glcore"))
   {
#if defined(HAVE_SLANG) && defined(HAVE_SPIRV_CROSS)
      BIT32_SET(flags, GFX_CTX_FLAGS_SHADERS_SLANG);
#endif
   }
   else
   {
      BIT32_SET(flags, GFX_CTX_FLAGS_SHADERS_GLSL);
   }
#endif

   return flags;
}

static void gfx_ctx_psl1ght_set_flags(void *data, uint32_t flags) { }

#if defined(HAVE_EGL)
static float gfx_ctx_psl1ght_get_refresh_rate(void *data)
{
   gfx_ctx_psl1ght_data_t *psl1ght = (gfx_ctx_psl1ght_data_t *)data;
   return psl1ght->refresh_rate;
}
#endif

const gfx_ctx_driver_t gfx_ctx_psl1ght = {
   gfx_ctx_psl1ght_init,
   gfx_ctx_psl1ght_destroy,
   gfx_ctx_psl1ght_get_api,
   gfx_ctx_psl1ght_bind_api,
   gfx_ctx_psl1ght_set_swap_interval,
   gfx_ctx_psl1ght_set_video_mode,
   gfx_ctx_psl1ght_get_video_size,
#if defined(HAVE_EGL)
   gfx_ctx_psl1ght_get_refresh_rate,
#else
   NULL, /* get_refresh_rate */
#endif
   gfx_ctx_psl1ght_get_video_output_size,
   gfx_ctx_psl1ght_get_video_output_prev,
   gfx_ctx_psl1ght_get_video_output_next,
   NULL, /* get_metrics */
   NULL,
   NULL, /* update_title */
   gfx_ctx_psl1ght_check_window,
   NULL, /* set_resize */
   gfx_ctx_psl1ght_has_focus,
   gfx_ctx_psl1ght_suppress_screensaver,
   false, /* has_windowed */
   gfx_ctx_psl1ght_swap_buffers,
   gfx_ctx_psl1ght_input_driver,
   NULL,
   NULL,
   NULL,
   NULL,
   "psl1ght",
   gfx_ctx_psl1ght_get_flags,
   gfx_ctx_psl1ght_set_flags,
#if defined(HAVE_EGL)
   gfx_ctx_psl1ght_bind_hw_render,
#else
   NULL,
#endif
   NULL
};
