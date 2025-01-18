#!/bin/sh
#
# link vmfikus
#
# vmfikus is linked from the objects selected by $(KBUILD_VMFIKUS_INIT) and
# $(KBUILD_VMFIKUS_MAIN). Most are built-in.o files from top-level directories
# in the kernel tree, others are specified in arch/$(ARCH)/Makefile.
# Ordering when linking is important, and $(KBUILD_VMFIKUS_INIT) must be first.
#
# vmfikus
#   ^
#   |
#   +-< $(KBUILD_VMFIKUS_INIT)
#   |   +--< init/version.o + more
#   |
#   +--< $(KBUILD_VMFIKUS_MAIN)
#   |    +--< drivers/built-in.o mm/built-in.o + more
#   |
#   +-< ${kallsymso} (see description in KALLSYMS section)
#
# vmfikus version (uname -v) cannot be updated during normal
# descending-into-subdirs phase since we do not yet know if we need to
# update vmfikus.
# Therefore this step is delayed until just before final link of vmfikus.
#
# System.map is generated to document addresses of all kernel symbols

# Error out on error
set -e

# Nice output in kbuild format
# Will be supressed by "make -s"
info()
{
	if [ "${quiet}" != "silent_" ]; then
		printf "  %-7s %s\n" ${1} ${2}
	fi
}

# Link of vmfikus.o used for section mismatch analysis
# ${1} output file
modpost_link()
{
	${LD} ${LDFLAGS} -r -o ${1} ${KBUILD_VMFIKUS_INIT}                   \
		--start-group ${KBUILD_VMFIKUS_MAIN} --end-group
}

# Link of vmfikus
# ${1} - optional extra .o files
# ${2} - output file
vmfikus_link()
{
	local lds="${objtree}/${KBUILD_LDS}"

	if [ "${SRCARCH}" != "um" ]; then
		${LD} ${LDFLAGS} ${LDFLAGS_vmfikus} -o ${2}                  \
			-T ${lds} ${KBUILD_VMFIKUS_INIT}                     \
			--start-group ${KBUILD_VMFIKUS_MAIN} --end-group ${1}
	else
		${CC} ${CFLAGS_vmfikus} -o ${2}                              \
			-Wl,-T,${lds} ${KBUILD_VMFIKUS_INIT}                 \
			-Wl,--start-group                                    \
				 ${KBUILD_VMFIKUS_MAIN}                      \
			-Wl,--end-group                                      \
			-lutil ${1}
		rm -f fikus
	fi
}


# Create ${2} .o file with all symbols from the ${1} object file
kallsyms()
{
	info KSYM ${2}
	local kallsymopt;

	if [ -n "${CONFIG_HAVE_UNDERSCORE_SYMBOL_PREFIX}" ]; then
		kallsymopt="${kallsymopt} --symbol-prefix=_"
	fi

	if [ -n "${CONFIG_KALLSYMS_ALL}" ]; then
		kallsymopt="${kallsymopt} --all-symbols"
	fi

	if [ -n "${CONFIG_ARM}" ] && [ -n "${CONFIG_PAGE_OFFSET}" ]; then
		kallsymopt="${kallsymopt} --page-offset=$CONFIG_PAGE_OFFSET"
	fi

	local aflags="${KBUILD_AFLAGS} ${KBUILD_AFLAGS_KERNEL}               \
		      ${NOSTDINC_FLAGS} ${FIKUSINCLUDE} ${KBUILD_CPPFLAGS}"

	${NM} -n ${1} | \
		scripts/kallsyms ${kallsymopt} | \
		${CC} ${aflags} -c -o ${2} -x assembler-with-cpp -
}

# Create map file with all symbols from ${1}
# See mksymap for additional details
mksysmap()
{
	${CONFIG_SHELL} "${srctree}/scripts/mksysmap" ${1} ${2}
}

sortextable()
{
	${objtree}/scripts/sortextable ${1}
}

