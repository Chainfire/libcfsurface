/*
 * Copyright (C) 2007 The Android Open Source Project
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *      http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#ifndef ANDROID_GUI_ISURFACE_COMPOSER_CLIENT_H
#define ANDROID_GUI_ISURFACE_COMPOSER_CLIENT_H

#include <stdint.h>
#include <sys/types.h>

#include <utils/Errors.h>
#include <utils/RefBase.h>

#include <binder/IInterface.h>

#include <ui/FrameStats.h>
#include <ui/PixelFormat.h>

namespace android {
// ----------------------------------------------------------------------------

class IGraphicBufferProducer;

class ISurfaceComposerClient : public IInterface
{
public:
    DECLARE_META_INTERFACE(SurfaceComposerClient);

    // flags for createSurface()
    enum { // (keep in sync with Surface.java)
        eHidden             = 0x00000004,
        eDestroyBackbuffer  = 0x00000020,
        eSecure             = 0x00000080,
        eNonPremultiplied   = 0x00000100,
        eOpaque             = 0x00000400,
        eProtectedByApp     = 0x00000800,
        eProtectedByDRM     = 0x00001000,
        eCursorWindow       = 0x00002000,

        eFXSurfaceNormal    = 0x00000000,
        eFXSurfaceDim       = 0x00020000,
        eFXSurfaceMask      = 0x000F0000,
    };

    /*
     * Requires ACCESS_SURFACE_FLINGER permission
     */
    virtual status_t createSurface(
            const String8& name, uint32_t w, uint32_t h,
            PixelFormat format, uint32_t flags,
            sp<IBinder>* handle,
            sp<IGraphicBufferProducer>* gbp) = 0;

    /*
     * Requires ACCESS_SURFACE_FLINGER permission
     */
    virtual status_t destroySurface(const sp<IBinder>& handle) = 0;

    /*
     * Requires ACCESS_SURFACE_FLINGER permission
     */
    virtual status_t clearLayerFrameStats(const sp<IBinder>& handle) const = 0;

    /*
     * Requires ACCESS_SURFACE_FLINGER permission
     */
    virtual status_t getLayerFrameStats(const sp<IBinder>& handle, FrameStats* outStats) const = 0;
};

// ----------------------------------------------------------------------------

class BnSurfaceComposerClient: public BnInterface<ISurfaceComposerClient> {
public:
    virtual status_t onTransact(uint32_t code, const Parcel& data,
            Parcel* reply, uint32_t flags = 0);
};

// ----------------------------------------------------------------------------

}; // namespace android

#endif // ANDROID_GUI_ISURFACE_COMPOSER_CLIENT_H
