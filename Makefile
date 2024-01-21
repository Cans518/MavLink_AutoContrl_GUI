CC := gcc
CFLAGS := -I Lib/ -no-pie
LIBS := $(shell pkg-config --cflags --libs gtk+-3.0)

SRC := AutoLand.c
TARGET :=  build/AutoLand
OUT := AutoLand
BUILDDIR := build
CACHE := $(BUILDDIR)/cache

OBJ := $(CACHE)/AutoLand.o

# ANSI color codes
RED := \033[0;31m
GREEN := \033[0;32m
YELLOW := \033[0;33m
PURPLE := \033[0;35m
BUIL := \033[0;36m
NC := \033[0m

.PHONY: all clean copybuild clean_all install_env

all: $(TARGET) copybuild

$(TARGET): $(OBJ)
	@echo "$(YELLOW)Compiling $@$(NC)"
	@$(CC) $(CFLAGS) $^ -o $@ $(LIBS)
	@echo "$(GREEN)Compiled $@$(NC)"

$(CACHE)/AutoLand.o: $(SRC)
	@mkdir -p $(dir $@)
	@echo "$(YELLOW)Compiling $@$(NC)"
	@$(CC) $(CFLAGS) -c $< -o $@ $(LIBS)
	@echo "$(GREEN)Compiled $@$(NC)"


copybuild: $(TARGET)
	@if [ ! -e ./$(OUT) ]; then \
		echo "$(YELLOW)Copying $(TARGET) to project root...$(NC)"; \
		cp $(TARGET) ./; \
		echo "$(GREEN)Copying done.$(NC)";\
	fi;
	@echo "$(GREEN)Now you can using $(NC) $(BUIL)./AutoLand $(NC) $(GREEN)to use it$(NC)"


clean:
	@echo "$(YELLOW)Cleaning...$(NC)"
	@rm -rf $(BUILDDIR) AutoLand
	@echo "$(GREEN)Cleaning done$(NC)";

clean_all:
	@echo "$(YELLOW)Cleaning...$(NC)"
	@rm -rf $(BUILDDIR) AutoLand  $(SCRIPT)
	@echo "$(GREEN)Cleaning done$(NC)";

install_env:
	@echo "$(PURPLE)Installing dependencies...$(NC)"
	@sudo apt-get update
	@sudo apt-get upgrade -y
	@sudo apt-get install -y libgtk-3-dev devhelp
	@echo "$(PURPLE)Installed dependencies.$(NC)"

# Ensure cache directory exists
$(shell mkdir -p $(CACHE))

# Ensure build directory exists
$(shell mkdir -p $(BUILDDIR))
