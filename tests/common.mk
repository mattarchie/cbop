LIBRARY = $(realpath ../inst.a)
LDFLAGS = -lm -Wl,--no-as-needed -ldl -pthread -rdynamic

define PROGRAM_template =
	OBJECTS += $(1)_bop.o $(1)_orig.o
	BINARIES += $(1)_bop $(1)_orig
endef

define \n


endef

$(foreach prog,$(BASE),$(eval $(call PROGRAM_template,$(prog))))



default: $(BINARIES) $(OBJECTS)

test:
	$(foreach x,$(BASE), ./$(x)_orig $(ARGUMENTS)${\n}./$(x)_bop $(ARGUMENTS)${\n})

clean:
	rm -f $(OBJECTS) $(BINARIES)

%_orig.o: %.c
	@echo compiling original test object $@
	@$(CC) $(CFLAGS) $(LDFLAGS) -c -o $@ $? -lm

%_bop.o: %.c
	@echo compiling test object $@
	@$(CC) $(CFLAGS) $(LDFLAGS) -DBOP -c -o $@ $? -lm


%_orig: %_orig.o $(LIBRARY)
	@echo Linking original test binary $@
	@$(CC) $(CFLAGS) -o $@ $?  $(ABS_LIB) $(LDFLAGS)

%_bop: %_bop.o
	@echo Linking bop test binary $@
	@$(CC) $(CFLAGS) -DBOP -o $@ $? $(ABS_LIB) $(NOOMR_LIB) $(LDFLAGS)
