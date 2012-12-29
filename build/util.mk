%/dirstamp:
	@$(call create-directory,$(@D))
	@touch $(abspath $@)


# Creates a new directory.
# mkdir -p on UNIX platforms.
#
# Example: $(call create-directory,foobar)
#
# Arguments: FOLDER
ifeq ($(WINHOST),y)
MKDIR ?= "c:\Program Files\GnuWin32\bin\mkdir"
create-directory = $(MKDIR) -p "$(1)"
else
MKDIR ?= mkdir
create-directory = $(MKDIR) -p $(abspath $(1))
endif
