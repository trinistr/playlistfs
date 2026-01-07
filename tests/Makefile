test:
	@total=0; failed=0; \
	for test_file in test*.sh; do \
		if [ -f "$$test_file" ]; then \
			total=$$((total + 1)); \
			echo "=== $$test_file ==="; \
			chmod +x "$$test_file" 2>/dev/null; \
			$(RUNFLAGS) ./"$$test_file"; \
			if [ $$? -eq 0 ]; then \
				echo "✓ $$test_file passed"; \
			else \
				echo "✗ $$test_file failed"; \
				failed=$$((failed + 1)); \
			fi; \
			echo ""; \
		fi; \
	done; \
	if [ $$failed -eq 0 ]; then \
		echo "$$total test files executed, all tests nominal!"; \
		exit 0; \
	else \
		echo "$$total test files executed, $$failed failed!"; \
		exit 1; \
	fi
