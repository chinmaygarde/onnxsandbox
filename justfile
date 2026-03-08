@format:
	find src -name '*.h' -o -name '*.cc' -o -name '*.cpp' | xargs clang-format -i

@format-check:
	find src -name '*.h' -o -name '*.cc' -o -name '*.cpp' | xargs clang-format --dry-run --Werror

@build preset='debug':
	cmake --build --preset {{preset}}

alias gen := setup

@test preset='debug':
	./build/src/ep/onnx_sandbox_tests

@setup preset='debug':
	cmake --preset {{preset}}

@clean:
	rm -rf build

@sync:
	git submodule update --init --recursive -j 8

@list-builds:
	cmake --list-presets

@docker_build:
	devcontainer build --workspace-folder .

@docker_dev: docker_build
	devcontainer up --workspace-folder .
	devcontainer exec --workspace-folder . /bin/bash

# Run a command inside the devcontainer
@docker_exec *args:
	devcontainer exec --workspace-folder . -- {{args}}

# Run individual steps inside the devcontainer
@docker_sync: docker_build
	devcontainer up --workspace-folder .
	just docker_exec just sync

@docker_setup preset='debug': docker_build
	devcontainer up --workspace-folder .
	just docker_exec just setup {{preset}}

@docker_build_code preset='debug': docker_build
	devcontainer up --workspace-folder .
	just docker_exec just build {{preset}}

@docker_test preset='debug': docker_build
	devcontainer up --workspace-folder .
	just docker_exec just test {{preset}}

# Run the full CI pipeline inside the devcontainer (mirrors ubuntu-24.04 CI)
@docker_ci preset='debug': docker_build
	devcontainer up --workspace-folder .
	just docker_exec just sync
	just docker_exec just setup {{preset}}
	just docker_exec just build {{preset}}
	just docker_exec just test {{preset}}
