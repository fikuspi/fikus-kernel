/**************************************************************************
 *
 * Copyright © 2009 VMware, Inc., Palo Alto, CA., USA
 * All Rights Reserved.
 *
 * Permission is hereby granted, free of charge, to any person obtaining a
 * copy of this software and associated documentation files (the
 * "Software"), to deal in the Software without restriction, including
 * without limitation the rights to use, copy, modify, merge, publish,
 * distribute, sub license, and/or sell copies of the Software, and to
 * permit persons to whom the Software is furnished to do so, subject to
 * the following conditions:
 *
 * The above copyright notice and this permission notice (including the
 * next paragraph) shall be included in all copies or substantial portions
 * of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NON-INFRINGEMENT. IN NO EVENT SHALL
 * THE COPYRIGHT HOLDERS, AUTHORS AND/OR ITS SUPPLIERS BE LIABLE FOR ANY CLAIM,
 * DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR
 * OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE
 * USE OR OTHER DEALINGS IN THE SOFTWARE.
 *
 **************************************************************************/

#ifndef _VMWGFX_DRV_H_
#define _VMWGFX_DRV_H_

#include "vmwgfx_reg.h"
#include <drm/drmP.h>
#include <drm/vmwgfx_drm.h>
#include <drm/drm_hashtab.h>
#include <fikus/suspend.h>
#include <drm/ttm/ttm_bo_driver.h>
#include <drm/ttm/ttm_object.h>
#include <drm/ttm/ttm_lock.h>
#include <drm/ttm/ttm_execbuf_util.h>
#include <drm/ttm/ttm_module.h>
#include "vmwgfx_fence.h"

#define VMWGFX_DRIVER_DATE "20120209"
#define VMWGFX_DRIVER_MAJOR 2
#define VMWGFX_DRIVER_MINOR 4
#define VMWGFX_DRIVER_PATCHLEVEL 0
#define VMWGFX_FILE_PAGE_OFFSET 0x00100000
#define VMWGFX_FIFO_STATIC_SIZE (1024*1024)
#define VMWGFX_MAX_RELOCATIONS 2048
#define VMWGFX_MAX_VALIDATIONS 2048
#define VMWGFX_MAX_DISPLAYS 16
#define VMWGFX_CMD_BOUNCE_INIT_SIZE 32768

#define VMW_PL_GMR TTM_PL_PRIV0
#define VMW_PL_FLAG_GMR TTM_PL_FLAG_PRIV0

#define VMW_RES_CONTEXT ttm_driver_type0
#define VMW_RES_SURFACE ttm_driver_type1
#define VMW_RES_STREAM ttm_driver_type2
#define VMW_RES_FENCE ttm_driver_type3

struct vmw_fpriv {
	struct drm_master *locked_master;
	struct ttm_object_file *tfile;
	struct list_head fence_events;
};

struct vmw_dma_buffer {
	struct ttm_buffer_object base;
	struct list_head res_list;
};

/**
 * struct vmw_validate_buffer - Carries validation info about buffers.
 *
 * @base: Validation info for TTM.
 * @hash: Hash entry for quick lookup of the TTM buffer object.
 *
 * This structure contains also driver private validation info
 * on top of the info needed by TTM.
 */
struct vmw_validate_buffer {
	struct ttm_validate_buffer base;
	struct drm_hash_item hash;
};

struct vmw_res_func;
struct vmw_resource {
	struct kref kref;
	struct vmw_private *dev_priv;
	int id;
	bool avail;
	unsigned long backup_size;
	bool res_dirty; /* Protected by backup buffer reserved */
	bool backup_dirty; /* Protected by backup buffer reserved */
	struct vmw_dma_buffer *backup;
	unsigned long backup_offset;
	const struct vmw_res_func *func;
	struct list_head lru_head; /* Protected by the resource lock */
	struct list_head mob_head; /* Protected by @backup reserved */
	void (*res_free) (struct vmw_resource *res);
	void (*hw_destroy) (struct vmw_resource *res);
};

enum vmw_res_type {
	vmw_res_context,
	vmw_res_surface,
	vmw_res_stream,
	vmw_res_max
};

