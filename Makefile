#neo: this makefile is used to compile all the parts together, each part has its own buildsystem (e.g. c has cmake)
#neo: output is in dist/
.PHONY: c web python java
all: c web python java
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
web:
	cp web/calculator.html dist/index.html
	cp web/calculator.css dist/
python:
	mkdir -p dist/
	cp python/find_order.py dist/
