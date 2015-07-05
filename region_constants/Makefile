all: constants.txt

clean:
	@rm -f constants.txt
	@cd E && make clean
	@cd U && make clean
	@cd J && make clean
	@echo "all cleaned up !"

constants.txt:
	@cd $(REGION) && make
	@cp $(REGION)/$@ ./