struct vmw_cursor_snooper {
	struct drm_crtc *crtc;
	size_t age;
	uint32_t *image;
};

struct vmw_framebuffer;
struct vmw_surface_offset;

struct vmw_surface {
	struct vmw_resource res;
	uint32_t flags;
	uint32_t format;
	uint32_t mip_levels[DRM_VMW_MAX_SURFACE_FACES];
	struct drm_vmw_size base_size;
	struct drm_vmw_size *sizes;
	uint32_t num_sizes;
	bool scanout;
	/* TODO so far just a extra pointer */
	struct vmw_cursor_snooper snooper;
	struct vmw_surface_offset *offsets;
	SVGA3dTextureFilter autogen_filter;
	uint32_t multisample_count;
};

struct vmw_marker_queue {
	struct list_head head;
	struct timespec lag;
	struct timespec lag_time;
	spinlock_t lock;
};

struct vmw_fifo_state {
	unsigned long reserved_size;
	__le32 *dynamic_buffer;
	__le32 *static_buffer;
	unsigned long static_buffer_size;
	bool using_bounce_buffer;
	uint32_t capabilities;
	struct mutex fifo_mutex;
	struct rw_semaphore rwsem;
	struct vmw_marker_queue marker_queue;
};

struct vmw_relocation {
	SVGAGuestPtr *location;
	uint32_t index;
};

/**
 * struct vmw_res_cache_entry - resource information cache entry
 *
 * @valid: Whether the entry is valid, which also implies that the execbuf
 * code holds a reference to the resource, and it's placed on the
 * validation list.
 * @handle: User-space handle of a resource.
 * @res: Non-ref-counted pointer to the resource.
 *
 * Used to avoid frequent repeated user-space handle lookups of the
 * same resource.
 */
struct vmw_res_cache_entry {
	bool valid;
	uint32_t handle;
	struct vmw_resource *res;
	struct vmw_resource_val_node *node;
};

struct vmw_sw_context{
	struct drm_open_hash res_ht;
	bool res_ht_initialized;
	bool kernel; /**< is the called made from the kernel */
	struct ttm_object_file *tfile;
	struct list_head validate_nodes;
	struct vmw_relocation relocs[VMWGFX_MAX_RELOCATIONS];
	uint32_t cur_reloc;
	struct vmw_validate_buffer val_bufs[VMWGFX_MAX_VALIDATIONS];
	uint32_t cur_val_buf;
	uint32_t *cmd_bounce;
	uint32_t cmd_bounce_size;
	struct list_head resource_list;
	uint32_t fence_flags;
	struct ttm_buffer_object *cur_query_bo;
	struct list_head res_relocations;
	uint32_t *buf_start;
	struct vmw_res_cache_entry res_cache[vmw_res_max];
	struct vmw_resource *last_query_ctx;
	bool needs_post_query_barrier;
	struct vmw_resource *error_resource;
};

struct vmw_legacy_display;
struct vmw_overlay;

struct vmw_master {
	struct ttm_lock lock;
	struct mutex fb_surf_mutex;
	struct list_head fb_surf;
};

struct vmw_vga_topology_state {
	uint32_t width;
	uint32_t height;
	uint32_t primary;
	uint32_t pos_x;
	uint32_t pos_y;
};

struct vmw_private {
	struct ttm_bo_device bdev;
	struct ttm_bo_global_ref bo_global_ref;
	struct drm_global_reference mem_global_ref;

	struct vmw_fifo_state fifo;

	struct drm_device *dev;
	unsigned long vmw_chipset;
	unsigned int io_start;
	uint32_t vram_start;
	uint32_t vram_size;
	uint32_t mmio_start;
	uint32_t mmio_size;
	uint32_t fb_max_width;
	uint32_t fb_max_height;
	uint32_t initial_width;
	uint32_t initial_height;
	__le32 __iomem *mmio_virt;
	int mmio_mtrr;
	uint32_t capabilities;
	uint32_t max_gmr_descriptors;
	uint32_t max_gmr_ids;
	uint32_t max_gmr_pages;
	uint32_t memory_size;
	bool has_gmr;
	struct mutex hw_mutex;

	/*
	 * VGA registers.
	 */

