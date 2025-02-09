#ifndef _FIKUS_FIRMWARE_H
#define _FIKUS_FIRMWARE_H

#include <fikus/types.h>
#include <fikus/compiler.h>
#include <fikus/gfp.h>

#define FW_ACTION_NOHOTPLUG 0
#define FW_ACTION_HOTPLUG 1

struct firmware {
	size_t size;
	const u8 *data;
	struct page **pages;

	/* firmware loader private fields */
	void *priv;
};

struct module;
struct device;

struct builtin_fw {
	char *name;
	void *data;
	unsigned long size;
};

/* We have to play tricks here much like stringify() to get the
   __COUNTER__ macro to be expanded as we want it */
#define __fw_concat1(x, y) x##y
#define __fw_concat(x, y) __fw_concat1(x, y)

#define DECLARE_BUILTIN_FIRMWARE(name, blob)				     \
	DECLARE_BUILTIN_FIRMWARE_SIZE(name, &(blob), sizeof(blob))

#define DECLARE_BUILTIN_FIRMWARE_SIZE(name, blob, size)			     \
	static const struct builtin_fw __fw_concat(__builtin_fw,__COUNTER__) \
	__used __section(.builtin_fw) = { name, blob, size }

#if defined(CONFIG_FW_LOADER) || (defined(CONFIG_FW_LOADER_MODULE) && defined(MODULE))
int request_firmware(const struct firmware **fw, const char *name,
		     struct device *device);
int request_firmware_nowait(
	struct module *module, bool uevent,
	const char *name, struct device *device, gfp_t gfp, void *context,
	void (*cont)(const struct firmware *fw, void *context));

void release_firmware(const struct firmware *fw);
#else
static inline int request_firmware(const struct firmware **fw,
				   const char *name,
				   struct device *device)
{
	return -EINVAL;
}
static inline int request_firmware_nowait(
	struct module *module, bool uevent,
	const char *name, struct device *device, gfp_t gfp, void *context,
	void (*cont)(const struct firmware *fw, void *context))
{
	return -EINVAL;
}

static inline void release_firmware(const struct firmware *fw)
{
}

#endif

#ifndef _FIKUS_LIBRE_FIRMWARE_H
#define _FIKUS_LIBRE_FIRMWARE_H

#include <fikus/device.h>

#define NONFREE_FIRMWARE "/*(DEBLOBBED)*/"

static inline int
report_missing_free_firmware(const char *name, const char *what)
{
	printk(KERN_ERR "%s: Missing Free %s\n", name,
	       what ? what : "firmware");
	return -EINVAL;
}
static inline int
reject_firmware(const struct firmware **fw,
		const char *name, struct device *device)
{
	const struct firmware *xfw = NULL;
	int retval;
	report_missing_free_firmware(dev_name(device), NULL);
	retval = request_firmware(&xfw, NONFREE_FIRMWARE, device);
	if (!retval)
		release_firmware(xfw);
	return -EINVAL;
}
static inline int
maybe_reject_firmware(const struct firmware **fw,
		      const char *name, struct device *device)
{
	if (strstr (name, NONFREE_FIRMWARE))
		return reject_firmware(fw, name, device);
	else
		return request_firmware(fw, name, device);
}
static inline void
discard_rejected_firmware(const struct firmware *fw, void *context)
{
	release_firmware(fw);
}
static inline int
reject_firmware_nowait(struct module *module, int uevent,
		       const char *name, struct device *device,
		       gfp_t gfp, void *context,
		       void (*cont)(const struct firmware *fw,
				    void *context))
{
	int retval;
	report_missing_free_firmware(dev_name(device), NULL);
	retval = request_firmware_nowait(module, uevent, NONFREE_FIRMWARE,
					 device, gfp, NULL,
					 discard_rejected_firmware);
	if (retval)
		return retval;
	return -EINVAL;
}
static inline int
maybe_reject_firmware_nowait(struct module *module, int uevent,
			     const char *name, struct device *device,
			     gfp_t gfp, void *context,
			     void (*cont)(const struct firmware *fw,
					  void *context))
{
	if (strstr (name, NONFREE_FIRMWARE))
		return reject_firmware_nowait(module, uevent, name,
					      device, gfp, context, cont);
	else
		return request_firmware_nowait(module, uevent, name,
					       device, gfp, context, cont);
}

#endif /* _FIKUS_LIBRE_FIRMWARE_H */

#endif
