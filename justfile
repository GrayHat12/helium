BUILD_DIR := './build'
SOURCE_DIR := '.'
EXECUTABLE := 'helium'

# default recipe to display help information
[default]
_list:
  @just --list

# one time initial cmake setup
@setup:
    mkdir -p {{BUILD_DIR}}
    @cmake -S {{SOURCE_DIR}} -B {{BUILD_DIR}}

# build your code
@build: setup
    @cmake --build {{BUILD_DIR}}

# run your code with any arguments you want to specify
[positional-arguments]
@run *args: build
    @{{BUILD_DIR}}/{{EXECUTABLE}} {{args}}