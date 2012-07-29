/*
 * Copyright (C) 2008 The Android Open Source Project
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

#include <fcntl.h>
#include <errno.h>
#include <math.h>
#include <poll.h>
#include <unistd.h>
#include <dirent.h>
#include <sys/select.h>

#include <cutils/log.h>

#include "AccelerationSensor.h"

/*****************************************************************************/
/*magic no*/
#define KXSD9_IOC_MAGIC  0xF6
/*max seq no*/
#define KXSD9_IOC_NR_MAX 12 

#define KXSD9_IOC_GET_ACC           _IOR(KXSD9_IOC_MAGIC, 0, KXSD9_acc_t)
#define KXSD9_IOC_DISB_MWUP         _IO(KXSD9_IOC_MAGIC, 1)
#define KXSD9_IOC_ENB_MWUP          _IOW(KXSD9_IOC_MAGIC, 2, unsigned short)
#define KXSD9_IOC_MWUP_WAIT         _IO(KXSD9_IOC_MAGIC, 3)
#define KXSD9_IOC_GET_SENSITIVITY   _IOR(KXSD9_IOC_MAGIC, 4, KXSD9_sensitivity_t)
#define KXSD9_IOC_GET_ZERO_G_OFFSET _IOR(KXSD9_IOC_MAGIC, 5, KXSD9_zero_g_offset_t )  
#define KXSD9_IOC_GET_DEFAULT       _IOR(KXSD9_IOC_MAGIC, 6, KXSD9_acc_t)
#define KXSD9_IOC_SET_RANGE         _IOWR( KXSD9_IOC_MAGIC, 7, int )
#define KXSD9_IOC_SET_BANDWIDTH     _IOWR( KXSD9_IOC_MAGIC, 8, int )
#define KXSD9_IOC_SET_MODE          _IOWR( KXSD9_IOC_MAGIC, 9, int )
#define KXSD9_IOC_SET_MODE_AKMD2    _IOWR( KXSD9_IOC_MAGIC, 10, int )
#define KXSD9_IOC_GET_INITIAL_VALUE _IOWR( KXSD9_IOC_MAGIC, 11, kxsd9_convert_t )
#define KXSD9_IOC_SET_DELAY         _IOWR( KXSD9_IOC_MAGIC, 12, int )

#define ACCEL       0
#define COMPASS     1

#define STANDBY     0
#define NORMAL      1

#define STANDBY         0
#define ONLYACCEL       1
#define ONLYCOMPASS     2
#define ACCELCOMPASS    3
/*****************************************************************************/

AccelerationSensor::AccelerationSensor()
    : SensorBase(ACCELEROMETER_DEVICE_NAME, "accel"),
      mEnabled(0),
      mOrientationEnabled(0),
      mInputReader(32)
{
    mPendingEvent.version = sizeof(sensors_event_t);
    mPendingEvent.sensor = ID_A;
    mPendingEvent.type = SENSOR_TYPE_ACCELEROMETER;
    mPendingEvent.acceleration.status = SENSOR_STATUS_ACCURACY_LOW;
    memset(mPendingEvent.data, 0x00, sizeof(mPendingEvent.data));

#if 0
    open_device();

    int flags = 0;
    if (ioctl(dev_fd, KXSD9_IOC_SET_MODE, &flags)>0) {
        if (flags)  {
            mEnabled = 1;
        }
    }
    if (!mEnabled) {
        close_device();
    }
#endif
}

AccelerationSensor::~AccelerationSensor() {
}

int AccelerationSensor::enable(int32_t, int en)
{

    int flags = en ? 1 : 0;
    int err = 0;

    if (flags != mEnabled) {

        // don't turn the accelerometer off, if the orientation
        // sensor is enabled
        if (mOrientationEnabled && !en) {
            mEnabled = flags;
            return 0;
        }
        if (flags) {
            open_device();
        }

        err = ioctl(dev_fd, KXSD9_IOC_SET_MODE, &flags);
        err = err<0 ? -errno : 0;

	if(err)
        	LOGE("KXSD9_IOC_SET_MODE failed (%s)", strerror(-err));
        if (err == 0) {
            mEnabled = flags;
        }
        if (!flags) {
        
            close_device();
        }
    }
    return err;
}

int AccelerationSensor::enableOrientation(int en)
{

    int flags = en ? 1 : 0;
    int err = 0;

    if (flags != mOrientationEnabled) {

        // don't turn the accelerometer off, if the user has requested it
        if (mEnabled && !en) {
            mOrientationEnabled = flags;
            return 0;
        }

        if (flags) {
            open_device();
        }

        err = ioctl(dev_fd, KXSD9_IOC_SET_MODE, &flags);
        err = err<0 ? -errno : 0;
	if(err)
                LOGE("KXSD9_IOC_SET_MODE failed (%s)", strerror(-err));
        if (err == 0) {
            mOrientationEnabled = flags;
        }
        if (!flags) {

            close_device();
        }
    }
    return err;
}

int AccelerationSensor::setDelay(int32_t handle, int64_t ns)
{

    if (ns < 0)
    {
        LOGE("%s:%d", __FUNCTION__,__LINE__);
        return -EINVAL;
    }
    if (mEnabled || mOrientationEnabled) {

        int delay = ns / 1000000;
        if (ioctl(dev_fd, KXSD9_IOC_SET_DELAY, &delay)>0) {
    LOGE("%s:%d, errno %d", __FUNCTION__,__LINE__,-errno);
            return -errno;
        }

    }
    return 0;
}

int AccelerationSensor::readEvents(sensors_event_t* data, int count)
{
    if (count < 1)
    {
        return -EINVAL;
    }   

    ssize_t n = mInputReader.fill(data_fd);
    if (n < 0)
    {
        return n;
    }
    int numEventReceived = 0;
    input_event const* event;

    while (count && mInputReader.readEvent(&event)) {
        int type = event->type;
        if (type == EV_ABS) {

            processEvent(event->code, event->value);

        } else if (type == EV_SYN) {

            int64_t time = timevalToNano(event->time);
            mPendingEvent.timestamp = time;
            if (mEnabled) {
                *data++ = mPendingEvent;
                count--;
                numEventReceived++;
            }
        // accelerometer sends valid ABS events for
        // userspace using EVIOCGABS
        } else if (type != EV_ABS) { 
            LOGE("AccelerationSensor: unknown event (type=%d, code=%d)",
                    type, event->code);
        }
        mInputReader.next();

    }
    return numEventReceived;
}

void AccelerationSensor::processEvent(int code, int value)
{
    switch (code) {
        case EVENT_TYPE_ACCEL_X:
            mPendingEvent.acceleration.x = -((value -2048 ) /81.9);//* CONVERT_A_X;value * CONVERT_A_X;
            break;
        case EVENT_TYPE_ACCEL_Y:
            mPendingEvent.acceleration.y = -((value -2048 ) /81.9);//* CONVERT_A_X;value * CONVERT_A_Y;
            break;
        case EVENT_TYPE_ACCEL_Z:
            mPendingEvent.acceleration.z = ((value -2048 ) /81.9);//* CONVERT_A_X;value * CONVERT_A_Z;
            break;
    }
}
