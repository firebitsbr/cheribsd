/*-
 * Copyright (c) 2020 Edward Tomasz Napierala <trasz@FreeBSD.org>
 * All rights reserved.
 *
 * This software was developed by SRI International and the University of
 * Cambridge Computer Laboratory (Department of Computer Science and
 * Technology) under DARPA contract HR0011-18-C-0016 ("ECATS"), as part of the
 * DARPA SSITH research programme.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR AND CONTRIBUTORS ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL THE AUTHOR OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 */

#include <atf-c.h>

#include <sys/param.h>
#include <sys/types.h>
#include <sys/sysctl.h>
#include <sys/syslimits.h>
#include <errno.h>
#include <signal.h>
#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

extern char **environ;

/*
 * We are our own helper.  Each test case that requires a helper
 * has its '_h' counterpart.  These helper test cases do nothing
 * (skip) when executed normally.
 */
static void
get_exec_path(char *path)
{
	int sysctlname[4];
	size_t pathlen;
	int ret;

	sysctlname[0] = CTL_KERN;
	sysctlname[1] = KERN_PROC;
	sysctlname[2] = KERN_PROC_PATHNAME;
	sysctlname[3] = -1;

	pathlen = PATH_MAX;
	ret = sysctl(sysctlname, nitems(sysctlname), path, &pathlen, NULL, 0);
	if (ret == -1)
		atf_tc_fail("KERN_PROC_PATHNAME returned %d: %s", ret, strerror(errno));
}

static void
coexec_helper(pid_t pid, char *name, char *arg, char *arg2)
{
	char path[PATH_MAX];
	char *helper_argv[4];
	int error;

	get_exec_path(path);

	helper_argv[0] = path;
	helper_argv[1] = name;
	helper_argv[2] = NULL;

	error = setenv("COCALL_TEST_HELPER_ARG", arg, 1);
	ATF_REQUIRE_EQ(error, 0);
	if (arg2 != NULL) {
		error = setenv("COCALL_TEST_HELPER_ARG2", arg2, 1);
		ATF_REQUIRE_EQ(error, 0);
	}

	error = coexecve(pid, path, helper_argv, environ);
	ATF_REQUIRE_EQ_MSG(error, 0, "failed to execute \"%s %s\": %s",
	    path, name, strerror(errno));
}

static char *
random_string(void)
{
	char *str = NULL;

	asprintf(&str, "cocall_test_%u", arc4random());
	return (str);
}

ATF_TC_WITHOUT_HEAD(cocall);
ATF_TC_BODY(cocall, tc)
{
	void * __capability switcher_code;
	void * __capability switcher_data;
	char *name;
	uint64_t buf;
	pid_t pid, pid2;
	int error;

	name = random_string();

	pid = atf_utils_fork();
	if (pid == 0) {
		error = cosetup(COSETUP_COACCEPT, &switcher_code, &switcher_data);
		ATF_REQUIRE_EQ(error, 0);

		error = coregister(name, NULL);
		ATF_REQUIRE_EQ(error, 0);

		for (;;) {
			buf = 42;
			error = coaccept(switcher_code, switcher_data, NULL, &buf, sizeof(buf));
			ATF_REQUIRE_EQ(error, 0);
			ATF_REQUIRE_EQ(buf, 7);
		}
		atf_tc_fail("You're not supposed to be here");
	}

	pid2 = atf_utils_fork();
	if (pid2 == 0) {
		coexec_helper(pid, "cocall_h", name, NULL);
		atf_tc_fail("You're not supposed to be here");
	}
	atf_utils_wait(pid2, 0, "passed\n", "save:/dev/null");

	error = kill(pid, SIGTERM);
	ATF_REQUIRE_EQ(error, 0);
}

ATF_TC_WITHOUT_HEAD(cocall_h);
ATF_TC_BODY(cocall_h, tc)
{
	void * __capability switcher_code;
	void * __capability switcher_data;
	void * __capability lookedup;
	char *arg;
	uint64_t buf;
	int error;

	arg = getenv("COCALL_TEST_HELPER_ARG");
	if (arg == NULL)
		atf_tc_skip("helper testcase, not supposed to be run directly");

	error = cosetup(COSETUP_COCALL, &switcher_code, &switcher_data);
	ATF_REQUIRE_EQ(error, 0);
	error = colookup(arg, &lookedup);
	ATF_REQUIRE_EQ(error, 0);
	buf = 7;
	error = cocall(switcher_code, switcher_data, lookedup, &buf, sizeof(buf));
	ATF_REQUIRE_EQ(error, 0);
	ATF_REQUIRE_EQ(buf, 42);
}