# Delete output files in case of error
trap cleanup SIGHUP SIGINT SIGQUIT SIGTERM ERR
cleanup()
{
	rm -f .old_version
	rm -f .tmp_System.map
	rm -f .tmp_kallsyms*
	rm -f .tmp_version
	rm -f .tmp_vmfikus*
	rm -f System.map
	rm -f vmfikus
	rm -f vmfikus.o
}

#
#
# Use "make V=1" to debug this script
case "${KBUILD_VERBOSE}" in
*1*)
	set -x
	;;
esac

if [ "$1" = "clean" ]; then
	cleanup
	exit 0
fi

# We need access to CONFIG_ symbols
case "${KCONFIG_CONFIG}" in
*/*)
	. "${KCONFIG_CONFIG}"
	;;
*)
	# Force using a file from the current directory
	. "./${KCONFIG_CONFIG}"
esac

#link vmfikus.o
info LD vmfikus.o
modpost_link vmfikus.o

# modpost vmfikus.o to check for section mismatches
${MAKE} -f "${srctree}/scripts/Makefile.modpost" vmfikus.o

# Update version
info GEN .version
if [ ! -r .version ]; then
	rm -f .version;
	echo 1 >.version;
else
	mv .version .old_version;
	expr 0$(cat .old_version) + 1 >.version;
fi;

# final build of init/
${MAKE} -f "${srctree}/scripts/Makefile.build" obj=init

kallsymso=""
kallsyms_vmfikus=""
if [ -n "${CONFIG_KALLSYMS}" ]; then

	# kallsyms support
	# Generate section listing all symbols and add it into vmfikus
	# It's a three step process:
	# 1)  Link .tmp_vmfikus1 so it has all symbols and sections,
	#     but __kallsyms is empty.
	#     Running kallsyms on that gives us .tmp_kallsyms1.o with
	#     the right size
	# 2)  Link .tmp_vmfikus2 so it now has a __kallsyms section of
	#     the right size, but due to the added section, some
	#     addresses have shifted.
	#     From here, we generate a correct .tmp_kallsyms2.o
	# 2a) We may use an extra pass as this has been necessary to
	#     woraround some alignment related bugs.
	#     KALLSYMS_EXTRA_PASS=1 is used to trigger this.
	# 3)  The correct ${kallsymso} is linked into the final vmfikus.
	#
	# a)  Verify that the System.map from vmfikus matches the map from
	#     ${kallsymso}.

	kallsymso=.tmp_kallsyms2.o
	kallsyms_vmfikus=.tmp_vmfikus2

	# step 1
	vmfikus_link "" .tmp_vmfikus1
	kallsyms .tmp_vmfikus1 .tmp_kallsyms1.o

	# step 2
	vmfikus_link .tmp_kallsyms1.o .tmp_vmfikus2
	kallsyms .tmp_vmfikus2 .tmp_kallsyms2.o

	# step 2a
	if [ -n "${KALLSYMS_EXTRA_PASS}" ]; then
		kallsymso=.tmp_kallsyms3.o
		kallsyms_vmfikus=.tmp_vmfikus3

		vmfikus_link .tmp_kallsyms2.o .tmp_vmfikus3

		kallsyms .tmp_vmfikus3 .tmp_kallsyms3.o
	fi
fi

info LD vmfikus
vmfikus_link "${kallsymso}" vmfikus

if [ -n "${CONFIG_BUILDTIME_EXTABLE_SORT}" ]; then
	info SORTEX vmfikus
	sortextable vmfikus
fi

info SYSMAP System.map
mksysmap vmfikus System.map

# step a (see comment above)
if [ -n "${CONFIG_KALLSYMS}" ]; then
	mksysmap ${kallsyms_vmfikus} .tmp_System.map

	if ! cmp -s System.map .tmp_System.map; then
		echo >&2 Inconsistent kallsyms data
		echo >&2 Try "make KALLSYMS_EXTRA_PASS=1" as a workaround
		cleanup
		exit 1
	fi
fi

# We made a new kernel - delete old version file
rm -f .old_version