	struct vmw_vga_topology_state vga_save[VMWGFX_MAX_DISPLAYS];
	uint32_t vga_width;
	uint32_t vga_height;
	uint32_t vga_bpp;
	uint32_t vga_bpl;
	uint32_t vga_pitchlock;

	uint32_t num_displays;

	/*
	 * Framebuffer info.
	 */

	void *fb_info;
	struct vmw_legacy_display *ldu_priv;
	struct vmw_screen_object_display *sou_priv;
	struct vmw_overlay *overlay_priv;

	/*
	 * Context and surface management.
	 */

	rwlock_t resource_lock;
	struct idr res_idr[vmw_res_max];
	/*
	 * Block lastclose from racing with firstopen.
	 */

	struct mutex init_mutex;

	/*
	 * A resource manager for kernel-only surfaces and
	 * contexts.
	 */

	struct ttm_object_device *tdev;

	/*
	 * Fencing and IRQs.
	 */

	atomic_t marker_seq;
	wait_queue_head_t fence_queue;
	wait_queue_head_t fifo_queue;
	int fence_queue_waiters; /* Protected by hw_mutex */
	int goal_queue_waiters; /* Protected by hw_mutex */
	atomic_t fifo_queue_waiters;
	uint32_t last_read_seqno;
	spinlock_t irq_lock;
	struct vmw_fence_manager *fman;
	uint32_t irq_mask;

	/*
	 * Device state
	 */

	uint32_t traces_state;
	uint32_t enable_state;
	uint32_t config_done_state;

	/**
	 * Execbuf
	 */
	/**
	 * Protected by the cmdbuf mutex.
	 */

	struct vmw_sw_context ctx;
	struct mutex cmdbuf_mutex;

	/**
	 * Operating mode.
	 */

	bool stealth;
	bool enable_fb;

	/**
	 * Master management.
	 */

	struct vmw_master *active_master;
	struct vmw_master fbdev_master;
	struct notifier_block pm_nb;
	bool suspended;

	struct mutex release_mutex;
	uint32_t num_3d_resources;

	/*
	 * Query processing. These members
	 * are protected by the cmdbuf mutex.
	 */

	struct ttm_buffer_object *dummy_query_bo;
	struct ttm_buffer_object *pinned_bo;
	uint32_t query_cid;
	uint32_t query_cid_valid;
	bool dummy_query_bo_pinned;

	/*
	 * Surface swapping. The "surface_lru" list is protected by the
	 * resource lock in order to be able to destroy a surface and take
	 * it off the lru atomically. "used_memory_size" is currently
	 * protected by the cmdbuf mutex for simplicity.
	 */

	struct list_head res_lru[vmw_res_max];
	uint32_t used_memory_size;
};

static inline struct vmw_surface *vmw_res_to_srf(struct vmw_resource *res)
{
	return container_of(res, struct vmw_surface, res);
}

static inline struct vmw_private *vmw_priv(struct drm_device *dev)
{
	return (struct vmw_private *)dev->dev_private;
}

static inline struct vmw_fpriv *vmw_fpriv(struct drm_file *file_priv)
{
	return (struct vmw_fpriv *)file_priv->driver_priv;
}

static inline struct vmw_master *vmw_master(struct drm_master *master)
{
	return (struct vmw_master *) master->driver_priv;
}

static inline void vmw_write(struct vmw_private *dev_priv,
			     unsigned int offset, uint32_t value)
{
	outl(offset, dev_priv->io_start + VMWGFX_INDEX_PORT);
	outl(value, dev_priv->io_start + VMWGFX_VALUE_PORT);
}

static inline uint32_t vmw_read(struct vmw_private *dev_priv,
				unsigned int offset)
{
	uint32_t val;

	outl(offset, dev_priv->io_start + VMWGFX_INDEX_PORT);
	val = inl(dev_priv->io_start + VMWGFX_VALUE_PORT);
	return val;
}

int vmw_3d_resource_inc(struct vmw_private *dev_priv, bool unhide_svga);
void vmw_3d_resource_dec(struct vmw_private *dev_priv, bool hide_svga);

/**
 * GMR utilities - vmwgfx_gmr.c
 */

