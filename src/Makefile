SIBDIR=\
	   wireless \
	   app\
	   drivers\
	   
ifeq ($(VERSION), 1)
	SIBDIR += platform 
	SIBDIR += product 
endif
ifeq ($(VERSION), 2)
	SIBDIR += hal
endif

define make_loop
	@for mkdir in $(SIBDIR); do \
		make -C $$mkdir $1; \
   	done  ;
endef


all :
	$(call make_loop)

debug :
	@echo $(OBJ_DIR)
	$(call make_loop,debug)
