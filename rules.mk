
.PHONY: all clean

all: $(LIBNAME)

$(OBJDIR)/%.o : %.c $(ROOTDIR)/config.mk $(ROOTDIR)/rules.mk
	$(CC) $(CFLAGS) -c -o $@ -MMD -MP -MF $(@:.o=.dep) $<

$(OBJDIR)/%.obj : %.cpp $(ROOTDIR)/config.mk $(ROOTDIR)/rules.mk
	$(CC) $(CPPFLAGS) -c -o $@ -MMD -MP -MF $(@:.obj=.dep) $<

$(LIBNAME): $(OBJS) $(CPPOBJS) $(ROOTDIR)/config.mk $(ROOTDIR)/rules.mk
	$(AR) -r $@ $(OBJS) $(CPPOBJS)

clean::
	rm -f $(OBJS) $(CPPOBJS) $(LIBNAME) $(DEPS)

cleandirs::
	rm -r -f $(OBJDIR)
	rm -r -f $(OUTPATH)

dirs::
	mkdir $(OBJDIR)
	mkdir $(OUTPATH)

-include $(wildcard $(OBJDIR)/*.dep)
