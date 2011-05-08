LOCAL_PATH := $(call my-dir)
include $(CLEAR_VARS)

LOCAL_MODULE    := libpismo

$(LOCAL_PATH)/scanner.c: $(LOCAL_PATH)/scanner.l
	flex -8 -s -o $@ $<

$(LOCAL_PATH)/parser.tab.c: $(LOCAL_PATH)/parser.tab.h

$(LOCAL_PATH)/parser.tab.h: $(LOCAL_PATH)/parser.y
	cd `dirname $^`; bison -v -d $<

$(LOCAL_PATH)/pismo.c: $(LOCAL_PATH)/parser.tab.h

LOCAL_SRC_FILES := pismo.c db_bin.c common.c parser.tab.c scanner.c
LOCAL_LDLIBS    := -llog
#LOCAL_LDLIBS    := -landroid -llog
#LOCAL_STATIC_LIBRARIES := android_native_app_glue


#include $(BUILD_STATIC_LIBRARY)
include $(BUILD_SHARED_LIBRARY)

#$(call import-module,android/native_app_glue)
