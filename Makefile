######################################################################
#  Top Level: STM32F103C8T6 Projects
######################################################################

PROJECTS = apps/blink
TOP_DIR 	:= $(dir $(abspath $(lastword $(MAKEFILE_LIST))))

.PHONY = libopencm3 clobber_libopencm3 clean_libopencm3 libthreadx

all:	libopencm3  libthreadx
	for d in $(PROJECTS) ; do \
		$(MAKE) -C $$d ; \
	done

clean:	clean_libopencm3 clean_libthreadx
	for d in $(PROJECTS) ; do \
		$(MAKE) -C $$d clean ; \
	done

clobber: clobber_libopencm3 clobber_libthreadx
	for d in $(PROJECTS) ; do \
		$(MAKE) -C $$d clobber ; \
	done

clean_libopencm3: clobber_libopencm3

clobber_libopencm3:
	rm -f libopencm3/lib/libopencm3_stm32f1.a
	-$(MAKE) -$(MAKEFLAGS) -C ./libopencm3 clean

libopencm3: libopencm3/lib/libopencm3_stm32f1.a

libopencm3/lib/libopencm3_stm32f1.a:
	$(MAKE) -C libopencm3 TARGETS=stm32/f1


clean_libthreadx: clobber_libthreadx

clobber_libthreadx::
	rm -f build/libthreadx.a

libthreadx: build/libthreadx.a

build/libthreadx.a:
	cmake -S$(TOP_DIR)threadx -Bbuild -GNinja -DCMAKE_TOOLCHAIN_FILE=cmake/cortex_m3.cmake 	
	cmake --build build
# Uncomment if necessary:
# MAKE	= gmake

# End
