.PHONY: clean All

All:
	@echo "----------Building project:[ battlenetwork - Debug ]----------"
	@"$(MAKE)" -f  "battlenetwork.mk"
clean:
	@echo "----------Cleaning project:[ battlenetwork - Debug ]----------"
	@"$(MAKE)" -f  "battlenetwork.mk" clean
