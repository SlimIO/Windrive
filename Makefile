.PHONY: install build

install:
	npm install

configure:
	npx node-gyp configure

build:
	npx node-gyp build