ATF_TC_WITHOUT_HEAD(cocall_bad_caller_buf);
ATF_TC_BODY(cocall_bad_caller_buf, tc)
{
	void * __capability switcher_code;
	void * __capability switcher_data;
	char *name;
	uint64_t buf;
	pid_t pid, pid2;
	int error;

	name = random_string();

	pid = atf_utils_fork();
	if (pid == 0) {
		error = cosetup(COSETUP_COACCEPT, &switcher_code, &switcher_data);
		ATF_REQUIRE_EQ(error, 0);

		error = coregister(name, NULL);
		ATF_REQUIRE_EQ(error, 0);

		for (;;) {
			buf = 42;
			error = coaccept(switcher_code, switcher_data, NULL, &buf, sizeof(buf));
			ATF_REQUIRE_EQ(error, 0);
			ATF_REQUIRE_EQ(buf, 42);
		}
		atf_tc_fail("You're not supposed to be here");
	}

	pid2 = atf_utils_fork();
	if (pid2 == 0) {
		coexec_helper(pid, "cocall_bad_caller_buf_h", name, NULL);
		atf_tc_fail("You're not supposed to be here");
	}
	atf_utils_wait(pid2, 0, "passed\n", "save:/dev/null");

	error = kill(pid, SIGTERM);
	ATF_REQUIRE_EQ(error, 0);
}

ATF_TC_WITHOUT_HEAD(cocall_bad_caller_buf_h);
ATF_TC_BODY(cocall_bad_caller_buf_h, tc)
{
	void * __capability switcher_code;
	void * __capability switcher_data;
	void * __capability lookedup;
	char *arg;
	int error;

	arg = getenv("COCALL_TEST_HELPER_ARG");
	if (arg == NULL)
		atf_tc_skip("helper testcase, not supposed to be run directly");

	error = cosetup(COSETUP_COCALL, &switcher_code, &switcher_data);
	ATF_REQUIRE_EQ(error, 0);
	error = colookup(arg, &lookedup);
	ATF_REQUIRE_EQ(error, 0);
	error = cocall(switcher_code, switcher_data, lookedup, (void *)13, 8);
	ATF_REQUIRE(error != 0);
}

ATF_TC_WITHOUT_HEAD(cocall_bad_callee_buf);
ATF_TC_BODY(cocall_bad_callee_buf, tc)
{
	void * __capability switcher_code;
	void * __capability switcher_data;
	char *name;
	pid_t pid, pid2;
	int error;

	name = random_string();

	pid = atf_utils_fork();
	if (pid == 0) {
		error = cosetup(COSETUP_COACCEPT, &switcher_code, &switcher_data);
		ATF_REQUIRE_EQ(error, 0);

		error = coregister(name, NULL);
		ATF_REQUIRE_EQ(error, 0);

		for (;;) {
			error = coaccept(switcher_code, switcher_data, NULL, (void *)13, 8);
			ATF_REQUIRE_EQ(error, 0);
		}
		atf_tc_fail("You're not supposed to be here");
	}

	pid2 = atf_utils_fork();
	if (pid2 == 0) {
		coexec_helper(pid, "cocall_bad_callee_buf_h", name, NULL);
		atf_tc_fail("You're not supposed to be here");
	}
	atf_utils_wait(pid2, 0, "passed\n", "save:/dev/null");

	error = kill(pid, SIGTERM);
	ATF_REQUIRE_EQ(error, 0);
}

ATF_TC_WITHOUT_HEAD(cocall_bad_callee_buf_h);
ATF_TC_BODY(cocall_bad_callee_buf_h, tc)
{
	void * __capability switcher_code;
	void * __capability switcher_data;
	void * __capability lookedup;
	char *arg;
	uint64_t buf;
	int error;

	arg = getenv("COCALL_TEST_HELPER_ARG");
	if (arg == NULL)
		atf_tc_skip("helper testcase, not supposed to be run directly");

	error = cosetup(COSETUP_COCALL, &switcher_code, &switcher_data);
	ATF_REQUIRE_EQ(error, 0);
	error = colookup(arg, &lookedup);
	ATF_REQUIRE_EQ(error, 0);
	buf = 7;
	error = cocall(switcher_code, switcher_data, lookedup, &buf, sizeof(buf));
	ATF_REQUIRE_EQ(error, 0);
	ATF_REQUIRE_EQ(buf, 7);
}

ATF_TC_WITHOUT_HEAD(cocall_callee_abort);
ATF_TC_BODY(cocall_callee_abort, tc)
{
	void * __capability switcher_code;
	void * __capability switcher_data;
	char *name;
	uint64_t buf;
	pid_t pid, pid2;
	int error;

	name = random_string();

	pid = atf_utils_fork();
	if (pid == 0) {
		error = cosetup(COSETUP_COACCEPT, &switcher_code, &switcher_data);
		ATF_REQUIRE_EQ(error, 0);

		error = coregister(name, NULL);
		ATF_REQUIRE_EQ(error, 0);

		for (;;) {
			buf = 42;
			error = coaccept(switcher_code, switcher_data, NULL, &buf, sizeof(buf));
			abort();
		}
		atf_tc_fail("You're not supposed to be here");
	}

	pid2 = atf_utils_fork();
	if (pid2 == 0) {
		coexec_helper(pid, "cocall_callee_abort_h", name, NULL);
		atf_tc_fail("You're not supposed to be here");
	}
	atf_utils_wait(pid2, 0, "passed\n", "save:/dev/null");

	error = kill(pid, SIGTERM);
	ATF_REQUIRE_EQ(error, 0);
}

