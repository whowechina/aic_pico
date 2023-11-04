#ifndef USB_DESCRIPTORS_H_
#define USB_DESCRIPTORS_H_

#include "common/tusb_common.h"
#include "device/usbd.h"

enum {
    REPORT_ID_EAMU = 1,
    REPORT_ID_FELICA = 2,
};

#define AIC_PICO_REPORT_DESC_CARDIO                        \
    HID_USAGE_PAGE_N(0xffca, 2),                           \
    HID_USAGE(0x01),                                       \
      HID_COLLECTION(HID_COLLECTION_APPLICATION),          \
        HID_REPORT_ID(REPORT_ID_EAMU)                      \
        HID_USAGE_PAGE_N(0xffca, 2),                       \
        HID_USAGE(0x41),                                   \
        HID_LOGICAL_MIN(1), HID_LOGICAL_MAX(0xff),         \
        HID_REPORT_SIZE(8), HID_REPORT_COUNT(8),           \
        HID_INPUT(HID_DATA | HID_VARIABLE | HID_ABSOLUTE), \
                                                           \
        HID_REPORT_ID(REPORT_ID_FELICA)                    \
        HID_USAGE_PAGE_N(0xffca, 2),                       \
        HID_USAGE(0x42),                                   \
        HID_LOGICAL_MIN(1), HID_LOGICAL_MAX(0xff),         \
        HID_REPORT_SIZE(8), HID_REPORT_COUNT(8),           \
        HID_INPUT(HID_DATA | HID_VARIABLE | HID_ABSOLUTE), \
    HID_COLLECTION_END

#endif