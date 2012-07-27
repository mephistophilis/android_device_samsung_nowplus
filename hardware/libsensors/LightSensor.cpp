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

#include "LightSensor.h"
/*****************************************************************************/
/*magic no*/
#define L_IOC_MAGIC  0xF6

/*min seq no*/
#define L_IOC_NR_MIN 20

/*max seq no*/
#define L_IOC_NR_MAX (L_IOC_NR_MIN + 3)

#define L_IOC_GET_ADC_VAL _IOR(L_IOC_MAGIC, (L_IOC_NR_MIN + 0), unsigned int)

#define L_IOC_GET_ILLUM_LVL _IOR(L_IOC_MAGIC, (L_IOC_NR_MIN + 1), unsigned short)

// 20091031 ryun for polling
#define L_IOC_POLLING_TIMER_SET _IOW(L_IOC_MAGIC, (L_IOC_NR_MIN + 2), unsigned short)

#define L_IOC_POLLING_TIMER_CANCEL _IOW(L_IOC_MAGIC, (L_IOC_NR_MIN + 3), unsigned short)

/*****************************************************************************/

LightSensor::LightSensor()
    : SensorBase(LIGHTING_DEVICE_NAME, "light sensor"),
      mEnabled(0),
      mInputReader(4),
      mHasPendingEvent(false)
{
    mPendingEvent.version = sizeof(sensors_event_t);
    mPendingEvent.sensor = ID_L;
    mPendingEvent.type = SENSOR_TYPE_LIGHT;
    memset(mPendingEvent.data, 0, sizeof(mPendingEvent.data));

}

LightSensor::~LightSensor() {
}

int LightSensor::enable(int32_t, int en) {
    int err;
    en = en ? 1 : 0;
    if(mEnabled != en) {
        if (en) {
            open_device();
        }
        err = ioctl(dev_fd, L_IOC_POLLING_TIMER_CANCEL,&en);
        err = err<0 ? -errno : 0;
		if(err)
        	LOGE("MAX9635_IOCTL_SET_ENABLE failed (%s)", strerror(-err));
        if (!err) {
            mEnabled = en;
        }
        if (!en) {
            close_device();
        }
    }
    return 0;
}

bool LightSensor::hasPendingEvents() const {
    return mHasPendingEvent;
}

int LightSensor::readEvents(sensors_event_t* data, int count)
{
    if (count < 1)
        return -EINVAL;

    if (mHasPendingEvent) {
        mHasPendingEvent = false;
        mPendingEvent.timestamp = getTimestamp();
        *data = mPendingEvent;
        return mEnabled ? 1 : 0;
    }

    ssize_t n = mInputReader.fill(data_fd);
    if (n < 0)
        return n;

    int numEventReceived = 0;
    input_event const* event;

    while (count && mInputReader.readEvent(&event)) {
        int type = event->type;
        if (type == EV_MSC) {
            if (event->code == EVENT_TYPE_LIGHT) {
                mPendingEvent.light = indexToValue(event->value);
            }
        } else if (type == EV_SYN) {
            mPendingEvent.timestamp = timevalToNano(event->time);
            if (mEnabled) {
                *data++ = mPendingEvent;
                count--;
                numEventReceived++;
            }
        } else {
            if (type == 4 && event->code == 3) {
                // weird, not sure why we're getting this all the time
            } else {
                LOGE("LightSensor: unknown event (type=%d, code=%d)",
                        type, event->code);
            }
        }
        mInputReader.next();
    }

    return numEventReceived;
}

float LightSensor::indexToValue(size_t index) const
{
    return float(index);
}