extern int vmw_gmr_bind(struct vmw_private *dev_priv,
			struct page *pages[],
			unsigned long num_pages,
			int gmr_id);
extern void vmw_gmr_unbind(struct vmw_private *dev_priv, int gmr_id);

/**
 * Resource utilities - vmwgfx_resource.c
 */
struct vmw_user_resource_conv;
extern const struct vmw_user_resource_conv *user_surface_converter;
extern const struct vmw_user_resource_conv *user_context_converter;

extern struct vmw_resource *vmw_context_alloc(struct vmw_private *dev_priv);
extern void vmw_resource_unreference(struct vmw_resource **p_res);
extern struct vmw_resource *vmw_resource_reference(struct vmw_resource *res);
extern int vmw_resource_validate(struct vmw_resource *res);
extern int vmw_resource_reserve(struct vmw_resource *res, bool no_backup);
extern bool vmw_resource_needs_backup(const struct vmw_resource *res);
extern int vmw_context_destroy_ioctl(struct drm_device *dev, void *data,
				     struct drm_file *file_priv);
extern int vmw_context_define_ioctl(struct drm_device *dev, void *data,
				    struct drm_file *file_priv);
extern int vmw_context_check(struct vmw_private *dev_priv,
			     struct ttm_object_file *tfile,
			     int id,
			     struct vmw_resource **p_res);
extern int vmw_user_lookup_handle(struct vmw_private *dev_priv,
				  struct ttm_object_file *tfile,
				  uint32_t handle,
				  struct vmw_surface **out_surf,
				  struct vmw_dma_buffer **out_buf);
extern int vmw_user_resource_lookup_handle(
	struct vmw_private *dev_priv,
	struct ttm_object_file *tfile,
	uint32_t handle,
	const struct vmw_user_resource_conv *converter,
	struct vmw_resource **p_res);
extern void vmw_surface_res_free(struct vmw_resource *res);
extern int vmw_surface_destroy_ioctl(struct drm_device *dev, void *data,
				     struct drm_file *file_priv);
extern int vmw_surface_define_ioctl(struct drm_device *dev, void *data,
				    struct drm_file *file_priv);
extern int vmw_surface_reference_ioctl(struct drm_device *dev, void *data,
				       struct drm_file *file_priv);
extern int vmw_surface_check(struct vmw_private *dev_priv,
			     struct ttm_object_file *tfile,
			     uint32_t handle, int *id);
extern int vmw_surface_validate(struct vmw_private *dev_priv,
				struct vmw_surface *srf);
extern void vmw_dmabuf_bo_free(struct ttm_buffer_object *bo);
extern int vmw_dmabuf_init(struct vmw_private *dev_priv,
			   struct vmw_dma_buffer *vmw_bo,
			   size_t size, struct ttm_placement *placement,
			   bool interuptable,
			   void (*bo_free) (struct ttm_buffer_object *bo));
extern int vmw_user_dmabuf_verify_access(struct ttm_buffer_object *bo,
				  struct ttm_object_file *tfile);
extern int vmw_dmabuf_alloc_ioctl(struct drm_device *dev, void *data,
				  struct drm_file *file_priv);
extern int vmw_dmabuf_unref_ioctl(struct drm_device *dev, void *data,
				  struct drm_file *file_priv);
extern uint32_t vmw_dmabuf_validate_node(struct ttm_buffer_object *bo,
					 uint32_t cur_validate_node);
extern void vmw_dmabuf_validate_clear(struct ttm_buffer_object *bo);
extern int vmw_user_dmabuf_lookup(struct ttm_object_file *tfile,
				  uint32_t id, struct vmw_dma_buffer **out);
extern int vmw_stream_claim_ioctl(struct drm_device *dev, void *data,
				  struct drm_file *file_priv);
extern int vmw_stream_unref_ioctl(struct drm_device *dev, void *data,
				  struct drm_file *file_priv);
extern int vmw_user_stream_lookup(struct vmw_private *dev_priv,
				  struct ttm_object_file *tfile,
				  uint32_t *inout_id,
				  struct vmw_resource **out);
extern void vmw_resource_unreserve(struct vmw_resource *res,
				   struct vmw_dma_buffer *new_backup,
				   unsigned long new_backup_offset);
