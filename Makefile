BUILD_ROOT ?= .

include $(BUILD_ROOT)/tools/tools.mk
include $(BUILD_ROOT)/make.mk

BIN_DIR= .
APP=dlna


OBJ_DIR = $(DEBUG_SUFFIX)
BIN = $(OBJ_DIR)/$(APP)

ifeq ($(DEBUG),y)
DEBUG_SUFFIX=debug
CFLAGS += -Wall -g
else
ifeq ($(DEBUG),)
DEBUG_SUFFIX=debug
CFLAGS += -Wall -g
else
DEBUG_SUFFIX=release
endif
endif

ifeq ($(IS_LIB),y)
CFLAGS += --shared -fPIC
endif

.PHONY : $(BIN) clean install $(OBJ_DIR)

$(BIN) : $(OBJ_DIR) $(BIN_DIR) $(LIB_DIR) $(addprefix $(OBJ_DIR)/,$(OBJS))
	@echo [Linking... $(notdir $@)]
ifeq ($(IS_LIB),y)
   	$(Q_)$(CC) $(CXXFLAGS) -o $(BIN) $(addprefix $(OBJ_DIR)/,$(OBJS)) $(LDFLAGS)  
	@echo [Copy $(notdir $(APP)) to $(LIB_DIR)
  	$(Q_)$(CP) $@ $(LIB_DIR)
#	$(Q_)$(CD) $(BIN_DIR) && $(LN) $(APP) $(LIB_NAME)
else
	$(Q_)$(CC) $(CXXFLAGS) -o $(BIN) $(addprefix $(OBJ_DIR)/,$(OBJS)) $(LDFLAGS) 
	@echo [Copy $(notdir $@) to $(BIN_DIR)]
	$(Q_)$(CP) $@ $(BIN_DIR)
endif

$(LIB_DIR) :
	@echo [mkdir : $(LIB_DIR) ]
	$(Q_)$(MKDIR) $(LIB_DIR)

$(BIN_DIR) :
	@echo [mkdir : $(BIN_DIR) ]
	$(Q_)$(MKDIR) $(BIN_DIR)
						
$(OBJ_DIR) :
	@echo [mkdir : $@ ]
	$(Q_)$(MKDIR) $@
							 	
$(OBJ_DIR)/%.o : %.c
	@echo [Compile... $(notdir $<) ]
	$(Q_)$(CC) $(CFLAGS) -c $< -o $@
										
$(OBJ_DIR)/%.o : %.cpp
	@echo [Compile... $(notdir $<) ]
	$(Q_)$(CXX) $(CFLAGS) $(CXXFLAGS) -c $< -o $@

clean :
	$(Q_)$(RM) -rf $(OBJ_DIR)
