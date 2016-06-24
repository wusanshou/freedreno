/*
 * Copyright © 2012 Rob Clark <robclark@freedesktop.org>
 *
 * Permission is hereby granted, free of charge, to any person obtaining a
 * copy of this software and associated documentation files (the "Software"),
 * to deal in the Software without restriction, including without limitation
 * the rights to use, copy, modify, merge, publish, distribute, sublicense,
 * and/or sell copies of the Software, and to permit persons to whom the
 * Software is furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice (including the next
 * paragraph) shall be included in all copies or substantial portions of the
 * Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.  IN NO EVENT SHALL
 * THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
 * SOFTWARE.
 */

/* bits and pieces borrowed from lima project.. concept is the same, wrap
 * various syscalls and log what happens
 */

#ifdef BIONIC
#  define assert(X)
#else
#  include <assert.h>
#endif

#include "wrap.h"

#ifdef USE_PTHREADS
static pthread_mutex_t l = PTHREAD_RECURSIVE_MUTEX_INITIALIZER;
#endif

struct device_info {
	const char *name;
	struct {
		const char *name;
	} ioctl_info[_IOC_NR(0xffffffff)];
};

#define IOCTL_INFO(n) \
		[_IOC_NR(n)] = { .name = #n }

static struct device_info kgsl_3d_info = {
		.name = "kgsl-3d",
		.ioctl_info = {
				IOCTL_INFO(IOCTL_KGSL_DEVICE_GETPROPERTY),
				IOCTL_INFO(IOCTL_KGSL_DEVICE_WAITTIMESTAMP),
				IOCTL_INFO(IOCTL_KGSL_DEVICE_WAITTIMESTAMP_CTXTID),
				IOCTL_INFO(IOCTL_KGSL_RINGBUFFER_ISSUEIBCMDS),
				IOCTL_INFO(IOCTL_KGSL_CMDSTREAM_READTIMESTAMP),
				IOCTL_INFO(IOCTL_KGSL_CMDSTREAM_FREEMEMONTIMESTAMP),
				IOCTL_INFO(IOCTL_KGSL_DRAWCTXT_CREATE),
				IOCTL_INFO(IOCTL_KGSL_DRAWCTXT_DESTROY),
				IOCTL_INFO(IOCTL_KGSL_MAP_USER_MEM),
				IOCTL_INFO(IOCTL_KGSL_SHAREDMEM_FROM_PMEM),
				IOCTL_INFO(IOCTL_KGSL_SHAREDMEM_FREE),
				IOCTL_INFO(IOCTL_KGSL_SHAREDMEM_FROM_VMALLOC),
				IOCTL_INFO(IOCTL_KGSL_SHAREDMEM_FLUSH_CACHE),
				IOCTL_INFO(IOCTL_KGSL_GPUMEM_ALLOC),
				IOCTL_INFO(IOCTL_KGSL_CFF_SYNCMEM),
				IOCTL_INFO(IOCTL_KGSL_CFF_USER_EVENT),
				IOCTL_INFO(IOCTL_KGSL_TIMESTAMP_EVENT),
				IOCTL_INFO(IOCTL_KGSL_GPUMEM_ALLOC_ID),
				IOCTL_INFO(IOCTL_KGSL_GPUMEM_FREE_ID),
				IOCTL_INFO(IOCTL_KGSL_PERFCOUNTER_GET),
				IOCTL_INFO(IOCTL_KGSL_PERFCOUNTER_PUT),
				/* kgsl-3d specific ioctls: */
				IOCTL_INFO(IOCTL_KGSL_DRAWCTXT_SET_BIN_BASE_OFFSET),
				IOCTL_INFO(IOCTL_KGSL_SUBMIT_COMMANDS),
		},
};

// kgsl-2d => Z180 vector graphcis core.. not sure if it is interesting..
static struct device_info kgsl_2d_info = {
		.name = "kgsl-2d",
		.ioctl_info = {
				IOCTL_INFO(IOCTL_KGSL_DEVICE_GETPROPERTY),
				IOCTL_INFO(IOCTL_KGSL_DEVICE_WAITTIMESTAMP),
				IOCTL_INFO(IOCTL_KGSL_DEVICE_WAITTIMESTAMP_CTXTID),
				IOCTL_INFO(IOCTL_KGSL_RINGBUFFER_ISSUEIBCMDS),
				IOCTL_INFO(IOCTL_KGSL_CMDSTREAM_READTIMESTAMP),
				IOCTL_INFO(IOCTL_KGSL_CMDSTREAM_FREEMEMONTIMESTAMP),
				IOCTL_INFO(IOCTL_KGSL_DRAWCTXT_CREATE),
				IOCTL_INFO(IOCTL_KGSL_DRAWCTXT_DESTROY),
				IOCTL_INFO(IOCTL_KGSL_MAP_USER_MEM),
				IOCTL_INFO(IOCTL_KGSL_SHAREDMEM_FROM_PMEM),
				IOCTL_INFO(IOCTL_KGSL_SHAREDMEM_FREE),
				IOCTL_INFO(IOCTL_KGSL_SHAREDMEM_FROM_VMALLOC),
				IOCTL_INFO(IOCTL_KGSL_SHAREDMEM_FLUSH_CACHE),
				IOCTL_INFO(IOCTL_KGSL_GPUMEM_ALLOC),
				IOCTL_INFO(IOCTL_KGSL_CFF_SYNCMEM),
				IOCTL_INFO(IOCTL_KGSL_CFF_USER_EVENT),
				IOCTL_INFO(IOCTL_KGSL_TIMESTAMP_EVENT),
				IOCTL_INFO(IOCTL_KGSL_GPUMEM_ALLOC_ID),
				IOCTL_INFO(IOCTL_KGSL_GPUMEM_FREE_ID),
				/* no kgsl-2d specific ioctls, I don't think.. */
		},
};

static struct device_info drm_info = {
		.name = "drm",
		.ioctl_info = {
				IOCTL_INFO(DRM_IOCTL_VERSION),
				IOCTL_INFO(DRM_IOCTL_GET_UNIQUE),
				IOCTL_INFO(DRM_IOCTL_GET_MAGIC),
				IOCTL_INFO(DRM_IOCTL_IRQ_BUSID),
				IOCTL_INFO(DRM_IOCTL_GET_MAP),
				IOCTL_INFO(DRM_IOCTL_GET_CLIENT),
				IOCTL_INFO(DRM_IOCTL_GET_STATS),
				IOCTL_INFO(DRM_IOCTL_SET_VERSION),
				IOCTL_INFO(DRM_IOCTL_MODESET_CTL),
				IOCTL_INFO(DRM_IOCTL_GEM_CLOSE),
				IOCTL_INFO(DRM_IOCTL_GEM_FLINK),
				IOCTL_INFO(DRM_IOCTL_GEM_OPEN),
				IOCTL_INFO(DRM_IOCTL_GET_CAP),
				IOCTL_INFO(DRM_IOCTL_SET_UNIQUE),
				IOCTL_INFO(DRM_IOCTL_AUTH_MAGIC),
				IOCTL_INFO(DRM_IOCTL_SET_MASTER),
				IOCTL_INFO(DRM_IOCTL_DROP_MASTER),
				IOCTL_INFO(DRM_IOCTL_KGSL_GEM_CREATE),
				IOCTL_INFO(DRM_IOCTL_KGSL_GEM_PREP),
				IOCTL_INFO(DRM_IOCTL_KGSL_GEM_SETMEMTYPE),
				IOCTL_INFO(DRM_IOCTL_KGSL_GEM_GETMEMTYPE),
				IOCTL_INFO(DRM_IOCTL_KGSL_GEM_MMAP),
				IOCTL_INFO(DRM_IOCTL_KGSL_GEM_ALLOC),
				IOCTL_INFO(DRM_IOCTL_KGSL_GEM_BIND_GPU),
				IOCTL_INFO(DRM_IOCTL_KGSL_GEM_UNBIND_GPU),
				IOCTL_INFO(DRM_IOCTL_KGSL_GEM_GET_BUFINFO),
				IOCTL_INFO(DRM_IOCTL_KGSL_GEM_SET_BUFCOUNT),
				IOCTL_INFO(DRM_IOCTL_KGSL_GEM_SET_ACTIVE),
				IOCTL_INFO(DRM_IOCTL_KGSL_GEM_LOCK_HANDLE),
				IOCTL_INFO(DRM_IOCTL_KGSL_GEM_UNLOCK_HANDLE),
				IOCTL_INFO(DRM_IOCTL_KGSL_GEM_UNLOCK_ON_TS),
				IOCTL_INFO(DRM_IOCTL_KGSL_GEM_CREATE_FD),
		},
};

static struct {
	int is_3d, is_2d, is_drm;
	int is_emulated;
} file_table[1024];

static int is_drm(int fd)
{
	if (fd >= ARRAY_SIZE(file_table))
		return 0;
	return file_table[fd].is_drm;
}

static struct device_info * get_kgsl_info(int fd)
{
	if (fd >= ARRAY_SIZE(file_table))
		return NULL;
	if (file_table[fd].is_2d)
		return &kgsl_2d_info;
	else if (file_table[fd].is_3d)
		return &kgsl_3d_info;
	return NULL;
}

void
hexdump(const void *data, int size)
{
	unsigned char *buf = (void *) data;
	char alpha[17];
	int i;

	for (i = 0; i < size; i++) {
		if (!(i % 16))
			printf("\t\t\t%08X", (unsigned int) i);
		if (!(i % 4))
			printf(" ");

		if (((void *) (buf + i)) < ((void *) data)) {
			printf("   ");
			alpha[i % 16] = '.';
		} else {
			printf(" %02x", buf[i]);

			if (isprint(buf[i]) && (buf[i] < 0xA0))
				alpha[i % 16] = buf[i];
			else
				alpha[i % 16] = '.';
		}

		if ((i % 16) == 15) {
			alpha[16] = 0;
			printf("\t|%s|\n", alpha);
		}
	}

	if (i % 16) {
		for (i %= 16; i < 16; i++) {
			printf("   ");
			alpha[i] = '.';

			if (i == 15) {
				alpha[16] = 0;
				printf("\t|%s|\n", alpha);
			}
		}
	}
}


void
hexdump_dwords(const void *data, int sizedwords)
{
	uint32_t *buf = (void *) data;
	int i;

	for (i = 0; i < sizedwords; i++) {
		if (!(i % 8))
			printf("\t\t\t%08X:   ", (unsigned int) i*4);
		printf(" %08x", buf[i]);
		if ((i % 8) == 7)
			printf("\n");
	}

	if (i % 8)
		printf("\n");
}


static void dump_ioctl(struct device_info *info, int dir, int fd,
		unsigned long int request, void *ptr, int ret)
{
	int nr = _IOC_NR(request);
	int sz = _IOC_SIZE(request);
	char c;
	const char *name;

	if (dir == _IOC_READ)
		c = '<';
	else
		c = '>';

	if (info->ioctl_info[nr].name)
		name = info->ioctl_info[nr].name;
	else
		name = "<unknown>";

	printf("%c [%4d] %8s: %s (%08lx)", c, fd, info->name, name, request);
	if (dir == _IOC_READ)
		printf(" => %d", ret);
	printf("\n");

	if (dir & _IOC_DIR(request))
		hexdump(ptr, sz);
}

static void dumpfile(const char *file)
{
	char buf[1024];
	int fd = open(file, 0);
	int n;

	while ((n = read(fd, buf, 1024)) > 0)
		write(1, buf, n);
	close(fd);
}

struct buffer {
	void *hostptr;
	unsigned int gpuaddr, flags, len, handle, id;
	uint64_t offset;
	struct list node;
	int munmap;
	int dumped;
};

static LIST_HEAD(buffers_of_interest);

static struct buffer * register_buffer(void *hostptr, unsigned int flags,
		unsigned int len, unsigned int handle)
{
	struct buffer *buf = calloc(1, sizeof *buf);
	buf->hostptr = hostptr;
	buf->flags = flags;
	buf->len = len;
	buf->handle = handle;
	list_add(&buf->node, &buffers_of_interest);
	return buf;
}

static struct buffer * find_buffer(void *hostptr, unsigned int gpuaddr,
		uint64_t offset, unsigned int handle, unsigned id)
{
	struct buffer *buf = NULL;
	list_for_each_entry(buf, &buffers_of_interest, node) {
		if (hostptr)
			if ((buf->hostptr <= hostptr) && (hostptr < (buf->hostptr + buf->len)))
				return buf;
		if (gpuaddr)
			if ((buf->gpuaddr <= gpuaddr) && (gpuaddr < (buf->gpuaddr + buf->len)))
				return buf;
		if (offset)
			if ((buf->offset <= offset) && (offset < (buf->offset + buf->len)))
				return buf;
		if (handle)
			if (buf->handle == handle)
				return buf;
		if (id)
			if (buf->id == id)
				return buf;
	}
	return NULL;
}

static void unregister_buffer(struct buffer *buf)
{
	if (buf) {
		list_del(&buf->node);
		if (buf->munmap)
			munmap(buf->hostptr, buf->len);
		free(buf);
	}
}

static void dump_buffer(unsigned int gpuaddr)
{
	static int cnt = 0;
	struct buffer *buf = find_buffer((void *)-1, gpuaddr, 0, 0, 0);
	if (buf) {
		char filename[32];
		int fd;
		sprintf(filename, "%04d-%08x.dat", cnt, buf->gpuaddr);
		printf("\t\tdumping: %s\n", filename);
		fd = open(filename, O_WRONLY| O_TRUNC | O_CREAT, 0644);
		write(fd, buf->hostptr, buf->len);
		close(fd);
		cnt++;
	}
}

uint32_t alloc_gpuaddr(uint32_t size)
{
	// TODO need better scheme to deal w/ deallocation..
	static uint32_t gpuaddr = 0xc0000000;
	uint32_t addr = gpuaddr;
	gpuaddr += size;
	return addr;
}

uint32_t alloc_gpuaddr(uint32_t size)
{
	// TODO need better scheme to deal w/ deallocation..
	static uint32_t gpuaddr = 0xc0000000;
	uint32_t addr = gpuaddr;
	gpuaddr += size;
	return addr;
}

/*****************************************************************************/

int open(const char* path, int flags, ...)
{
	mode_t mode = 0;
	int ret;
	PROLOG(open);

	if (flags & O_CREAT) {
		va_list args;

		va_start(args, flags);
		mode = (mode_t) va_arg(args, int);
		va_end(args);

		ret = orig_open(path, flags, mode);
	} else {
		const char *actual_path = path;
		if (access(path, F_OK) && (path == strstr(path, "/dev/"))) {
			/* fake non-existant device files: */
			printf("emulating: %s\n", path);
			actual_path = "/dev/null";
		}
		ret = orig_open(actual_path, flags);
		if ((ret != -1) && (path != actual_path)) {
			assert(ret < ARRAY_SIZE(file_table));
			file_table[ret].is_emulated = 1;
		}
	}

	if (ret != -1) {
		assert(ret < ARRAY_SIZE(file_table));
		if (!strcmp(path, "/dev/kgsl-3d0")) {
			if (!(wrap_gpu_id() && wrap_gmem_size())) {
				printf("need WRAP_GPU_ID/WRAP_GMEM_SIZE!\n");
				return -1;
			}
			file_table[ret].is_3d = 1;
			printf("found kgsl_3d0: %d\n", ret);
		} else if (!strcmp(path, "/dev/kgsl-2d0")) {
			file_table[ret].is_2d = 1;
			printf("found kgsl_2d0: %d\n", ret);
		} else if (!strcmp(path, "/dev/kgsl-2d1")) {
			file_table[ret].is_2d = 1;
			printf("found kgsl_2d1: %d\n", ret);
		} else if (!strcmp(path, "/dev/dri/card0")) {
			file_table[ret].is_drm = 1;
			printf("found drm: %d\n", ret);
		} else if (strstr(path, "/dev/")) {
			printf("#### missing device, path: %s: %d\n", path, ret);
		}
	}

	return ret;
}

int close(int fd)
{
	PROLOG(close);

	if (fd < ARRAY_SIZE(file_table)) {
		file_table[fd].is_3d = 0;
		file_table[fd].is_2d = 0;
		file_table[fd].is_drm = 0;
	}

	return orig_close(fd);
}

int drmOpen(const char *name, const char *busid)
{
	/* quick hack to deal w/ opening drm device via libdrm */
	return open("/dev/dri/card0", O_RDWR, 0);
}

static void log_gpuaddr(uint32_t gpuaddr, uint32_t len)
{
	uint32_t sect[2] = {
			gpuaddr, len
	};
	rd_write_section(RD_GPUADDR, sect, sizeof(sect));
}


static void dump_ib_prep(void)
{
	struct buffer *other_buf;

	list_for_each_entry(other_buf, &buffers_of_interest, node) {
		other_buf->dumped = 0;
	}
}

static void dump_ib(struct kgsl_ibdesc *ibdesc)
{
	struct buffer *buf = find_buffer(NULL, ibdesc->gpuaddr, 0, 0, 0);
	if (buf && buf->hostptr) {
		struct buffer *other_buf;
		uint32_t off = ibdesc->gpuaddr - buf->gpuaddr;
		uint32_t *ptr = buf->hostptr + off;

		printf("\t\tcmd: (%u dwords)\n", ibdesc->sizedwords);

		hexdump_dwords(ptr, ibdesc->sizedwords);

		list_for_each_entry(other_buf, &buffers_of_interest, node) {
			if (other_buf && other_buf->hostptr && !other_buf->dumped) {
				uint32_t len = other_buf->len;
				log_gpuaddr(other_buf->gpuaddr, len);
				rd_write_section(RD_BUFFER_CONTENTS,
						other_buf->hostptr, len);
				other_buf->dumped = 1;
			}
		}

		/* we already dump all the buffer contents, so just need
		 * to dump the address/size of the cmdstream:
		 */
		rd_write_section(RD_CMDSTREAM_ADDR, (uint32_t[2]) {
			ibdesc->gpuaddr, ibdesc->sizedwords,
		}, 8);
	}
}

static void kgsl_ioctl_ringbuffer_issueibcmds_pre(int fd,
		struct kgsl_ringbuffer_issueibcmds *param)
{
	int is2d = get_kgsl_info(fd) == &kgsl_2d_info;
	int i;
	struct kgsl_ibdesc *ibdesc;
	dump_ib_prep();
	printf("\t\tdrawctxt_id:\t%08x\n", param->drawctxt_id);
	/*
For z180_cmdstream_issueibcmds():

#define KGSL_CONTEXT_SAVE_GMEM	1
#define KGSL_CONTEXT_NO_GMEM_ALLOC	2
#define KGSL_CONTEXT_SUBMIT_IB_LIST	4
#define KGSL_CONTEXT_CTX_SWITCH	8
#define KGSL_CONTEXT_PREAMBLE	16

#define Z180_STREAM_PACKET_CALL 0x7C000275   <-- seems to be always first 4 bytes..

if there isn't a context switch, skip the first PACKETSIZE_STATESTREAM words:

PACKETSIZE_STATE:
	#define NUMTEXUNITS             4
	#define TEXUNITREGCOUNT         25
	#define VG_REGCOUNT             0x39

	#define PACKETSIZE_BEGIN        3
	#define PACKETSIZE_G2DCOLOR     2
	#define PACKETSIZE_TEXUNIT      (TEXUNITREGCOUNT * 2)
	#define PACKETSIZE_REG          (VG_REGCOUNT * 2)
	#define PACKETSIZE_STATE        (PACKETSIZE_TEXUNIT * NUMTEXUNITS + \
					 PACKETSIZE_REG + PACKETSIZE_BEGIN + \
					 PACKETSIZE_G2DCOLOR)

		((25 * 2) * 4 + (0x39 * 2) + 3 + 2) =>
		((25 * 2) * 4 + (57 * 2) + 3 + 2) =>
		319

PACKETSIZE_STATESTREAM:
	#define x  (ALIGN((PACKETSIZE_STATE * \
					 sizeof(unsigned int)), 32) / \
					 sizeof(unsigned int))

	ALIGN((PACKETSIZE_STATE * sizeof(unsigned int)), 32) / sizeof(unsigned int) =>
	1280 / 4 =>
	320 => 0x140

so the context, restored on context switch, is the first: 320 (0x140) words
	*/
	printf("\t\tflags:\t\t%08x\n", param->flags);
	printf("\t\tnumibs:\t\t%08x\n", param->numibs);
	printf("\t\tibdesc_addr:\t%08x\n", param->ibdesc_addr);
	ibdesc = (struct kgsl_ibdesc *)param->ibdesc_addr;
	for (i = 0; i < param->numibs; i++) {
		// z180_cmdstream_issueibcmds or adreno_ringbuffer_issueibcmds
		printf("\t\tibdesc[%d].ctrl:\t\t%08x\n", i, ibdesc[i].ctrl);
		printf("\t\tibdesc[%d].sizedwords:\t%08x\n", i, ibdesc[i].sizedwords);
		printf("\t\tibdesc[%d].gpuaddr:\t%08x\n", i, ibdesc[i].gpuaddr);
		printf("\t\tibdesc[%d].hostptr:\t%p\n", i, ibdesc[i].hostptr);
		if (is2d) {
			if (ibdesc[i].sizedwords > PACKETSIZE_STATESTREAM) {
				unsigned int len, *ptr;
				/* note: kernel side seems to expect param->timestamp to
				 * contain same thing as ibdesc[0].hostptr ... this seems to
				 * be what actually gets read from on kernel side.  Maybe a
				 * legacy thing??
				 * Update: this seems to be needed so z180_cmdstream_issueibcmds()
				 * can patch up the cmdstream to jump back to the next ringbuffer
				 * entry.
				 */
				printf("\t\tcontext:\n");
				hexdump_dwords(ibdesc[i].hostptr, PACKETSIZE_STATESTREAM);
				rd_write_section(RD_CONTEXT, ibdesc[i].hostptr,
						PACKETSIZE_STATESTREAM * sizeof(unsigned int));

				printf("\t\tcmd:\n");
				ptr = (unsigned int *)(ibdesc[i].hostptr +
						PACKETSIZE_STATESTREAM * sizeof(unsigned int));
				len = ptr[2] & 0xfff;
				/* 5 is length of first packet, 2 for the two 7f000000's */
				hexdump_dwords(ptr, len + 5 + 2);
				rd_write_section(RD_CMDSTREAM, ptr,
						(len + 5 + 2) * sizeof(unsigned int));
				/* dump out full buffer in case I need to go back and check
				 * if I missed something..
				 */
				dump_buffer(ibdesc[i].gpuaddr);
			} else {
				printf("\t\tWARNING: INVALID CONTEXT!\n");
				hexdump_dwords(ibdesc[i].hostptr, ibdesc[i].sizedwords);
			}
		} else {
			dump_ib(&ibdesc[i]);
		}
	}
}

static void kgsl_ioctl_ringbuffer_issueibcmds_post(int fd,
		struct kgsl_ringbuffer_issueibcmds *param)
{
	printf("\t\ttimestamp:\t%08x\n", param->timestamp);
}

static void kgsl_ioctl_submit_commands_pre(int fd,
		struct kgsl_submit_commands *param)
{
	int i;
	struct kgsl_ibdesc *ibdesc;

	dump_ib_prep();

	ibdesc = (struct kgsl_ibdesc *)param->cmdlist;

	printf("\t\tdrawctxt_id:\t%08x\n", param->context_id);
	printf("\t\tflags:\t\t%08x\n", param->flags);
	printf("\t\tnumibs:\t\t%08x\n", param->numcmds);
	for (i = 0; i < param->numcmds; i++) {
		printf("\t\tibdesc[%d].ctrl:\t\t%08x\n", i, ibdesc[i].ctrl);
		printf("\t\tibdesc[%d].sizedwords:\t%08x\n", i, ibdesc[i].sizedwords);
		printf("\t\tibdesc[%d].gpuaddr:\t%08x\n", i, ibdesc[i].gpuaddr);
		printf("\t\tibdesc[%d].hostptr:\t%p\n", i, ibdesc[i].hostptr);
		dump_ib(&ibdesc[i]);
	}
}

static void kgsl_ioctl_submit_commands_post(int fd,
		struct kgsl_submit_commands *param)
{
	printf("\t\ttimestamp:\t%08x\n", param->timestamp);
}

static void kgsl_ioctl_drawctxt_create_pre(int fd,
		struct kgsl_drawctxt_create *param)
{
	printf("\t\tflags:\t\t%08x\n", param->flags);
}

static void kgsl_ioctl_drawctxt_create_post(int fd,
		struct kgsl_drawctxt_create *param)
{
	static unsigned ctxid = 0;
	param->drawctxt_id = ++ctxid;
	printf("\t\tdrawctxt_id:\t%08x\n", param->drawctxt_id);
}

#define PROP_INFO(n) [n] = #n
static const char *propnames[] = {
		PROP_INFO(KGSL_PROP_DEVICE_INFO),
		PROP_INFO(KGSL_PROP_DEVICE_SHADOW),
		PROP_INFO(KGSL_PROP_DEVICE_POWER),
		PROP_INFO(KGSL_PROP_SHMEM),
		PROP_INFO(KGSL_PROP_SHMEM_APERTURES),
		PROP_INFO(KGSL_PROP_MMU_ENABLE),
		PROP_INFO(KGSL_PROP_INTERRUPT_WAITS),
		PROP_INFO(KGSL_PROP_VERSION),
		PROP_INFO(KGSL_PROP_GPU_RESET_STAT),
};

static void kgsl_ioctl_device_getproperty_post(int fd,
		struct kgsl_device_getproperty *param)
{
	printf("\t\ttype:\t\t%08x (%s)\n", param->type, propnames[param->type]);
	if (param->type == KGSL_PROP_DEVICE_INFO) {
		struct kgsl_devinfo *devinfo = param->value;
		uint32_t gpu_id = wrap_gpu_id();
		/* convert gpu-id into chip-id, and add optional patch level: */
		unsigned core  = gpu_id / 100;
		unsigned major = (gpu_id % 100) / 10;
		unsigned minor = gpu_id % 10;
		int patch = wrap_gpu_id_patchid();

		devinfo->device_id = 1;
		devinfo->chip_id = (patch & 0xff) |
				((minor & 0xff) << 8) |
				((major & 0xff) << 16) |
				((core & 0xff) << 24);
		devinfo->mmu_enabled = 1;
		devinfo->gmem_gpubaseaddr = 0;
		devinfo->gpu_id = gpu_id;
		devinfo->gmem_sizebytes = wrap_gmem_size();

		printf("\t\tEMULATING gpu_id: %d (%08x)!!!\n",
			devinfo->gpu_id, devinfo->chip_id);
		printf("\t\tEMULATING gmem_sizebytes: 0x%x !!!\n", devinfo->gmem_sizebytes);

		rd_write_section(RD_GPU_ID, &devinfo->gpu_id, sizeof(devinfo->gpu_id));

		printf("\t\tgpu_id: %d\n", devinfo->gpu_id);
		printf("\t\tgmem_sizebytes: 0x%x\n", devinfo->gmem_sizebytes);
	} else if (param->type == KGSL_PROP_DEVICE_SHADOW) {
		struct kgsl_shadowprop *shadow = param->value;
		shadow->gpuaddr = 0xc0009000;
		shadow->size = 0x2000;
		shadow->flags = 0x00000204;
	}
	hexdump(param->value, param->sizebytes);
}

static int len_from_vma(unsigned int hostptr)
{
	long long addr, endaddr, offset, inode;
	FILE *f;
	int ret;

	// TODO: only for debug..
	if (0)
		dumpfile("/proc/self/maps");

	f = fopen("/proc/self/maps", "r");

	do {
		char c;
		ret = fscanf(f, "%llx-%llx", &addr, &endaddr);
		if (addr == hostptr)
			return endaddr - addr;
		/* find end of line.. we could do this more cleverly w/ glibc.. :-( */
		while (((ret = fscanf(f, "%c", &c)) > 0) && (c != '\n'));
	} while (ret > 0);
	return -1;
}

static void kgsl_ioctl_sharedmem_from_vmalloc_pre(int fd,
		struct kgsl_sharedmem_from_vmalloc *param)
{
	int len;

	/* just make gpuaddr == hostptr.. should make it easy to track */
	printf("\t\tflags:\t\t%08x\n", param->flags);
	printf("\t\thostptr:\t%08x\n", param->hostptr);
	if (param->gpuaddr) {
		len = param->gpuaddr;
	} else {
		/* note: if gpuaddr not specified, need to figure out length from
		 * vma.. that is nasty!
		 */
		len = len_from_vma(param->hostptr);
	}

	/* register buffer of interest */
	register_buffer((void *)param->hostptr, param->flags, len, 0);

	param->gpuaddr = alloc_gpuaddr(len);
	printf("\t\tlen:\t\t%08x\n", len);
}

static void kgsl_ioctl_sharedmem_from_vmalloc_post(int fd,
		struct kgsl_sharedmem_from_vmalloc *param)
{
	struct buffer *buf = find_buffer((void *)param->hostptr, 0, 0, 0, 0);
	log_gpuaddr(param->gpuaddr, len_from_vma(param->hostptr));
	if (buf)
		buf->gpuaddr = param->gpuaddr;
	printf("\t\tgpuaddr:\t%08x\n", param->gpuaddr);
}

static void kgsl_ioctl_sharedmem_free_pre(int fd,
		struct kgsl_sharedmem_free *param)
{
	struct buffer *buf = find_buffer((void *)-1, param->gpuaddr, 0, 0, 0);
	printf("\t\tgpuaddr:\t%08x\n", param->gpuaddr);
	unregister_buffer(buf);
}

static void kgsl_ioctl_gpumem_alloc_pre(int fd,
		struct kgsl_gpumem_alloc *param)
{
	printf("\t\tflags:\t\t%08x\n", param->flags);
	printf("\t\tsize:\t\t%08x\n", param->size);
}

static void kgsl_ioctl_gpumem_alloc_post(int fd,
		struct kgsl_gpumem_alloc *param)
{
	struct buffer *buf;
	log_gpuaddr(param->gpuaddr, param->size);
	printf("\t\tgpuaddr:\t%08lx\n", param->gpuaddr);
	/* NOTE: host addr comes from mmap'ing w/ gpuaddr as offset */
	buf = register_buffer(NULL, param->flags, param->size, 0);
	buf->gpuaddr = param->gpuaddr;
	buf->offset = param->gpuaddr;
}

static void kgsl_ioctl_gpumem_alloc_id_pre(int fd,
		struct kgsl_gpumem_alloc_id *param)
{
	printf("\t\tflags:\t\t%08x\n", param->flags);
	printf("\t\tsize:\t\t%08x\n", param->size);
	/* easier to force it not to USE_CPU_MAP than dealing with
	 * the mmap dance:
	 */
	param->flags &= ~KGSL_MEMFLAGS_USE_CPU_MAP;
}

#define ALIGN(_v, _d) (((_v) + ((_d) - 1)) & ~((_d) - 1))

static void kgsl_ioctl_gpumem_alloc_id_post(int fd,
		struct kgsl_gpumem_alloc_id *param)
{
	struct buffer *buf;
	static int id = 0;

	param->id = ++id;
	param->mmapsize = ALIGN(param->size, 0x1000);
	param->gpuaddr = alloc_gpuaddr(param->mmapsize);

	log_gpuaddr(param->gpuaddr, param->size);
	printf("\t\tid:\t%u\n", param->id);
	printf("\t\tgpuaddr:\t%08lx\n", param->gpuaddr);
	/* NOTE: host addr comes from mmap'ing w/ gpuaddr as offset */
	buf = register_buffer(NULL, param->flags, param->size, 0);
	buf->id = param->id;
	buf->gpuaddr = param->gpuaddr;
	buf->offset = param->gpuaddr;
}

static void kgsl_ioctl_gpumem_free_id_pre(int fd,
		struct kgsl_gpumem_free_id *param)
{
	printf("\t\tid:\t%u\n", param->id);
}

static void kgsl_ioctl_gpumem_free_id_post(int fd,
		struct kgsl_gpumem_free_id *param)
{
	struct buffer *buf = find_buffer((void *)-1, 0, 0, 0, param->id);
	unregister_buffer(buf);
}

static void kgls_ioctl_perfcounter_get_post(int fd,
		struct kgsl_perfcounter_get *param)
{
	static struct {
		uint32_t hi, lo;
	} cache[10][10];
	char buf[128];
	if (cache[param->groupid][param->countable].lo == 0) {
		static int off = 0x9c; // REG_A4XX_RBBM_PERFCTR_CP_0_LO
		cache[param->groupid][param->countable].lo = off++;
		cache[param->groupid][param->countable].hi = off++;
	}
	param->offset = cache[param->groupid][param->countable].lo;
	param->offset_hi = cache[param->groupid][param->countable].hi;
	printf("\t\tgroupid:\t\t%u\n", param->groupid);
	printf("\t\tcountable:\t\t%u\n", param->countable);
	printf("\t\toffset_lo: 0x%x\n", param->offset);
	printf("\t\toffset_hi: 0x%x\n", param->offset_hi);

	rd_write_section(RD_CMD, buf, snprintf(buf, sizeof(buf),
			"perfcounter_get: groupid=%u, countable=%u, off_lo=0x%x, off_hi=0x%x",
			param->groupid, param->countable, param->offset, param->offset_hi));
}

static void kgls_ioctl_perfcounter_put_pre(int fd,
		struct kgsl_perfcounter_put *param)
{
	char buf[128];

	printf("\t\tgroupid:\t\t%u\n", param->groupid);
	printf("\t\tcountable:\t\t%u\n", param->countable);

	rd_write_section(RD_CMD, buf, snprintf(buf, sizeof(buf),
			"perfcounter_put: groupid=%u, countable=%u",
			param->groupid, param->countable));
}

static void kgsl_ioctl_pre(int fd, unsigned long int request, void *ptr)
{
	dump_ioctl(get_kgsl_info(fd), _IOC_WRITE, fd, request, ptr, 0);
	switch(_IOC_NR(request)) {
	case _IOC_NR(IOCTL_KGSL_RINGBUFFER_ISSUEIBCMDS):
		kgsl_ioctl_ringbuffer_issueibcmds_pre(fd, ptr);
		break;
	case _IOC_NR(IOCTL_KGSL_SUBMIT_COMMANDS):
		kgsl_ioctl_submit_commands_pre(fd, ptr);
		break;
	case _IOC_NR(IOCTL_KGSL_DRAWCTXT_CREATE):
		kgsl_ioctl_drawctxt_create_pre(fd, ptr);
		break;
	case _IOC_NR(IOCTL_KGSL_SHAREDMEM_FROM_VMALLOC):
		kgsl_ioctl_sharedmem_from_vmalloc_pre(fd, ptr);
		break;
	case _IOC_NR(IOCTL_KGSL_SHAREDMEM_FREE):
		kgsl_ioctl_sharedmem_free_pre(fd, ptr);
		break;
	case _IOC_NR(IOCTL_KGSL_GPUMEM_ALLOC):
		kgsl_ioctl_gpumem_alloc_pre(fd, ptr);
		break;
	case _IOC_NR(IOCTL_KGSL_GPUMEM_ALLOC_ID):
		kgsl_ioctl_gpumem_alloc_id_pre(fd, ptr);
		break;
	case _IOC_NR(IOCTL_KGSL_GPUMEM_FREE_ID):
		kgsl_ioctl_gpumem_free_id_pre(fd, ptr);
		break;
	case _IOC_NR(IOCTL_KGSL_PERFCOUNTER_PUT):
		kgls_ioctl_perfcounter_put_pre(fd, ptr);
		break;
	}
}

static void kgsl_ioctl_post(int fd, unsigned long int request, void *ptr, int ret)
{
	dump_ioctl(get_kgsl_info(fd), _IOC_READ, fd, request, ptr, ret);
	switch(_IOC_NR(request)) {
	case _IOC_NR(IOCTL_KGSL_RINGBUFFER_ISSUEIBCMDS):
		kgsl_ioctl_ringbuffer_issueibcmds_post(fd, ptr);
		break;
	case _IOC_NR(IOCTL_KGSL_SUBMIT_COMMANDS):
		kgsl_ioctl_submit_commands_post(fd, ptr);
		break;
	case _IOC_NR(IOCTL_KGSL_DRAWCTXT_CREATE):
		kgsl_ioctl_drawctxt_create_post(fd, ptr);
		break;
	case _IOC_NR(IOCTL_KGSL_DEVICE_GETPROPERTY):
		kgsl_ioctl_device_getproperty_post(fd, ptr);
		break;
	case _IOC_NR(IOCTL_KGSL_SHAREDMEM_FROM_VMALLOC):
		kgsl_ioctl_sharedmem_from_vmalloc_post(fd, ptr);
		break;
	case _IOC_NR(IOCTL_KGSL_GPUMEM_ALLOC):
		kgsl_ioctl_gpumem_alloc_post(fd, ptr);
		break;
	case _IOC_NR(IOCTL_KGSL_GPUMEM_ALLOC_ID):
		kgsl_ioctl_gpumem_alloc_id_post(fd, ptr);
		break;
	case _IOC_NR(IOCTL_KGSL_GPUMEM_FREE_ID):
		kgsl_ioctl_gpumem_free_id_post(fd, ptr);
		break;
	case _IOC_NR(IOCTL_KGSL_PERFCOUNTER_GET):
		kgls_ioctl_perfcounter_get_post(fd, ptr);
		break;
	}
}

static void drm_ioctl_pre(int fd, unsigned long int request, void *ptr)
{
	dump_ioctl(&drm_info, _IOC_WRITE, fd, request, ptr, 0);
}

static void drm_ioctl_gem_create_post(int fd,
		struct drm_kgsl_gem_create *param)
{
	printf("\t\thandle:\t%d\n", param->handle);
	printf("\t\tsize:\t\t%08x\n", param->size);
	register_buffer(NULL, 0, param->size, param->handle);
}

static void drm_ioctl_gem_alloc_post(int fd,
		struct drm_kgsl_gem_alloc *param)
{
	struct buffer *buf = find_buffer(NULL, 0, 0, param->handle, 0);
	if (buf) {
		buf->offset = param->offset;
		printf("\t\thandle:\t%d\n", buf->handle);
		printf("\t\toffset:\t%08llx\n", buf->offset);
	}
}

static void drm_ioctl_gem_get_bufinfo_post(int fd,
		struct drm_kgsl_gem_bufinfo *param)
{
	struct buffer *buf = find_buffer(NULL, 0, 0, param->handle, 0);
	if (buf) {
		buf->gpuaddr = param->gpuaddr[0];
		log_gpuaddr(buf->gpuaddr, buf->len);
		printf("\t\thandle:\t%d\n", param->handle);
		printf("\t\tgpuaddr:\t%08lx\n", param->gpuaddr[0]);
	}
}

static void drm_ioctl_gem_close_post(int fd,
		struct drm_gem_close *param)
{
	struct buffer *buf = find_buffer(NULL, 0, 0, param->handle, 0);
	printf("\t\thandle:\t%d\n", param->handle);
	unregister_buffer(buf);
}

static void drm_ioctl_post(int fd, unsigned long int request, void *ptr, int ret)
{
	dump_ioctl(&drm_info, _IOC_READ, fd, request, ptr, ret);
	switch(_IOC_NR(request)) {
	case _IOC_NR(DRM_IOCTL_KGSL_GEM_CREATE):
		drm_ioctl_gem_create_post(fd, ptr);
		break;
	case _IOC_NR(DRM_IOCTL_KGSL_GEM_ALLOC):
		drm_ioctl_gem_alloc_post(fd, ptr);
		break;
	case _IOC_NR(DRM_IOCTL_KGSL_GEM_GET_BUFINFO):
		drm_ioctl_gem_get_bufinfo_post(fd, ptr);
		break;
	case _IOC_NR(DRM_IOCTL_GEM_CLOSE):
		drm_ioctl_gem_close_post(fd, ptr);
		break;
	}
}

int ioctl(int fd, unsigned long int request, ...)
{
	int ioc_size = _IOC_SIZE(request);
	int ret;
	PROLOG(ioctl);
	void *ptr;

	// XXX fbdev doesn't appear to play by the rules:
	ioc_size = 1;

	if (ioc_size) {
		va_list args;

		va_start(args, request);
		ptr = va_arg(args, void *);
		va_end(args);
	} else {
		ptr = NULL;
	}

#ifdef USE_PTHREADS
	pthread_mutex_lock(&l);
#endif

	if (get_kgsl_info(fd))
		kgsl_ioctl_pre(fd, request, ptr);
	else if (is_drm(fd))
		drm_ioctl_pre(fd, request, ptr);
	else
		printf("> [%4d]         : <unknown> (%08lx)\n", fd, request);

	if ((_IOC_NR(request) == _IOC_NR(IOCTL_KGSL_RINGBUFFER_ISSUEIBCMDS)) &&
			get_kgsl_info(fd) && wrap_safe()) {
		sync();
		sleep(1);
	}

#ifdef USE_PTHREADS
	pthread_mutex_unlock(&l);
#endif

	if (file_table[fd].is_emulated) {
		ret = 0;
	} else {
		ret = orig_ioctl(fd, request, ptr);
	}

#ifdef USE_PTHREADS
	pthread_mutex_lock(&l);
#endif

	if (get_kgsl_info(fd))
		kgsl_ioctl_post(fd, request, ptr, ret);
	else if (is_drm(fd))
		drm_ioctl_post(fd, request, ptr, ret);
	else
		printf("< [%4d]         : <unknown> (%08lx) (%d)\n", fd, request, ret);

#ifdef USE_PTHREADS
	pthread_mutex_unlock(&l);
#endif

	if ((_IOC_NR(request) == _IOC_NR(IOCTL_KGSL_RINGBUFFER_ISSUEIBCMDS)) &&
			get_kgsl_info(fd) && wrap_safe()) {
		sync();
		sleep(1);
	}

	return ret;
}

void * mmap(void *addr, size_t length, int prot, int flags, int fd, off_t offset)
{
	void *ret = NULL;
	PROLOG(mmap);

#ifdef USE_PTHREADS
	pthread_mutex_lock(&l);
#endif

	if (get_kgsl_info(fd) || is_drm(fd)) {
		struct buffer *buf = find_buffer(NULL, 0, offset, 0, 0);

		printf("< [%4d]         : mmap: addr=%p, length=%d, prot=%x, flags=%x, offset=%08lx\n",
				fd, addr, length, prot, flags, offset);

		if (buf && buf->hostptr) {
			buf->munmap = 0;
			ret = buf->hostptr;
		}
	}

	if (!ret) {
		if ((fd >= 0) && file_table[fd].is_emulated) {
			ret = malloc(length);
		} else {
			ret = orig_mmap(addr, length, prot, flags, fd, offset);
		}
	}

	if (get_kgsl_info(fd) || is_drm(fd)) {
		struct buffer *buf = find_buffer(NULL, 0, offset, 0, 0);
		if (buf)
			buf->hostptr = ret;
		else {
			/*
			 * when a buffer is allocated using IOCTL_KGSL_GPUMEM_ALLOC_ID
			 * it's mmapped by id, not by gpuaddr, so try to find that
			 * buffer via id now.
			 */
			buf = find_buffer(NULL, 0, 0, 0, offset >> 12);
			if (buf)
				buf->hostptr = ret;
		}
		printf("< [%4d]         : mmap: -> (%p)\n", fd, ret);
	}

#ifdef USE_PTHREADS
	pthread_mutex_unlock(&l);
#endif

	return ret;
}

void *mmap64(void *addr, size_t length, int prot, int flags, int fd, __off64_t offset)
{
	void *ret = NULL;
	PROLOG(mmap64);

#ifdef USE_PTHREADS
	pthread_mutex_lock(&l);
#endif

	if (get_kgsl_info(fd) || is_drm(fd)) {
		struct buffer *buf = find_buffer(NULL, 0, offset, 0, 0);

		printf("< [%4d]         : mmap64: addr=%p, length=%d, prot=%x, flags=%x, offset=%08lx\n",
				fd, addr, length, prot, flags, offset);

		if (buf && buf->hostptr) {
			buf->munmap = 0;
			ret = buf->hostptr;
		}
	}

	if (!ret) {
		if ((fd >= 0) && file_table[fd].is_emulated) {
			ret = calloc(1, length);
		} else {
			ret = orig_mmap64(addr, length, prot, flags, fd, offset);
		}
	}

	if (get_kgsl_info(fd) || is_drm(fd)) {
		struct buffer *buf = find_buffer(NULL, 0, offset, 0, 0);
		if (buf)
			buf->hostptr = ret;
		else {
			/*
			 * when a buffer is allocated using IOCTL_KGSL_GPUMEM_ALLOC_ID
			 * it's mmapped by id, not by gpuaddr, so try to find that
			 * buffer via id now.
			 */
			buf = find_buffer(NULL, 0, 0, 0, offset >> 12);
			if (buf)
				buf->hostptr = ret;
		}
		printf("< [%4d]         : mmap64: -> (%p)\n", fd, ret);
	}

#ifdef USE_PTHREADS
	pthread_mutex_unlock(&l);
#endif

	return ret;
}

int munmap(void *addr, size_t length)
{
	struct buffer *buf = find_buffer(addr, 0, 0, 0, 0);
	PROLOG(munmap);

	if (buf) {
		buf->munmap = 1;
		return 0;
	}

	return orig_munmap(addr, length);
}