extern void vmw_resource_move_notify(struct ttm_buffer_object *bo,
				     struct ttm_mem_reg *mem);
extern void vmw_fence_single_bo(struct ttm_buffer_object *bo,
				struct vmw_fence_obj *fence);
extern void vmw_resource_evict_all(struct vmw_private *dev_priv);

/**
 * DMA buffer helper routines - vmwgfx_dmabuf.c
 */
extern int vmw_dmabuf_to_placement(struct vmw_private *vmw_priv,
				   struct vmw_dma_buffer *bo,
				   struct ttm_placement *placement,
				   bool interruptible);
extern int vmw_dmabuf_to_vram(struct vmw_private *dev_priv,
			      struct vmw_dma_buffer *buf,
			      bool pin, bool interruptible);
extern int vmw_dmabuf_to_vram_or_gmr(struct vmw_private *dev_priv,
				     struct vmw_dma_buffer *buf,
				     bool pin, bool interruptible);
extern int vmw_dmabuf_to_start_of_vram(struct vmw_private *vmw_priv,
				       struct vmw_dma_buffer *bo,
				       bool pin, bool interruptible);
extern int vmw_dmabuf_unpin(struct vmw_private *vmw_priv,
			    struct vmw_dma_buffer *bo,
			    bool interruptible);
extern void vmw_bo_get_guest_ptr(const struct ttm_buffer_object *buf,
				 SVGAGuestPtr *ptr);
extern void vmw_bo_pin(struct ttm_buffer_object *bo, bool pin);

/**
 * Misc Ioctl functionality - vmwgfx_ioctl.c
 */

extern int vmw_getparam_ioctl(struct drm_device *dev, void *data,
			      struct drm_file *file_priv);
extern int vmw_get_cap_3d_ioctl(struct drm_device *dev, void *data,
				struct drm_file *file_priv);
extern int vmw_present_ioctl(struct drm_device *dev, void *data,
			     struct drm_file *file_priv);
extern int vmw_present_readback_ioctl(struct drm_device *dev, void *data,
				      struct drm_file *file_priv);
extern unsigned int vmw_fops_poll(struct file *filp,
				  struct poll_table_struct *wait);
extern ssize_t vmw_fops_read(struct file *filp, char __user *buffer,
			     size_t count, loff_t *offset);

/**
 * Fifo utilities - vmwgfx_fifo.c
 */

extern int vmw_fifo_init(struct vmw_private *dev_priv,
			 struct vmw_fifo_state *fifo);
extern void vmw_fifo_release(struct vmw_private *dev_priv,
			     struct vmw_fifo_state *fifo);
extern void *vmw_fifo_reserve(struct vmw_private *dev_priv, uint32_t bytes);
extern void vmw_fifo_commit(struct vmw_private *dev_priv, uint32_t bytes);
extern int vmw_fifo_send_fence(struct vmw_private *dev_priv,
			       uint32_t *seqno);
extern void vmw_fifo_ping_host(struct vmw_private *dev_priv, uint32_t reason);
extern bool vmw_fifo_have_3d(struct vmw_private *dev_priv);
extern bool vmw_fifo_have_pitchlock(struct vmw_private *dev_priv);
extern int vmw_fifo_emit_dummy_query(struct vmw_private *dev_priv,
				     uint32_t cid);

/**
 * TTM glue - vmwgfx_ttm_glue.c
 */

extern int vmw_ttm_global_init(struct vmw_private *dev_priv);
extern void vmw_ttm_global_release(struct vmw_private *dev_priv);
extern int vmw_mmap(struct file *filp, struct vm_area_struct *vma);

/**
 * TTM buffer object driver - vmwgfx_buffer.c
 */

extern struct ttm_placement vmw_vram_placement;
extern struct ttm_placement vmw_vram_ne_placement;
extern struct ttm_placement vmw_vram_sys_placement;
extern struct ttm_placement vmw_vram_gmr_placement;
extern struct ttm_placement vmw_vram_gmr_ne_placement;
extern struct ttm_placement vmw_sys_placement;
extern struct ttm_placement vmw_evictable_placement;
extern struct ttm_placement vmw_srf_placement;
extern struct ttm_bo_driver vmw_bo_driver;
extern int vmw_dma_quiescent(struct drm_device *dev);

