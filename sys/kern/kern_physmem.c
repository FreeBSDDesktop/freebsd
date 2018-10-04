/*-
 * SPDX-License-Identifier: BSD-2-Clause-FreeBSD
 *
 * Copyright (c) 2018 Johannes Lundberg
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

#include <sys/cdefs.h>
__FBSDID("$FreeBSD$");

#include <sys/types.h>
#include <sys/param.h>
#include <sys/systm.h>
#include <machine/physmem.h>

static void phys_avail_insert_at(unsigned int index, vm_paddr_t val);
static void phys_avail_delete_at(unsigned int index);

static void
phys_avail_insert_at(unsigned int index, vm_paddr_t val)
{
	unsigned int i;

	i = PHYS_AVAIL_ARRAY_END;
	while (i >= index) {
		phys_avail[i + 1] = phys_avail[i];
		i--;
	}
	phys_avail[i + 1] = val;
}

static void
phys_avail_delete_at(unsigned int index)
{
	unsigned int i;

	i = index;
	while (i <= PHYS_AVAIL_ARRAY_END) {
		phys_avail[i] = phys_avail[i + 1];
		i++;
	}
}

void
phys_avail_reserve(vm_paddr_t start, vm_paddr_t end)
{
	unsigned int i;
	/* Check if inside a hole before first segment. */
	if (end <= phys_avail[0])
		return;

	i = 0;
	/* Check if inside any hole. */
	while (phys_avail[i + 2] != 0) {
		if (start >= phys_avail[i + 1] && end <= phys_avail[i + 2])
			return;
		i += 2;
	}

	/* Check if new hole is after last segment end (i+1). */
	if (start >= phys_avail[i + 1])
		return;

	i=0;
	/* Check if inside any segment */
	while (phys_avail[i + 1] != 0) {
		if (start > phys_avail[i] && end < phys_avail[i + 1]) {
			phys_avail_insert_at(i + 1, end);
			phys_avail_insert_at(i + 1, start);
			return;
		}
		i += 2;
	}

	/* Other cases */
	i=0;
	while (phys_avail[i] < start && i <= PHYS_AVAIL_ARRAY_END) {
		i++;
	}

	if (phys_avail[i + 1] == 0) {
		/* The new hole starts in the last segment and ends after. */
		phys_avail[i] = start;
		return;
	}

	/* At the index located on or after the new hole start. */
	if (i % 2 == 0) {
		/*
		 * Even index mean beginning of a segment. If hole ends in
		 * segment, shrink the segment. If exact match, delete the
		 * segment. Other cases are handled below.
		 */
		if (start == phys_avail[i]) {
			if (end < phys_avail[i + 1]) {
				phys_avail[i] = end;
				return;
			} else if (end == phys_avail[i + 1]) {
				phys_avail_delete_at(i);
				phys_avail_delete_at(i);
				return;
			}
		}
	} else {
		/*
		 * Odd index mean the hole starts inside a memory segment.
		 * Shrink segment to hole start.
		 */
		phys_avail[i] = start;
		i++;
	}

	int deleted = 0;
	while (phys_avail[i] <= end && phys_avail[i + 1] != 0) {
		phys_avail_delete_at(i);
		deleted++;
	}

	/* At the index located after the new hole end. */
	if (i % 2 == 0 && deleted % 2 == 1) {
		/*
		 * If index is even and odd number of entries have been deleted,
		 * we are at the end of a segment. Insert hole end before this
		 * segment end.
		 */
		phys_avail_insert_at(i, end);
	} else if (i % 2 == 1) {
		/*
		 * Odd index mean our hole ends in the previous segment,
		 * move segment start to new hole end.
		 */
		phys_avail[i-1] = start;
	}
}
