
all: $(LIBNAME)

$(OBJDIR)/%.o : %.c
	$(CC) $(CFLAGS) -c -o $@ -MMD -MP -MF $(@:.o=.dep) $<

$(LIBNAME): $(OBJS)
	$(AR) -r $@ $(OBJS)

clean:
	rm -f $(OBJS) $(LIBNAME) $(DEPS)


-include $(wildcard $(OBJDIR)/*.dep)
