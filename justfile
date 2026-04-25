set quiet := true
set shell := ["bash", "-c"]

proj_name := "bumpy"

# Pretty Colors

red := "\\x1b[31m"
green := "\\x1b[32m"
yellow := "\\x1b[33m"
reset := "\\x1b[0m"

# Compiler

cc := "gcc"

# Directories

include := "include"
src := "src"
out := "out"
lib := "lib"

# Flags

include_flags := '-I' + include
debug_shared_flags := '-ggdb -g -Og -fsanitize=address,undefined,leak -fno-sanitize-recover=all -fno-omit-frame-pointer -fno-optimize-sibling-calls -fsanitize-address-use-after-scope -fno-common -std=gnu11'
debug_compile_flags := debug_shared_flags + ' -Wall -Wextra -Wpedantic -Wno-unused-parameter'
debug_link_flags := debug_shared_flags + ' -static-libasan'
release_shared_flags := '-O2 -std=gnu11'
release_compile_flags := release_shared_flags
release_link_flags := release_shared_flags
debug_static_link_flags := debug_link_flags + ' -static'
release_static_link_flags := release_link_flags + ' -static'

# Default justfile target (lists all available targets)
default:
    just --list

compile type="debug" force="false" threads=num_cpus():
    #!/usr/bin/env bash
    shopt -s globstar

    [[ -d {{ out }} ]] || mkdir -p {{ out }}

    WILL_COMPILE=false

    for file in {{ src }}/**/*.c; do
        if [[ $file -nt  {{ out }}/$(basename "${file%.c}")-{{ type }}.o ]]; then
          WILL_COMPILE=true
        fi

        if ! [[ -f {{ out }}/$(basename "${file%.c}")-{{ type }}.o ]]; then
          WILL_COMPILE=true
        fi
    done

    if [[ {{ force }} == "force" || {{ force }} == "true" ]]; then
        echo -e "Compile: Forcing"
        WILL_COMPILE=true
    fi

    if [[ $WILL_COMPILE == false ]]; then
        echo -e "Compile: Nothing to do"
        exit 0
    fi


    echo -e "Using {{ red }}{{ threads }}{{ reset }} threads"
    echo -e "Target: {{ green }}{{ type }}{{ reset }}"
    if [[ {{ type }} == "debug" ]]; then
        find {{ src }} -name "*.c" -print0 | xargs -0 -P{{ threads }} -n1 \
            sh -c 'if [[ "$1" -nt "{{ out }}/$(basename "${1%.c}")-debug.o" || {{ force }} == "true" || {{ force }} == "force" ]]; then echo -e "Compiling {{ green }}$1{{ reset }}..."; {{ cc }} {{ include_flags }} {{ debug_compile_flags }} -c "$1" -o "{{ out }}/$(basename "${1%.c}")-debug.o"; fi' sh
    else
        find {{ src }} -name "*.c" -print0 | xargs -0 -P{{ threads }} -n1 \
            sh -c 'if [[ "$1" -nt "{{ out }}/$(basename "${1%.c}")-release.o" || {{ force }} == "true" || {{ force }} == "force" ]]; then echo -e "Compiling {{ green }}$1{{ reset }}..."; {{ cc }} {{ include_flags }} {{ release_compile_flags }} -c "$1" -o "{{ out }}/$(basename "${1%.c}")-release.o"; fi' sh
    fi

    echo -e "Compile: Compiling {{ green }}{{ type }}{{ reset }} complete"

assemble type="debug" force="false" static="dynamic":
    #!/usr/bin/env bash
    shopt -s globstar

    [[ -d {{ out }} ]] || just compile
    [[ -d {{ lib }} ]] || mkdir -p {{ lib }}

    WILL_ASSEMBLE=false

    if [[ {{ static }} == "dynamic" || {{ static }} == "false" ]]; then
        for file in {{ out }}/*.o; do
            if [[ $file -nt {{ lib }}/lib{{ proj_name }}-{{ type }}.so ]]; then
              WILL_ASSEMBLE=true
            fi
        done
    else
        for file in {{ out }}/*.o; do
            if [[ $file -nt {{ lib }}/lib{{ proj_name }}.a ]]; then
              WILL_ASSEMBLE=true
            fi
        done
    fi

    if [[ $WILL_ASSEMBLE == false ]]; then
        echo -e "Assemble: Nothing to do"
        exit 0
    fi

    echo -e "Target: {{ green }}{{ type }}{{ reset }}"
    if [[ {{ static }} == "dynamic" || {{ static }} == "false" ]]; then
        if [[ {{ type }} == "debug" ]]; then
            {{ cc }} -shared -o {{ lib }}/lib{{ proj_name }}-debug.so {{ out }}/*-debug.o {{ debug_link_flags }} -fuse-ld=mold
        else
            {{ cc }} -shared -o {{ lib }}/lib{{ proj_name }}.so {{ out }}/*-release.o {{ release_link_flags }} -fuse-ld=mold
        fi
    else
        if [[ {{ type }} == "debug" ]]; then
            ar -rcs {{ lib }}/lib{{ proj_name }}-{{ type }}.a {{ out }}/*-{{ type }}.o
            ranlib {{ lib }}/lib{{ proj_name }}-{{ type }}.a
        else
            ar -rcs {{ lib }}/lib{{ proj_name }}.a {{ out }}/*-{{ type }}.o
            ranlib {{ lib }}/lib{{ proj_name }}.a
        fi
    fi

    echo -e "Assemble: Assemble {{ green }}{{ type }}{{ reset }} complete"

build type="debug" force="false" threads=num_cpus() static="dynamic":
    just compile {{ type }} {{ force }} {{ threads }}
    just assemble {{ type }} {{ force }} {{ static }}

debug force="false" threads=num_cpus():
    just build debug {{ force }} {{ threads }}

release force="false" threads=num_cpus():
    just build release {{ force }} {{ threads }}

release-static force="false" threads=num_cpus():
    just build-static {{ force }} {{ threads }}

clean:
    [[ ! -d {{ lib }} ]] || rm -rf {{ lib }}
    [[ ! -d {{ out }} ]] || rm -rf {{ out }}

__bear-compile:
    just compile debug force

bear:
    bear -- just __bear-compile
    sed -i 's|"/nix/store/[^"]*{{ cc }}[^"]*|\"{{ cc }}|g' compile_commands.json
