# turn off built-in rules
.SUFFIXES:
MAKEFLAGS += -r

CXXFLAGS=-std=c++17
CXX=clang++
RUN_CXX=$(CXX) $(CXXFLAGS)

.PHONY: all
all:

all: template-this.pdf
template-this.pdf: template-this.latex
	pdflatex "$<"
	pdflatex "$<"
	pdflatex "$<"

all: forward_like
forward_like: forward_like.cpp
	$(RUN_CXX) -o "$@" "$<"

all: test-failing
.PHONY: test-failing
test-failing: 

define test_failing_tpl
.PHONY: test-forward-like-$1
test-failing: test-forward-like-$1
test-forward-like-$1: forward_like.cpp
	( ! $$(RUN_CXX) -o "$$@" "$$<" -DENSURE_FAILS_TO_COMPILE_$1 \
	> /dev/null 2>&1 ) || ( echo "$$@ succeeded, something is wrong." && false )
endef
def-test-failing=$(eval $(call test_failing_tpl,$1))

$(call def-test-failing,1)
$(call def-test-failing,2)
$(call def-test-failing,3)
$(call def-test-failing,4)
$(call def-test-failing,5)
$(call def-test-failing,6)
$(call def-test-failing,7)
$(call def-test-failing,8)
$(call def-test-failing,9)
$(call def-test-failing,10)
$(call def-test-failing,11)
$(call def-test-failing,12)
$(call def-test-failing,13)
$(call def-test-failing,14)
$(call def-test-failing,15)
$(call def-test-failing,16)
$(call def-test-failing,17)
$(call def-test-failing,18)
$(call def-test-failing,19)
$(call def-test-failing,20)
$(call def-test-failing,21)
$(call def-test-failing,22)

clean:
	rm forward_like


