# PREFIX fixing
ifneq ("$(LOCALBASE)","")
	PREFIX=$(LOCALBASE)
else
	PREFIX?=/usr
endif
