#ifndef USB_DESCRIPTORS_H_
#define USB_DESCRIPTORS_H_

#include "common/tusb_common.h"
#include "device/usbd.h"

enum {
    REPORT_ID_EAMU = 1,
    REPORT_ID_FELICA = 2,
    REPORT_ID_LIGHTS = 3,
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

#define AIC_PICO_REPORT_DESC_NKRO                          \
    HID_USAGE_PAGE(HID_USAGE_PAGE_DESKTOP),                \
    HID_USAGE(HID_USAGE_DESKTOP_KEYBOARD),                 \
    HID_COLLECTION(HID_COLLECTION_APPLICATION),            \
        /* Modifier */                                     \
        HID_REPORT_SIZE(1),                                \
        HID_REPORT_COUNT(8),                               \
        HID_USAGE_PAGE(HID_USAGE_PAGE_KEYBOARD),           \
        HID_USAGE_MIN(224),                                \
        HID_USAGE_MAX(231),                                \
        HID_LOGICAL_MIN(0),                                \
        HID_LOGICAL_MAX(1),                                \
        HID_INPUT(HID_DATA | HID_VARIABLE | HID_ABSOLUTE), \
        /* LED output that we don't care */                \
        HID_REPORT_COUNT(5),                               \
        HID_REPORT_SIZE(1),                                \
        HID_USAGE_PAGE(HID_USAGE_PAGE_LED),                \
        HID_USAGE_MIN(1),                                  \
        HID_USAGE_MAX(5),                                  \
        HID_OUTPUT(HID_DATA | HID_VARIABLE | HID_ABSOLUTE),\
        HID_REPORT_COUNT(1),                               \
        HID_REPORT_SIZE(3),                                \
        HID_OUTPUT(HID_CONSTANT),                          \
        /* Full Keyboard Bitmap */                         \
        HID_REPORT_SIZE(1),                                \
        HID_REPORT_COUNT(120),                             \
        HID_LOGICAL_MIN(0),                                \
        HID_LOGICAL_MAX(1),                                \
        HID_USAGE_PAGE(HID_USAGE_PAGE_KEYBOARD),           \
        HID_USAGE_MIN(0),                                  \
        HID_USAGE_MAX(119),                                \
        HID_INPUT(HID_DATA | HID_VARIABLE | HID_ABSOLUTE), \
    HID_COLLECTION_END

#define HID_STRING_MINIMUM(x) HID_REPORT_ITEM(x, 8, RI_TYPE_LOCAL, 1)
#define HID_STRING_MAXIMUM(x) HID_REPORT_ITEM(x, 9, RI_TYPE_LOCAL, 1)

#define AIC_PICO_REPORT_DESC_LIGHT                         \
  HID_USAGE_PAGE(HID_USAGE_PAGE_DESKTOP),                  \
  HID_USAGE(0x00),                                         \
    HID_COLLECTION(HID_COLLECTION_APPLICATION),            \
      HID_REPORT_ID(REPORT_ID_LIGHTS)                      \
      HID_REPORT_COUNT(3), HID_REPORT_SIZE(8),             \
      HID_LOGICAL_MIN(0x00), HID_LOGICAL_MAX_N(0x00ff, 2), \
      HID_USAGE_PAGE(HID_USAGE_PAGE_ORDINAL),              \
      HID_STRING_MINIMUM(9), HID_STRING_MAXIMUM(12),       \
      HID_USAGE_MIN(8), HID_USAGE_MAX(11),                 \
      HID_OUTPUT(HID_DATA | HID_VARIABLE | HID_ABSOLUTE),  \
      HID_REPORT_COUNT(15), HID_REPORT_SIZE(8),            \
      HID_INPUT(HID_CONSTANT | HID_VARIABLE | HID_ABSOLUTE), \
      HID_COLLECTION_END

#endif