#neo: this makefile is used to compile all the parts together, each part has its own buildsystem (e.g. c has cmake)
#neo: output is in dist/
.PHONY: c web python java lua kotlin
all: c web python java lua kotlin
clean:
	rm -rf */build
	rm -rf dist/*
c:
	mkdir -p dist/
	cmake -S c -B c/build
	cmake --build c/build
	cp c/build/polyglotcalc dist/
java:
	javac java/Main.java
	cp java/Main.class dist/
kotlin:
	@if command -v kotlinc >/dev/null 2>&1; then \
		echo "Found kotlinc. Compiling..."; \
		kotlinc kotlin/KotlinMain.kt -d ./kotlin; \
	else \
		echo "kotlinc not found. Skipping compilation and using existing class file..."; \
	fi
	cp kotlin/KotlinMain.class dist/
web:
	cp web/calculator.html dist/index.html
	cp web/calculator.css dist/
	cp web/calculator.js dist/
python:
	mkdir -p dist/
	cp python/find_order.py dist/
	cp python/e_and_pi.py dist/
lua:
	mkdir -p dist/
	cp lua/pow.lua dist/
