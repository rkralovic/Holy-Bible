LOCAL_PATH := $(call my-dir)

include $(CLEAR_VARS)

LOCAL_MODULE    := libpismo
LOCAL_SRC_FILES := pismo.c db_bin.c common.c parser.tab.c scanner.c
#LOCAL_LDLIBS    := -landroid -llog
LOCAL_LDLIBS    := -llog
#LOCAL_STATIC_LIBRARIES := android_native_app_glue

pismo.c: parser.tab.h

scanner.c: scanner.l parser.tab.c parser.tab.h
	        flex -8 -s -o $@ $<

parser.tab.c: parser.tab.h

parser.tab.h: parser.y
	        bison -v -d $<

#include $(BUILD_STATIC_LIBRARY)
include $(BUILD_SHARED_LIBRARY)

#$(call import-module,android/native_app_glue)
