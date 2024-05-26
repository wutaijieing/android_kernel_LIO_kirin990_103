#include <mali_kbase.h>
#include <mali_kbase_hisilicon.h>
#include "mali_kbase_hisi_lb_mem_pools.h"

//===============================================================================
//The base interfaces to process memory pools.
//===============================================================================

/**
 * kbase_create_mem_pool - Create a memory pool.
 * @kbdev:        Kbase device where memory is used.
 * @max_size:     Maximum number of free pages the pool can hold
 * @order:        Page order for physical page size (order=0=>4kB, order=9=>2MB)
 * @lb_policy_id: Cache policy id of the memory pool.
 * @next_pools:   Pointer to the next pools or NULL.
 *
 * This funciton is used to create and init a memory pool with max_size and order,
 * typically used by @kbase_mem_pool_create_pools.
 */
static struct kbase_mem_pool* kbase_create_mem_pool(struct kbase_device *kbdev,
                                                    size_t max_size,
                                                    size_t order,
                                                    unsigned int lb_policy_id,
                                                    struct kbase_mem_pool* next_pool)
{
	int err;
	struct kbase_mem_pool *pool = vzalloc(sizeof(struct kbase_mem_pool));
	if (!pool)
		return NULL;

	KBASE_DEBUG_ASSERT(kbdev);

	err = kbase_mem_pool_init(pool, max_size, order,
	                          lb_policy_id, kbdev, next_pool);
	if (err) {
		vfree(pool);
		pool = NULL;
	}

	return pool;
}

#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wunused-function"
/**
 * kbase_destroy_mem_pool - Destroy the specific memory pool.
 * @pool:  The pool to destroy.
 *
 * This funciton is used to term the specific memory pool and free it.
 */
static void kbase_destroy_mem_pool(struct kbase_mem_pool* pool)
{
	if (pool != NULL) {
		kbase_mem_pool_term(pool);
		vfree(pool);
		pool = NULL;
	}
}
#pragma GCC diagnostic pop

/**
 * kbase_add_mem_pool - add the specific memory pool to the pool list.
 * @pool:  The pool to add.
 * @pool_list:  Pointer to the pools list to operate.
 *
 * This funciton is used to add the specific memory pool to pool list.
 */
static void kbase_add_mem_pool(struct kbase_mem_pool *pool, struct list_head *pool_list)
{
	KBASE_DEBUG_ASSERT(pool);
	KBASE_DEBUG_ASSERT(pool_list);

	list_add_tail(&pool->pool_entry, pool_list);
}

#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wunused-function"
/**
 * kbase_remove_mem_pool - Remove the specific memory pool from pool list.
 * @pool:  The pool to remove.
 * @pool_list:  Pointer to the pools list to operate.
 *
 * This funciton is used to remove the specific memory pool from pool list.
 */
static void kbase_remove_mem_pool(struct kbase_mem_pool *pool, struct list_head *pool_list)
{
	KBASE_DEBUG_ASSERT(pool);
	KBASE_DEBUG_ASSERT(pool_list);

	struct kbase_mem_pool *walker, *temp;

	list_for_each_entry_safe(walker, temp, pool_list, pool_entry) {
		if (walker == pool) {
			list_del_init(&walker->pool_entry);
			return;
		}
	}

	dev_warn(pool->kbdev->dev, "Can not find specific memory pool(%pK)\n", pool);
	return;
}
#pragma GCC diagnostic pop

/**
 * kbase_find_mem_pool - Find the specific memory pool from pool list by policy id.
 * @policy_id:  The policy id of the memory pool to find.
 * @pool_list:  Pointer to the pools list to operate.
 *
 * This funciton is used to get the specific memory pool from pool list.
 */
static struct kbase_mem_pool *kbase_find_mem_pool(unsigned int policy_id, struct list_head *pool_list)
{
	struct kbase_mem_pool *walker = NULL;

	KBASE_DEBUG_ASSERT(pool_list);
	list_for_each_entry(walker, pool_list, pool_entry) {
		if (walker->lb_policy_id == policy_id) {
			return walker;
		}
	}

	return NULL;
}

/**
 * kbase_mem_pool_mark_dying_pools - Mark all the memory pools of different cache policy dying.
 * @pool_list:  Pointer to the pools list to operate.
 *
 * This funciton is used to mark the memory pool in pool_list to dying state.
 */
static void kbase_mem_pool_mark_dying_pools(struct list_head *pool_list)
{
	struct kbase_mem_pool *walker = NULL;

	KBASE_DEBUG_ASSERT(pool_list);
	list_for_each_entry(walker, pool_list, pool_entry) {
		kbase_mem_pool_mark_dying(walker);
	}
}

/**
 * kbase_mem_pool_destroy_pools - Destroy all the memory pools of different cache policy.
 * @pool_list:  Pointer to the pools list to destroy.
 *
 * This funciton is used to destroy the memory pool in pool_list, used by @kbase_mem_term
 * to destroy the device pools and @kbase_destroy_context to destroy the context pools.
 */
