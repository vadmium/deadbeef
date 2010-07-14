LOCAL_PATH := $(call my-dir)
include $(CLEAR_VARS)

LOCAL_MODULE := adplug

LOCAL_SRC_FILES +=\
				  adplug-db.cpp\
				  plugin.c\
				  libbinio/binfile.h\
				  libbinio/binio.h\
				  libbinio/binstr.h\
				  libbinio/binwrap.h\
				  libbinio/binfile.cpp\
				  libbinio/binio.cpp\
				  libbinio/binstr.cpp\
				  libbinio/binwrap.cpp\
				  adplug/a2m.cpp\
				  adplug/a2m.h\
				  adplug/adl.cpp\
				  adplug/adl.h\
				  adplug/adlibemu.c\
				  adplug/adlibemu.h\
				  adplug/adplug.cpp\
				  adplug/adplug.h\
				  adplug/adtrack.cpp\
				  adplug/adtrack.h\
				  adplug/amd.cpp\
				  adplug/amd.h\
				  adplug/analopl.cpp\
				  adplug/analopl.h\
				  adplug/bam.cpp\
				  adplug/bam.h\
				  adplug/bmf.cpp\
				  adplug/bmf.h\
				  adplug/cff.cpp\
				  adplug/cff.h\
				  adplug/d00.cpp\
				  adplug/d00.h\
				  adplug/database.cpp\
				  adplug/database.h\
				  adplug/debug.c\
				  adplug/debug.h\
				  adplug/dfm.cpp\
				  adplug/dfm.h\
				  adplug/diskopl.cpp\
				  adplug/diskopl.h\
				  adplug/dmo.cpp\
				  adplug/dmo.h\
				  adplug/dro2.cpp\
				  adplug/dro2.h\
				  adplug/dro.cpp\
				  adplug/dro.h\
				  adplug/dtm.cpp\
				  adplug/dtm.h\
				  adplug/emuopl.cpp\
				  adplug/emuopl.h\
				  adplug/flash.cpp\
				  adplug/flash.h\
				  adplug/fmc.cpp\
				  adplug/fmc.h\
				  adplug/fmopl.c\
				  adplug/fmopl.h\
				  adplug/fprovide.cpp\
				  adplug/fprovide.h\
				  adplug/hsc.cpp\
				  adplug/hsc.h\
				  adplug/hsp.cpp\
				  adplug/hsp.h\
				  adplug/hybrid.cpp\
				  adplug/hybrid.h\
				  adplug/hyp.cpp\
				  adplug/hyp.h\
				  adplug/imf.cpp\
				  adplug/imf.h\
				  adplug/jbm.cpp\
				  adplug/jbm.h\
				  adplug/kemuopl.h\
				  adplug/ksm.cpp\
				  adplug/ksm.h\
				  adplug/lds.cpp\
				  adplug/lds.h\
				  adplug/mad.cpp\
				  adplug/mad.h\
				  adplug/mid.cpp\
				  adplug/mid.h\
				  adplug/mididata.h\
				  adplug/mkj.cpp\
				  adplug/mkj.h\
				  adplug/msc.cpp\
				  adplug/msc.h\
				  adplug/mtk.cpp\
				  adplug/mtk.h\
				  adplug/opl.h\
				  adplug/player.cpp\
				  adplug/player.h\
				  adplug/players.cpp\
				  adplug/players.h\
				  adplug/protrack.cpp\
				  adplug/protrack.h\
				  adplug/psi.cpp\
				  adplug/psi.h\
				  adplug/rad.cpp\
				  adplug/rad.h\
				  adplug/rat.cpp\
				  adplug/rat.h\
				  adplug/raw.cpp\
				  adplug/raw.h\
				  adplug/realopl.cpp\
				  adplug/realopl.h\
				  adplug/rix.cpp\
				  adplug/rix.h\
				  adplug/rol.cpp\
				  adplug/rol.h\
				  adplug/s3m.cpp\
				  adplug/s3m.h\
				  adplug/sa2.cpp\
				  adplug/sa2.h\
				  adplug/silentopl.h\
				  adplug/sng.cpp\
				  adplug/sng.h\
				  adplug/surroundopl.cpp\
				  adplug/surroundopl.h\
				  adplug/temuopl.cpp\
				  adplug/temuopl.h\
				  adplug/u6m.cpp\
				  adplug/u6m.h\
				  adplug/xad.cpp\
				  adplug/xad.h\
				  adplug/xsm.cpp\
				  adplug/xsm.h

LOCAL_CFLAGS += -Dstricmp=strcasecmp -DVERSION=\"2.1\" -I$(LOCAL_PATH)/adplug -I$(LOCAL_PATH)/libbinio

LOCAL_ARM_MODE := arm

include $(BUILD_STATIC_LIBRARY)

