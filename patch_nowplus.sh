#!/bin/bash
patch -p1 -d ./external/bluetooth/bluez  <./device/samsung/nowplus/patch/external_bluetooth_bluez.diff
patch -p1 -d ./system/core/  <./device/samsung/nowplus/patch/system_core.diff
patch -p1 -d ./frameworks/base/  <./device/samsung/nowplus/patch/frameworks_base_media_libstagefright.diff
patch -p1 -d ./packages/apps/Gallery3D  <./device/samsung/nowplus/patch/packages_apps_Gallery3D.diff