@build preset='debug':
	cmake --build --preset {{preset}}

alias gen := setup

@test preset='debug':
	./build/src/onnx_sandbox_tests

@setup preset='debug':
	cmake --preset {{preset}}

@clean:
	rm -rf build

@sync:
	git submodule update --init --recursive -j 8

@list-builds:
	cmake --list-presets

@docker_build:
	devcontainer build

@docker_dev: docker_build
	devcontainer up
	devcontainer exec /bin/bash
