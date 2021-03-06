/*
 * Copyright (c) 2014 Stefan Sørensen <stefan.sorensen@spectralink.com>
 * Copyright (c) 2014-2015 Dmitry V. Levin <ldv@strace.io>
 * Copyright (c) 2014-2019 The strace developers.
 * All rights reserved.
 *
 * SPDX-License-Identifier: LGPL-2.1-or-later
 */

#include "defs.h"

#ifdef HAVE_STRUCT_PTP_SYS_OFFSET

# include "print_fields.h"

# include <linux/ioctl.h>
# include <linux/ptp_clock.h>

# ifndef PTP_CLOCK_GETCAPS2
#  define PTP_CLOCK_GETCAPS2 _IOR(PTP_CLK_MAGIC, 10, struct ptp_clock_caps)
# endif
# ifndef PTP_EXTTS_REQUEST2
#  define PTP_EXTTS_REQUEST2 _IOW(PTP_CLK_MAGIC, 11, struct ptp_extts_request)
# endif
# ifndef PTP_PEROUT_REQUEST2
#  define PTP_PEROUT_REQUEST2 _IOW(PTP_CLK_MAGIC, 12, struct ptp_perout_request)
# endif
# ifndef PTP_ENABLE_PPS2
#  define PTP_ENABLE_PPS2 _IOW(PTP_CLK_MAGIC, 13, int)
# endif
# ifndef PTP_SYS_OFFSET2
#  define PTP_SYS_OFFSET2 _IOW(PTP_CLK_MAGIC, 14, struct ptp_sys_offset)
# endif

# include "xlat/ptp_extts_flags.h"
# include "xlat/ptp_perout_flags.h"

static void
print_ptp_clock_time(const struct ptp_clock_time *const p)
{
	PRINT_FIELD_D("{", *p, sec);
	PRINT_FIELD_U(", ", *p, nsec);
	tprints("}");
	tprints_comment(sprinttime_nsec(p->sec, p->nsec));
}

static bool
print_ptp_clock_time_am(struct tcb *tcp, void *elem_buf, size_t elem_size,
			void *data)
{
	print_ptp_clock_time(elem_buf);
	return true;
}

int
ptp_ioctl(struct tcb *const tcp, const unsigned int code,
	  const kernel_ulong_t arg)
{
	if (!verbose(tcp))
		return RVAL_DECODED;

	switch (code) {
	case PTP_EXTTS_REQUEST:
	case PTP_EXTTS_REQUEST2: {
		struct ptp_extts_request extts;

		tprints(", ");
		if (umove_or_printaddr(tcp, arg, &extts))
			break;

		PRINT_FIELD_D("{", extts, index);
		PRINT_FIELD_FLAGS(", ", extts, flags, ptp_extts_flags, "PTP_???");
		tprints("}");
		break;
	}

	case PTP_PEROUT_REQUEST:
	case PTP_PEROUT_REQUEST2: {
		struct ptp_perout_request perout;

		tprints(", ");
		if (umove_or_printaddr(tcp, arg, &perout))
			break;

		PRINT_FIELD_OBJ_PTR("{", perout, start, print_ptp_clock_time);
		PRINT_FIELD_OBJ_PTR(", ", perout, period, print_ptp_clock_time);
		PRINT_FIELD_D(", ", perout, index);
		PRINT_FIELD_FLAGS(", ", perout, flags, ptp_perout_flags,
				  "PTP_???");
		tprints("}");
		break;
	}

	case PTP_ENABLE_PPS:
	case PTP_ENABLE_PPS2:
		tprintf(", %" PRI_kld, arg);
		break;

	case PTP_SYS_OFFSET:
	case PTP_SYS_OFFSET2: {
		struct ptp_sys_offset sysoff;

		if (entering(tcp)) {
			tprints(", ");
			if (umove_or_printaddr(tcp, arg, &sysoff))
				break;

			PRINT_FIELD_U("{", sysoff, n_samples);
			return 0;
		} else {
			if (tfetch_mem(tcp, arg, sizeof(sysoff), &sysoff)) {
				unsigned int n_samples =
					sysoff.n_samples > PTP_MAX_SAMPLES
					? PTP_MAX_SAMPLES : sysoff.n_samples;
				tprints(", ");
				PRINT_FIELD_ARRAY_UPTO(", ", sysoff, ts,
						       2 * n_samples + 1, tcp,
						       print_ptp_clock_time_am);
			}
			tprints("}");
			break;
		}
	}
	case PTP_CLOCK_GETCAPS:
	case PTP_CLOCK_GETCAPS2: {
		struct ptp_clock_caps caps;

		if (entering(tcp))
			return 0;

		tprints(", ");
		if (umove_or_printaddr(tcp, arg, &caps))
			break;

		PRINT_FIELD_D("{", caps, max_adj);
		PRINT_FIELD_D(", ", caps, n_alarm);
		PRINT_FIELD_D(", ", caps, n_ext_ts);
		PRINT_FIELD_D(", ", caps, n_per_out);
		PRINT_FIELD_D(", ", caps, pps);
		tprints("}");
		break;
	}

	default:
		return RVAL_DECODED;
	}

	return RVAL_IOCTL_DECODED;
}

#endif /* HAVE_STRUCT_PTP_SYS_OFFSET */
