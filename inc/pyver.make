# checking for python
PYTHONBIN?=$(shell which python$(shell pkg-config --modversion python-2.7 2> /dev/null))
PYTHONBIN?=$(shell which python2.6)
PYTHONBIN?=$(shell which python2)

ifeq ("$(PYTHONBIN)", "")
$(error Python not found. Version >= 2.6 is required.)
endif

export
