
.PHONY: all clean

all: $(LIBNAME)

$(OBJDIR)/%.o : %.c $(OBJDIR) $(ROOTDIR)/config.mk $(ROOTDIR)/rules.mk
	$(CC) $(CFLAGS) -c -o $@ -MMD -MP -MF $(@:.o=.dep) $<

$(OBJDIR)/%.obj : %.cpp $(OBJDIR) $(ROOTDIR)/config.mk $(ROOTDIR)/rules.mk
	$(CC) $(CPPFLAGS) -c -o $@ -MMD -MP -MF $(@:.obj=.dep) $<

$(LIBNAME): $(OBJS) $(CPPOBJS) $(OBJDIR) $(ROOTDIR)/config.mk $(ROOTDIR)/rules.mk
	$(AR) -r $@ $(OBJS) $(CPPOBJS)

clean::
	rm -f $(OBJS) $(CPPOBJS) $(LIBNAME) $(DEPS)
	rm -r -f $(OBJDIR)

$(OBJDIR):
	mkdir $(OBJDIR)

$(OUTPATH):
	mkdir $(OUTPATH)

-include $(wildcard $(OBJDIR)/*.dep)
