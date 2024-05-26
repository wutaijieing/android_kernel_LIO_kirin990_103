#ifndef _HKIP_CRED_H_
#define _HKIP_CRED_H_

#include <linux/cred.h>
#include <linux/atomic.h>


#ifdef CONFIG_HKIP_PROTECT_CRED
const struct cred *hkip_override_creds(struct cred *new);
void hkip_revert_creds(const struct cred *old);
#else
static inline const struct cred *hkip_override_creds(struct cred *new)
{
	return override_creds(new);
}

static inline void hkip_revert_creds(const struct cred *old)
{
	revert_creds(old);
}
#endif /* CONFIG_HKIP_PROTECT_CRED */

#endif /* _HKIP_CRED_H_ */
