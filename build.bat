@echo off

pushd %~dp0run_tree\cmake
cmake --build . --config debug
popd
