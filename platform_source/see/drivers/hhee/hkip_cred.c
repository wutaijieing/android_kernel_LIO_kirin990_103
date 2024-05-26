#include <platform_include/see/hkip_cred.h>
#include <linux/printk.h>
#include <linux/spinlock.h>
#include <linux/module.h>
#include <linux/threads.h>
#include <linux/smp.h>

#define CRED_POOL_SIZE 50
enum cred_status {
	CRED_USED = 0,
	CRED_UNUSED,
	CRED_INVALID
};

struct cred_node {
	struct cred *cred;
	enum cred_status status;
	unsigned long flag;
	spinlock_t lock;
};

static struct cred_node creds_pool[CRED_POOL_SIZE];

static struct cred *hkip_search_cred(const struct cred *cred)
{
	unsigned int i;
	struct cred cmp_cred;

	memcpy(&cmp_cred, cred, sizeof(struct cred)); /* unsafe_function_ignore: memcpy */
	cmp_cred.task = current;
	for (i = 0; i < CRED_POOL_SIZE; i++) {
		spin_lock_irqsave(&creds_pool[i].lock, creds_pool[i].flag);
		if (creds_pool[i].cred->task == current &&
		    creds_pool[i].status == CRED_UNUSED &&
		    !memcmp(creds_pool[i].cred, &cmp_cred, sizeof(struct cred))) {
			creds_pool[i].status = CRED_USED;
			spin_unlock_irqrestore(&creds_pool[i].lock, creds_pool[i].flag);
			return creds_pool[i].cred;
		}
		spin_unlock_irqrestore(&creds_pool[i].lock, creds_pool[i].flag);
	}
	for (i = 0; i < CRED_POOL_SIZE; i++) {
		spin_lock_irqsave(&creds_pool[i].lock, creds_pool[i].flag);
		if (creds_pool[i].status == CRED_UNUSED) {
			creds_pool[i].status = CRED_USED;
			wr_memcpy(creds_pool[i].cred, &cmp_cred, sizeof(struct cred));
			spin_unlock_irqrestore(&creds_pool[i].lock, creds_pool[i].flag);
			return creds_pool[i].cred;
		}
		spin_unlock_irqrestore(&creds_pool[i].lock, creds_pool[i].flag);
	}
	return NULL;
}

static bool hkip_put_cred(struct cred *cred)
{
	unsigned int i;

	for (i = 0; i < CRED_POOL_SIZE; i++) {
		spin_lock_irqsave(&creds_pool[i].lock, creds_pool[i].flag);
		if (creds_pool[i].cred == cred) {
			creds_pool[i].status = CRED_UNUSED;
			spin_unlock_irqrestore(&creds_pool[i].lock, creds_pool[i].flag);
			return true;
		}
		spin_unlock_irqrestore(&creds_pool[i].lock, creds_pool[i].flag);
	}

	return false;
}

const struct cred *hkip_override_creds(struct cred *new)
{
	struct cred *cred = NULL;

	cred = hkip_search_cred(new);
	if (!cred) {
		cred = hkip_prepare_creds(new);
		if (!cred) {
			pr_err("prepare cred fail\n");
			return NULL;
		}
	}
	return override_creds(cred);
}

void hkip_revert_creds(const struct cred *old)
{
	const struct cred *cred = current->cred;

	if (!old)
		return;

	revert_creds(old);
	if (!hkip_put_cred(cred))
		put_cred(cred);
}

static int __init hkip_cred_init(void)
{
	unsigned int i;

	for (i = 0; i < CRED_POOL_SIZE; i++) {
		creds_pool[i].cred = hkip_alloc_cred();
		creds_pool[i].status = CRED_UNUSED;
		if (!creds_pool[i].cred) {
			pr_err("preload %u cred fail\n", i);
			creds_pool[i].status = CRED_INVALID;
		}
		creds_pool[i].lock = __SPIN_LOCK_UNLOCKED(creds_pool[i].lock);
	}
	pr_err("hkip cred init success\n");
	return 0;
}

static void __exit hkip_cred_exit(void)
{
	unsigned int i;

	for (i = 0; i < CRED_POOL_SIZE; i++) {
		if (!creds_pool[i].cred)
			continue;
		hkip_free_cred(creds_pool[i].cred);
		creds_pool[i].status = CRED_INVALID;
	}
	pr_err("hkip cred exit success\n");
}

module_init(hkip_cred_init);
module_exit(hkip_cred_exit);
MODULE_DESCRIPTION("FOR HMDFS CRED PROTECTION");
MODULE_LICENSE("GPL");
