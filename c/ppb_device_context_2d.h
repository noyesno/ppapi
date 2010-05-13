// Copyright (c) 2010 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef PPAPI_C_PPB_DEVICE_CONTEXT_2D_H_
#define PPAPI_C_PPB_DEVICE_CONTEXT_2D_H_

#include "ppapi/c/pp_resource.h"
#include "ppapi/c/pp_stdint.h"

typedef struct _pp_Module PP_Module;
typedef struct _pp_Rect PP_Rect;

// Callback function type for PaintImageData.
typedef void (*PPB_DeviceContext2D_FlushCallback)(PP_Resource context,
                                                  void* data);

#define PPB_DEVICECONTEXT2D_INTERFACE "PPB_DeviceContext2D;1"

typedef struct _ppb_DeviceContext2D {
  // The returned device context will not be bound to any plugin instance on
  // creation (call BindGraphicsDeviceContext on the plugin instance to do
  // that. The device context has a lifetime that can exceed that of the given
  // plugin instance.
  //
  // Set the is_always_opaque flag if you know that you will be painting only
  // opaque data to this device. This will disable blending when compositing
  // the plugin with the web page, which will give slightly higher performance.
  // If you aren't sure, it is always correct to specify that it it not opaque.
  PP_Resource (*Create)(PP_Module module, int32_t width, int32_t height,
                        bool is_always_opaque);

  // Enqueues a paint of the given image into the device. THIS HAS NO EFFECT
  // UNTIL YOU CALL Flush(). As a result, what counts is the contents of the
  // bitmap when you call Flush, not when you call this function.
  //
  // The given image will be placed at (x, y) from the top left of the device's
  // internal backing store. Then the src_rect will be copied into the
  // backing store.
  //
  // The src_rect is specified in the coordinate system of the image being
  // painted, not the device. For the common case of copying the entire image,
  // you may specify a NULL |src_rect| pointer. If you are frequently updating
  // the entire image, consider using SwapImageData which will give slightly
  // higher performance.
  //
  // Returns true on success, false on failure. Failure means one of the
  // devices was invalid, or the coordinates were out of bounds.
  bool (*PaintImageData)(PP_Resource device_context,
                         PP_Resource image,
                         int32_t x, int32_t y,
                         const PP_Rect* src_rect);

  // Enqueues a scroll of the device's backing store. THIS HAS NO EFFECT UNTIL
  // YOU CALL Flush(). The data within the given clip rect (you may specify
  // NULL to scroll the entire region) will be shifted by (dx, dy) pixels.
  //
  // This will result in some exposed region which will have undefined
  // contents. The plugin should call PaintImageData on these exposed regions
  // to give the correct contents.
  //
  // The scroll can be larger than the area of the clip rect, which means the
  // current image will be scrolled out of the rect. This is not an error but
  // will be a no-op.
  //
  // Returns true on success, false on failure. Failure means one of the
  // devices was invalid, or the clip rect went out of bounds of the device.
  bool (*Scroll)(PP_Resource device_context,
                 const PP_Rect* clip_rect,
                 int32_t dx, int32_t dy);

  // This function provides a slightly more efficient way to paint the entire
  // plugin's image. Normally, calling PaintImageData requires that the browser
  // copy the pixels out of the image and into the device context's backing
  // store. This function replaces the device context's backing store with the
  // given image, avoiding the copy.
  //
  // The new image must be the exact same size as this device context.
  //
  // THE NEW IMAGE WILL NOT BE PAINTED UNTIL YOU CALL FLUSH.
  //
  // After this call succeeds, the image in the input resource will be replaced
  // with an image of zero size that will be unusable for other calls.  This is
  // because the browser has taken ownership of the bitmap, and modifying it
  // from the plugin can cause strange painting artifacts. The object itself
  // will still be alive with the same reference count as before, so the plugin
  // should continue doing reference counting as normal.
  //
  // In the case of an animation, you will want to allocate a new image for the
  // next frame. It is best if you wait until the flush callback has executed
  // before allocating this bitmap. This gives the browser the option of
  // caching the previous backing store and handing it back to you (assuming
  // the sizes match). In the optimal case, this means no bitmaps are allocated
  // during the animation, and the backing store and "front buffer" (which the
  // plugin is painting into) are just being swapped back and forth.
  //
  // Returns true on success. Failure indicates the device context or image is
  // invalid, or the input image is a different size than the device context.
  // In the failure case, the image will still be valid.
  bool (*ReplaceContents)(PP_Resource device_context, PP_Resource image);

  // Flushes any enqueued paint, scroll, and swap commands for the backing
  // store. This actually executes the updates, and causes a repaint of the
  // webpage. This can run in two modes:
  //
  // - In synchronous mode, you specify NULL for the callback and the callback
  //   data. This function will block the calling thread until the image has
  //   been painted to the screen. It is not legal to block the main thread of
  //   the plugin, you can use synchronous mode only from background threads.
  //
  // - In asynchronous mode, you specify a callback function and the argument
  //   for that callback function. The callback function will be executed on
  //   the calling thread when the image has been painted to the screen.
  //
  // Because the callback is executed (or thread unblocked) only when the
  // plugin's current state is actually on the screen, this function provides a
  // way to rate limit animations. By waiting until the image is on the screen
  // before painting the next frame, you can ensure you're not generating
  // updates faster than the screen can be updated.
  //
  // Returns true on success or false on failure. Failure means the device
  // context is invalid or you are requesting a synchronous flush from the
  // main thread of the plugin. In this case, nothing will be updated.
  bool (*Flush)(PP_Resource device_context,
                PPB_DeviceContext2D_FlushCallback callback,
                void* callback_data);

} PPB_DeviceContext2D;

#endif  // PPAPI_C_PPB_DEVICE_CONTEXT_2D_H_
