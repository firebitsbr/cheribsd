# $FreeBSD$

.PATH:	${.CURDIR}

PACKAGE=	tests

_REBASE_SCRIPT=	mkimg_test
ATF_TESTS_SH=	${_REBASE_SCRIPT}

SOURCES!=	cd ${.CURDIR}; echo *.hex
${PACKAGE}FILES+=		${SOURCES:S,.hex,,g}

.for f in ${${PACKAGE}FILES}
$f: $f.hex
	sed -e '/^#.*/D' < ${.ALLSRC} > ${.TARGET}
.endfor

# Note: Pre-generating this test file saves a lot of time when building on
# emulated platforms such as QEMU. It can take about 5 seconds to generate
# the test file using jot and this runs for each of the 168 test configurations.
# The effect is even more pronounced on CHERI-RISCV QEMU (emulating CHERI inside
# QEMU adds additional run-time overhead): Running the apm_1x1_512_raw test took
# about 108 seconds of which 102 seconds (over 95%!) were spent running jot -b.
# It's even works on CHERI-MIPS QEMU: 187 seconds for jot -b P 2097152 > /dev/null.
# By using a pre-generated 4MB file, the slowest test variant
# (vtoc8_63x255_4096_vhdx) now only takes 29 seconds (of which 26s are spent
# in hexdump -C) instead of previously 2min30s.
# TODO: Find a way to avoid the slow hexdump -C call.
# FIXME: why is this only so slow on CHERI QEMU? Seems okay-ish upstream
${PACKAGE}FILES+=	partition_data_4M.bin
partition_data_4M.bin: Makefile
	jot -b P 2097152 > ${.TARGET}

CLEANFILES+=	${${PACKAGE}FILES}}

rebase: .PHONY
	(cd ${.CURDIR}; /usr/libexec/atf-sh ${_REBASE_SCRIPT}.sh rebase)

.include <bsd.test.mk>
