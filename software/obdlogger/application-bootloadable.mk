SUBMAKEFILES := Sources/main.mk

LDFLAGS = $(COMMON_FLAGS) $(LIBC_SPECS) $(COMMON_LDFLAGS) -T ./Project_Settings/Linker_Files/MKE06Z128xxx4_flash_bootloadable.ld

TARGET :=main-bootloadable.elf

TGT_POSTMAKE := $(postmake)
TGT_POSTCLEAN := $(postclean)