static void kbase_mem_pool_destroy_pools(struct list_head *pool_list)
{
	struct kbase_mem_pool *pool = NULL;
	struct kbase_mem_pool *temp = NULL;

	KBASE_DEBUG_ASSERT(pool_list);
	list_for_each_entry_safe(pool, temp, pool_list, pool_entry) {
		kbase_mem_pool_term(pool);
		list_del_init(&pool->pool_entry);
		vfree(pool);
		pool = NULL;
	}
}

/**
 * kbase_mem_pool_create_pools - Create the memory pools of different cache policy.
 * @kbdev:      Kbase device where memory is used.
 * @init_pools: Pointer to the pools to init.
 * @max_size:   Maximum number of free pages the pool can hold
 * @order:      Page order for physical page size (order=0=>4kB, order=9=>2MB)
 * @next_pools: Pointer to the next pools or NULL.
 *
 * This funciton is used to create and init the memory pool in init_pools, used by @kbase_mem_init
 * to init the device pools and @kbase_create_context to init the context pools.
 */
static int kbase_mem_pool_create_pools(struct kbase_device *kbdev,
	                               struct list_head *init_pools,
	                               size_t max_size,
                                       size_t order,
                                       struct list_head *next_pools)
{
	u8 counter = 0;
	struct kbase_mem_pool *pool = NULL;
	struct kbase_mem_pool *next_pool = NULL;
	struct kbase_hisi_device_data *data;
	struct kbase_mem_cache_policy_info *info;
	LIST_HEAD(pool_list);

	KBASE_DEBUG_ASSERT(NULL != kbdev);
	KBASE_DEBUG_ASSERT(NULL != init_pools);

	data = &kbdev->hisi_dev_data;
	info = data->cache_policy_info;
	while (info)
	{
		if (next_pools) {
			next_pool = kbase_find_mem_pool(info->policy_id, next_pools);
			if (!next_pool) {
				dev_err(kbdev->dev, "Can not find the specfic device memory pool.\n");
				goto rollback;
			}
		}

		pool = kbase_create_mem_pool(kbdev, max_size, order, info->policy_id, next_pool);
		if (!pool) {
			dev_err(kbdev->dev, "Can not create the specfic device memory pool. order(%zu)\n",order);
			goto rollback;
		}

		kbase_add_mem_pool(pool, &pool_list);

		counter++;
		info = info->next_info;
	}

	KBASE_DEBUG_ASSERT(counter == kbdev->nr_cache_policy);

	list_splice(&pool_list, init_pools);

	return 0;

rollback:
	kbase_mem_pool_destroy_pools(&pool_list);
	return -ENOMEM;
}

//===============================================================================
//The common interfaces to process lb_pools.
//===============================================================================

static int kbase_hisi_lb_init_pools(struct kbase_device *kbdev,
				    struct kbase_hisi_lb_pools *pools,
				    bool   device_pool)
{
	struct list_head *next_pools = NULL;
	struct list_head *next_lp_pools = NULL;
	size_t max_size = KBASE_MEM_POOL_MAX_SIZE_KBDEV;
	KBASE_DEBUG_ASSERT(NULL != kbdev);
	KBASE_DEBUG_ASSERT(NULL != pools);

	spin_lock_init(&pools->mem_pools_lock);
	INIT_LIST_HEAD(&pools->mem_pools);
	spin_lock_init(&pools->lp_mem_pools_lock);
	INIT_LIST_HEAD(&pools->lp_mem_pools);

	/* if it is context pool...*/
	if (!device_pool) {
		struct kbase_hisi_lb_pools *dev_pools = &kbdev->hisi_dev_data.lb_pools;
		KBASE_DEBUG_ASSERT(NULL != dev_pools);

		max_size = kbdev->mem_pool_max_size_default;
		next_pools = &dev_pools->mem_pools;
		next_lp_pools = &dev_pools->lp_mem_pools;
	}

	/* Create 4kb page pool for last buffer. init next_pool = NULL. */
	int err = kbase_mem_pool_create_pools(kbdev, &pools->mem_pools,
                                        max_size,
		                    	KBASE_MEM_POOL_4KB_PAGE_TABLE_ORDER,
		                    	next_pools);

	if (err) {
		dev_err(kbdev->dev, "Failed to create memory pool of last buffer with 4KB page size.\n");
		return err;
	}

	/* Create 2MB page pool for last buffer. init next_pool = NULL. */
	err = kbase_mem_pool_create_pools(kbdev, &pools->lp_mem_pools,
                                        max_size >> 9,
		                    	KBASE_MEM_POOL_2MB_PAGE_TABLE_ORDER,
		                    	next_lp_pools);

	if (err) {
		dev_err(kbdev->dev, "Failed to create memory pool of last buffer with 2MB page size.\n");
		kbase_mem_pool_destroy_pools(&pools->mem_pools);
		return err;
	}

	return 0;
}

