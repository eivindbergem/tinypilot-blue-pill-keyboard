GDB = gdb-multiarch

SOURCES         += $(wildcard src/*.c)
OBJS            += $(SOURCES:.c=.o)

BINDIR          = bin
ELF             = $(BINDIR)/$(DEVICE).elf
HEX             = $(ELF:%.elf=%.hex)
LD_DIR		= ld

DEVICE = stm32f103c8t6

OPENCM3_DIR     = libopencm3

CFLAGS          += -Os -ggdb3 --std=gnu11 -fms-extensions -Wall -Wextra
CPPFLAGS	+= -MD
LDFLAGS         += -static -nostartfiles
LDLIBS          += -Wl,--start-group -lc -lgcc -lnosys -Wl,--end-group

include $(OPENCM3_DIR)/mk/genlink-config.mk
include $(OPENCM3_DIR)/mk/gcc-config.mk

LDSCRIPT:=$(LD_DIR)/$(LDSCRIPT)

.PHONY: clean all flash gdb

all: $(ELF) $(OBJS)

$(ELF): $(BINDIR)

$(OBJS): $(MAKEFILE_LIST) $(OPENCM3_DIR)

$(BINDIR):
	mkdir -p $@

$(LDSCRIPT): $(LD_DIR)

$(LD_DIR):
	mkdir -p $@

clean:
	$(Q)$(RM) $(ELF) $(HEX) $(OBJS)

include $(OPENCM3_DIR)/mk/genlink-rules.mk
include $(OPENCM3_DIR)/mk/gcc-rules.mk