/**
 * Command submission - vmwgfx_execbuf.c
 */

extern int vmw_execbuf_ioctl(struct drm_device *dev, void *data,
			     struct drm_file *file_priv);
extern int vmw_execbuf_process(struct drm_file *file_priv,
			       struct vmw_private *dev_priv,
			       void __user *user_commands,
			       void *kernel_commands,
			       uint32_t command_size,
			       uint64_t throttle_us,
			       struct drm_vmw_fence_rep __user
			       *user_fence_rep,
			       struct vmw_fence_obj **out_fence);
extern void __vmw_execbuf_release_pinned_bo(struct vmw_private *dev_priv,
					    struct vmw_fence_obj *fence);
extern void vmw_execbuf_release_pinned_bo(struct vmw_private *dev_priv);

extern int vmw_execbuf_fence_commands(struct drm_file *file_priv,
				      struct vmw_private *dev_priv,
				      struct vmw_fence_obj **p_fence,
				      uint32_t *p_handle);
extern void vmw_execbuf_copy_fence_user(struct vmw_private *dev_priv,
					struct vmw_fpriv *vmw_fp,
					int ret,
					struct drm_vmw_fence_rep __user
					*user_fence_rep,
					struct vmw_fence_obj *fence,
					uint32_t fence_handle);

/**
 * IRQs and wating - vmwgfx_irq.c
 */

extern irqreturn_t vmw_irq_handler(DRM_IRQ_ARGS);
extern int vmw_wait_seqno(struct vmw_private *dev_priv, bool lazy,
			     uint32_t seqno, bool interruptible,
			     unsigned long timeout);
extern void vmw_irq_preinstall(struct drm_device *dev);
extern int vmw_irq_postinstall(struct drm_device *dev);
extern void vmw_irq_uninstall(struct drm_device *dev);
extern bool vmw_seqno_passed(struct vmw_private *dev_priv,
				uint32_t seqno);
extern int vmw_fallback_wait(struct vmw_private *dev_priv,
			     bool lazy,
			     bool fifo_idle,
			     uint32_t seqno,
			     bool interruptible,
			     unsigned long timeout);
extern void vmw_update_seqno(struct vmw_private *dev_priv,
				struct vmw_fifo_state *fifo_state);
extern void vmw_seqno_waiter_add(struct vmw_private *dev_priv);
extern void vmw_seqno_waiter_remove(struct vmw_private *dev_priv);
extern void vmw_goal_waiter_add(struct vmw_private *dev_priv);
extern void vmw_goal_waiter_remove(struct vmw_private *dev_priv);

/**
 * Rudimentary fence-like objects currently used only for throttling -
 * vmwgfx_marker.c
 */

extern void vmw_marker_queue_init(struct vmw_marker_queue *queue);
extern void vmw_marker_queue_takedown(struct vmw_marker_queue *queue);
extern int vmw_marker_push(struct vmw_marker_queue *queue,
			  uint32_t seqno);
extern int vmw_marker_pull(struct vmw_marker_queue *queue,
			  uint32_t signaled_seqno);
extern int vmw_wait_lag(struct vmw_private *dev_priv,
			struct vmw_marker_queue *queue, uint32_t us);

/**
 * Kernel framebuffer - vmwgfx_fb.c
 */

int vmw_fb_init(struct vmw_private *vmw_priv);
int vmw_fb_close(struct vmw_private *dev_priv);
int vmw_fb_off(struct vmw_private *vmw_priv);
int vmw_fb_on(struct vmw_private *vmw_priv);

/**
 * Kernel modesetting - vmwgfx_kms.c
 */

int vmw_kms_init(struct vmw_private *dev_priv);
int vmw_kms_close(struct vmw_private *dev_priv);
int vmw_kms_save_vga(struct vmw_private *vmw_priv);
int vmw_kms_restore_vga(struct vmw_private *vmw_priv);
int vmw_kms_cursor_bypass_ioctl(struct drm_device *dev, void *data,
				struct drm_file *file_priv);