static void kbase_hisi_lb_destroy_pools(struct kbase_device *kbdev,
                                        struct kbase_hisi_lb_pools *pools)
{
	KBASE_DEBUG_ASSERT(NULL != kbdev);
	KBASE_DEBUG_ASSERT(NULL != pools);

	kbase_mem_pool_destroy_pools(&pools->mem_pools);
	kbase_mem_pool_destroy_pools(&pools->lp_mem_pools);
}

static void kbase_hisi_lb_mark_dying_pools(struct kbase_device *kbdev, 
                                           struct kbase_hisi_lb_pools *pools)
{
	KBASE_DEBUG_ASSERT(NULL != kbdev);
	KBASE_DEBUG_ASSERT(NULL != pools);

	kbase_mem_pool_mark_dying_pools(&pools->mem_pools);
	kbase_mem_pool_mark_dying_pools(&pools->lp_mem_pools);
}

//================================================================================
//Implement the callbacks.
//================================================================================

static int kbase_hisi_lb_create_ctx_pools(struct kbase_context *kctx)
{
	struct kbase_device *kbdev;
	struct kbase_hisi_ctx_data *data;

	KBASE_DEBUG_ASSERT(kctx);
	kbdev = kctx->kbdev;
	KBASE_DEBUG_ASSERT(kbdev);
	data = &kctx->hisi_ctx_data;
	KBASE_DEBUG_ASSERT(data);

	return kbase_hisi_lb_init_pools(kbdev, &data->lb_pools, false);
}

static void kbase_hisi_lb_destroy_ctx_pools(struct kbase_context *kctx)
{
	struct kbase_device *kbdev;
	struct kbase_hisi_ctx_data *data;

	KBASE_DEBUG_ASSERT(kctx);
	kbdev = kctx->kbdev;
	KBASE_DEBUG_ASSERT(kbdev);
	data = &kctx->hisi_ctx_data;
	KBASE_DEBUG_ASSERT(data);

	kbase_hisi_lb_destroy_pools(kbdev, &data->lb_pools);
}

static void kbase_hisi_lb_dying_ctx_pools(struct kbase_context *kctx)
{
	struct kbase_device *kbdev;
	struct kbase_hisi_ctx_data *data;

	KBASE_DEBUG_ASSERT(kctx);
	kbdev = kctx->kbdev;
	KBASE_DEBUG_ASSERT(kbdev);

	data = &kctx->hisi_ctx_data;
	KBASE_DEBUG_ASSERT(data);

	kbase_hisi_lb_mark_dying_pools(kbdev, &data->lb_pools);
}

static int kbase_hisi_lb_create_dev_pools(struct kbase_device *kbdev)
{
	struct kbase_hisi_device_data *data;

	KBASE_DEBUG_ASSERT(kbdev);
	data = &kbdev->hisi_dev_data;
	KBASE_DEBUG_ASSERT(data);

	return kbase_hisi_lb_init_pools(kbdev, &data->lb_pools, true);
}

static void kbase_hisi_lb_destroy_dev_pools(struct kbase_device *kbdev)
{
	struct kbase_hisi_device_data *data;

	KBASE_DEBUG_ASSERT(kbdev);
	data = &kbdev->hisi_dev_data;
	KBASE_DEBUG_ASSERT(data);

	kbase_hisi_lb_destroy_pools(kbdev, &data->lb_pools);
}

/**
 * kbase_hisi_lb_dying_dev_pools - Mark device last buffer pools dying.
 * @kbdev:      Kbase device where memory is used.
 *
 * This funciton is used to mark the device's last buffer pools as dying state.
 * Used as a callback when call @kbase_mem_term().
 */
static void kbase_hisi_lb_dying_dev_pools(struct kbase_device *kbdev)
{
	struct kbase_hisi_device_data *data;

	KBASE_DEBUG_ASSERT(kbdev);
	data = &kbdev->hisi_dev_data;
	KBASE_DEBUG_ASSERT(data);

	kbase_hisi_lb_mark_dying_pools(kbdev, &data->lb_pools);
}

/* The callbacks of last buffer memory pools. */
struct kbase_hisi_lb_pools_callbacks pools_callbacks = {

	.find_pool     = kbase_find_mem_pool,

	.init_ctx_pools = &kbase_hisi_lb_create_ctx_pools,
	.term_ctx_pools = &kbase_hisi_lb_destroy_ctx_pools,
	.dying_ctx_pools= &kbase_hisi_lb_dying_ctx_pools,

	.init_dev_pools = &kbase_hisi_lb_create_dev_pools,
	.term_dev_pools = &kbase_hisi_lb_destroy_dev_pools,
	.dying_dev_pools= &kbase_hisi_lb_dying_dev_pools,
};
