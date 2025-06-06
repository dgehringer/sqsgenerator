@echo off
pip install --upgrade pip-tools
pip-compile -v --all-build-deps --output-file=requirements-ci.txt --strip-extras python/pyproject.toml
pip install -r requirements-ci.txt
