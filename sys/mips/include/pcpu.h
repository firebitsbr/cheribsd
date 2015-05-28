/*-
 * Copyright (c) 1999 Luoqi Chen <luoqi@freebsd.org>
 * Copyright (c) Peter Wemm <peter@netplex.com.au>
 * All rights reserved.
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
 *
 *	from: src/sys/alpha/include/pcpu.h,v 1.15 2004/11/05 19:16:44 jhb
 * $FreeBSD$
 */

#ifndef _MACHINE_PCPU_H_
#define	_MACHINE_PCPU_H_

#include <machine/cpufunc.h>
#include <machine/pte.h>

#if defined(MIPS_EXC_CNTRS) && defined(CPU_CHERI)
#define	PCPU_MD_COMMON_FIELDS						\
	pd_entry_t	*pc_segbase;		/* curthread segbase */	\
	struct	pmap	*pc_curpmap;		/* pmap of curthread */	\
	register_t	pc_tlb_miss_cnt;	/* TLB miss count */    \
	register_t	pc_tlb_invalid_cnt;	/* TLB invalid count */ \
	register_t	pc_tlb_mod_cnt;		/* TLB modification count */ \
	register_t	pc_cheri_ccall_cnt;	/* Cheri ccall count */ \
	register_t	pc_cheri_creturn_cnt;	/* Cheri creturn count */ \
	u_int32_t	pc_next_asid;		/* next ASID to alloc */ \
	u_int32_t	pc_asid_generation;	/* current ASID generation */ \
	u_int		pc_pending_ipis;	/* IPIs pending to this CPU */

#define	PCPU_NUM_EXC_CNTRS	5

#elif defined(MIPS_EXC_CNTRS) && !defined(CPU_CHERI)
#define	PCPU_MD_COMMON_FIELDS						\
	pd_entry_t	*pc_segbase;		/* curthread segbase */	\
	struct	pmap	*pc_curpmap;		/* pmap of curthread */	\
	register_t	pc_tlb_miss_cnt;	/* TLB miss count */    \
	register_t	pc_tlb_invalid_cnt;	/* TLB invalid count */ \
	register_t	pc_tlb_mod_cnt;		/* TLB modification count */ \
	u_int32_t	pc_next_asid;		/* next ASID to alloc */ \
	u_int32_t	pc_asid_generation;	/* current ASID generation */ \
	u_int		pc_pending_ipis;	/* IPIs pending to this CPU */

#define	PCPU_NUM_EXC_CNTRS	3

#else /* ! defined(MIPS_EXC_CNTRS) */

#define	PCPU_MD_COMMON_FIELDS						\
	pd_entry_t	*pc_segbase;		/* curthread segbase */	\
	struct	pmap	*pc_curpmap;		/* pmap of curthread */	\
	u_int32_t	pc_next_asid;		/* next ASID to alloc */ \
	u_int32_t	pc_asid_generation;	/* current ASID generation */ \
	u_int		pc_pending_ipis;	/* IPIs pending to this CPU */

#define	PCPU_NUM_EXC_CNTRS	0

#endif /* ! defined(MIPS_EXC_CNTRS) */

#ifdef	__mips_n64
#define	PCPU_MD_MIPS64_FIELDS						\
	PCPU_MD_COMMON_FIELDS						\
	char		__pad[(61 - (PCPU_NUM_EXC_CNTRS * 8))]
#else
#define	PCPU_MD_MIPS32_FIELDS						\
	PCPU_MD_COMMON_FIELDS						\
	char		__pad[(133 - (PCPU_NUM_EXC_CNTRS * 4))]
#endif

#ifdef	__mips_n64
#define	PCPU_MD_FIELDS	PCPU_MD_MIPS64_FIELDS
#else
#define	PCPU_MD_FIELDS	PCPU_MD_MIPS32_FIELDS
#endif

#ifdef _KERNEL

extern char pcpu_space[MAXCPU][PAGE_SIZE * 2];
#define	PCPU_ADDR(cpu)		(struct pcpu *)(pcpu_space[(cpu)])

extern struct pcpu *pcpup;
#define	PCPUP	pcpup

#define	PCPU_ADD(member, value)	(PCPUP->pc_ ## member += (value))
#define	PCPU_GET(member)	(PCPUP->pc_ ## member)
#define	PCPU_INC(member)	PCPU_ADD(member, 1)
#define	PCPU_PTR(member)	(&PCPUP->pc_ ## member)
#define	PCPU_SET(member,value)	(PCPUP->pc_ ## member = (value))
#define PCPU_LAZY_INC(member)   (++PCPUP->pc_ ## member)

#ifdef SMP
/*
 * Instantiate the wired TLB entry at PCPU_TLB_ENTRY to map 'pcpu' at 'pcpup'.
 */
void	mips_pcpu_tlb_init(struct pcpu *pcpu);
#endif

#endif	/* _KERNEL */

#endif	/* !_MACHINE_PCPU_H_ */
