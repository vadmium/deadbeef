LOCAL_PATH := $(call my-dir)
include $(CLEAR_VARS)

LOCAL_MODULE := adplug

LOCAL_SRC_FILES +=\
				  adplug-db.cpp\
				  plugin.c\
				  libbinio/binfile.cpp\
				  libbinio/binio.cpp\
				  libbinio/binstr.cpp\
				  libbinio/binwrap.cpp\
				  adplug/a2m.cpp\
				  adplug/adl.cpp\
				  adplug/adlibemu.c\
				  adplug/adplug.cpp\
				  adplug/adtrack.cpp\
				  adplug/amd.cpp\
				  adplug/analopl.cpp\
				  adplug/bam.cpp\
				  adplug/bmf.cpp\
				  adplug/cff.cpp\
				  adplug/d00.cpp\
				  adplug/debug.c\
				  adplug/dfm.cpp\
				  adplug/diskopl.cpp\
				  adplug/dmo.cpp\
				  adplug/dro2.cpp\
				  adplug/dro.cpp\
				  adplug/dtm.cpp\
				  adplug/emuopl.cpp\
				  adplug/flash.cpp\
				  adplug/fmc.cpp\
				  adplug/fmopl.c\
				  adplug/fprovide.cpp\
				  adplug/hsc.cpp\
				  adplug/hsp.cpp\
				  adplug/hybrid.cpp\
				  adplug/hyp.cpp\
				  adplug/imf.cpp\
				  adplug/jbm.cpp\
				  adplug/ksm.cpp\
				  adplug/lds.cpp\
				  adplug/mad.cpp\
				  adplug/mid.cpp\
				  adplug/mkj.cpp\
				  adplug/msc.cpp\
				  adplug/mtk.cpp\
				  adplug/player.cpp\
				  adplug/players.cpp\
				  adplug/protrack.cpp\
				  adplug/psi.cpp\
				  adplug/rad.cpp\
				  adplug/rat.cpp\
				  adplug/raw.cpp\
				  adplug/realopl.cpp\
				  adplug/rix.cpp\
				  adplug/rol.cpp\
				  adplug/s3m.cpp\
				  adplug/sa2.cpp\
				  adplug/sng.cpp\
				  adplug/surroundopl.cpp\
				  adplug/temuopl.cpp\
				  adplug/u6m.cpp\
				  adplug/xad.cpp\
				  adplug/xsm.cpp\

LOCAL_CFLAGS += -Dstricmp=strcasecmp -DVERSION=\"2.1\" -I$(LOCAL_PATH)/adplug -I$(LOCAL_PATH)/libbinio -DHAVE_CONFIG_H -I$(LOCAL_PATH)/../.. -O2

LOCAL_ARM_MODE := arm

include $(BUILD_SHARED_LIBRARY)

