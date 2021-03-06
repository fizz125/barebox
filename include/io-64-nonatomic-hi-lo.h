/* SPDX-License-Identifier: GPL-2.0 */
#ifndef _LINUX_IO_64_NONATOMIC_HI_LO_H_
#define _LINUX_IO_64_NONATOMIC_HI_LO_H_

#include <io.h>

static inline __u64 hi_lo_readq(const volatile void __iomem *addr)
{
	const volatile u32 __iomem *p = addr;
	u32 low, high;

	high = readl(p + 1);
	low = readl(p);

	return low + ((u64)high << 32);
}

static inline void hi_lo_writeq(__u64 val, volatile void __iomem *addr)
{
	writel(val >> 32, addr + 4);
	writel(val, addr);
}

#ifndef readq
#define readq hi_lo_readq
#endif

#ifndef writeq
#define writeq hi_lo_writeq
#endif

#endif	/* _LINUX_IO_64_NONATOMIC_HI_LO_H_ */