void vmw_kms_cursor_post_execbuf(struct vmw_private *dev_priv);
void vmw_kms_cursor_snoop(struct vmw_surface *srf,
			  struct ttm_object_file *tfile,
			  struct ttm_buffer_object *bo,
			  SVGA3dCmdHeader *header);
int vmw_kms_write_svga(struct vmw_private *vmw_priv,
		       unsigned width, unsigned height, unsigned pitch,
		       unsigned bpp, unsigned depth);
void vmw_kms_idle_workqueues(struct vmw_master *vmaster);
bool vmw_kms_validate_mode_vram(struct vmw_private *dev_priv,
				uint32_t pitch,
				uint32_t height);
u32 vmw_get_vblank_counter(struct drm_device *dev, int crtc);
int vmw_enable_vblank(struct drm_device *dev, int crtc);
void vmw_disable_vblank(struct drm_device *dev, int crtc);
int vmw_kms_present(struct vmw_private *dev_priv,
		    struct drm_file *file_priv,
		    struct vmw_framebuffer *vfb,
		    struct vmw_surface *surface,
		    uint32_t sid, int32_t destX, int32_t destY,
		    struct drm_vmw_rect *clips,
		    uint32_t num_clips);
int vmw_kms_readback(struct vmw_private *dev_priv,
		     struct drm_file *file_priv,
		     struct vmw_framebuffer *vfb,
		     struct drm_vmw_fence_rep __user *user_fence_rep,
		     struct drm_vmw_rect *clips,
		     uint32_t num_clips);
int vmw_kms_update_layout_ioctl(struct drm_device *dev, void *data,
				struct drm_file *file_priv);

int vmw_dumb_create(struct drm_file *file_priv,
		    struct drm_device *dev,
		    struct drm_mode_create_dumb *args);

int vmw_dumb_map_offset(struct drm_file *file_priv,
			struct drm_device *dev, uint32_t handle,
			uint64_t *offset);
int vmw_dumb_destroy(struct drm_file *file_priv,
		     struct drm_device *dev,
		     uint32_t handle);
/**
 * Overlay control - vmwgfx_overlay.c
 */

int vmw_overlay_init(struct vmw_private *dev_priv);
int vmw_overlay_close(struct vmw_private *dev_priv);
int vmw_overlay_ioctl(struct drm_device *dev, void *data,
		      struct drm_file *file_priv);
int vmw_overlay_stop_all(struct vmw_private *dev_priv);
int vmw_overlay_resume_all(struct vmw_private *dev_priv);
int vmw_overlay_pause_all(struct vmw_private *dev_priv);
int vmw_overlay_claim(struct vmw_private *dev_priv, uint32_t *out);
int vmw_overlay_unref(struct vmw_private *dev_priv, uint32_t stream_id);
int vmw_overlay_num_overlays(struct vmw_private *dev_priv);
int vmw_overlay_num_free_overlays(struct vmw_private *dev_priv);

/**
 * GMR Id manager
 */

extern const struct ttm_mem_type_manager_func vmw_gmrid_manager_func;

/**
 * Inline helper functions
 */

static inline void vmw_surface_unreference(struct vmw_surface **srf)
{
	struct vmw_surface *tmp_srf = *srf;
	struct vmw_resource *res = &tmp_srf->res;
	*srf = NULL;

	vmw_resource_unreference(&res);
}

static inline struct vmw_surface *vmw_surface_reference(struct vmw_surface *srf)
{
	(void) vmw_resource_reference(&srf->res);
	return srf;
}

static inline void vmw_dmabuf_unreference(struct vmw_dma_buffer **buf)
{
	struct vmw_dma_buffer *tmp_buf = *buf;

	*buf = NULL;
	if (tmp_buf != NULL) {
		struct ttm_buffer_object *bo = &tmp_buf->base;

		ttm_bo_unref(&bo);
	}
}

static inline struct vmw_dma_buffer *vmw_dmabuf_reference(struct vmw_dma_buffer *buf)
{
	if (ttm_bo_reference(&buf->base))
		return buf;
	return NULL;
}

static inline struct ttm_mem_global *vmw_mem_glob(struct vmw_private *dev_priv)
{
	return (struct ttm_mem_global *) dev_priv->mem_global_ref.object;
}
#endif