ATF_TC_WITHOUT_HEAD(cocall_callee_abort_h);
ATF_TC_BODY(cocall_callee_abort_h, tc)
{
	void * __capability switcher_code;
	void * __capability switcher_data;
	void * __capability lookedup;
	char *arg;
	uint64_t buf;
	int error;

	arg = getenv("COCALL_TEST_HELPER_ARG");
	if (arg == NULL)
		atf_tc_skip("helper testcase, not supposed to be run directly");

	error = cosetup(COSETUP_COCALL, &switcher_code, &switcher_data);
	ATF_REQUIRE_EQ(error, 0);
	error = colookup(arg, &lookedup);
	ATF_REQUIRE_EQ(error, 0);
	buf = 7;
	error = cocall(switcher_code, switcher_data, lookedup, &buf, sizeof(buf));
	ATF_REQUIRE_EQ(error, 0);
	ATF_REQUIRE_EQ(buf, 7);
}

ATF_TC_WITHOUT_HEAD(cocall_callee_dead);
ATF_TC_BODY(cocall_callee_dead, tc)
{
	void * __capability switcher_code;
	void * __capability switcher_data;
	char *name, *pidstr;
	uint64_t buf;
	pid_t pid, pid2;
	int error;

	name = random_string();

	pid = atf_utils_fork();
	if (pid == 0) {
		error = cosetup(COSETUP_COACCEPT, &switcher_code, &switcher_data);
		ATF_REQUIRE_EQ(error, 0);

		error = coregister(name, NULL);
		ATF_REQUIRE_EQ(error, 0);

		for (;;) {
			buf = 42;
			error = coaccept(switcher_code, switcher_data, NULL, &buf, sizeof(buf));
			ATF_REQUIRE_EQ(error, -1);
			ATF_REQUIRE_ERRNO(EINTR, error);
			ATF_REQUIRE_EQ(buf, 42);
			abort();
		}
		atf_tc_fail("You're not supposed to be here");
	}

	pidstr = NULL;
	asprintf(&pidstr, "%d",pid);
	ATF_REQUIRE(pidstr != NULL);

	pid2 = atf_utils_fork();
	if (pid2 == 0) {
		coexec_helper(pid, "cocall_callee_dead_h", name, pidstr);
		atf_tc_fail("You're not supposed to be here");
	}
	atf_utils_wait(pid2, 0, "passed\n", "save:/dev/null");
}

ATF_TC_WITHOUT_HEAD(cocall_callee_dead_h);
ATF_TC_BODY(cocall_callee_dead_h, tc)
{
	void * __capability switcher_code;
	void * __capability switcher_data;
	void * __capability lookedup;
	char *arg, *arg2;
	uint64_t buf;
	pid_t pid;
	int error;

	arg = getenv("COCALL_TEST_HELPER_ARG");
	if (arg == NULL)
		atf_tc_skip("helper testcase, not supposed to be run directly");

	arg2 = getenv("COCALL_TEST_HELPER_ARG2");
	ATF_REQUIRE(arg2 != NULL);

	pid = atoi(arg2);
	ATF_REQUIRE(pid != 0);

	error = cosetup(COSETUP_COCALL, &switcher_code, &switcher_data);
	ATF_REQUIRE_EQ(error, 0);
	error = colookup(arg, &lookedup);
	ATF_REQUIRE_EQ(error, 0);

	/*
	 * XXX: Slightly racy.
	 */
	sleep(1);

	error = kill(pid, SIGTERM);
	ATF_REQUIRE_EQ(error, 0);

	buf = 7;
	error = cocall(switcher_code, switcher_data, lookedup, &buf, sizeof(buf));
	ATF_REQUIRE_EQ(error, -1);
	ATF_REQUIRE_ERRNO(EINVAL, error);
	ATF_REQUIRE_EQ(buf, 7);
}

ATF_TP_ADD_TCS(tp)
{
	ATF_TP_ADD_TC(tp, cocall);
	ATF_TP_ADD_TC(tp, cocall_h);
	ATF_TP_ADD_TC(tp, cocall_bad_caller_buf);
	ATF_TP_ADD_TC(tp, cocall_bad_caller_buf_h);
	ATF_TP_ADD_TC(tp, cocall_bad_callee_buf);
	ATF_TP_ADD_TC(tp, cocall_bad_callee_buf_h);
	ATF_TP_ADD_TC(tp, cocall_callee_abort);
	ATF_TP_ADD_TC(tp, cocall_callee_abort_h);
	ATF_TP_ADD_TC(tp, cocall_callee_dead);
	ATF_TP_ADD_TC(tp, cocall_callee_dead_h);
#if 0
	ATF_TP_ADD_TC(tp, cocall_many_callers);
	ATF_TP_ADD_TC(tp, cocall_three_way);
	ATF_TP_ADD_TC(tp, cocall_three_way_with_abort);
	ATF_TP_ADD_TC(tp, cocall_three_way_with_abort_2);
#endif

	return atf_no_error();
}
