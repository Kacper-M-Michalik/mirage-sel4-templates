S5L_OBJS := guest.o elf_solo5.o fault.o vcpu.o 
S5L_OBJS_BUILD := $(addprefix solo5libvmm/, $(S5L_OBJS))

$(S5L_OBJS_BUILD): |solo5libvmm

solo5libvmm.a: $(S5L_OBJS_BUILD)
	$(AR) crv $@ $^
	$(RANLIB) $@

solo5libvmm/%.o: $(SOLO5LIBVMM)/src/%.c
	$(CC) $(CFLAGS) -c -o $@ $<

solo5libvmm/%.o: $(SOLO5LIBVMM)/src/aarch64/%.c
	$(CC) ${CFLAGS} -c -o $@ $<

solo5libvmm:
	mkdir -p $@