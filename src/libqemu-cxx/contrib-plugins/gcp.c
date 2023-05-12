/*
 * Copyright (c) 2013, Xilinx, Edgar E. Iglesias
 * Copyright (c) 2023, Qualcomm Innovation Center, Inc. All Rights Reserved.
 *
 * gcp plugin - generic code coverage plugin.
 *
 * License: GNU GPL, version 2 or later.
 *   See the COPYING file in the top-level directory.
 */

#include <assert.h>
#include <glib.h>
#include <inttypes.h>
#include <qemu-plugin.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#if defined(_WIN32) && (defined(__x86_64__) || defined(__i386__))
#define QEMU_PACKED __attribute__((gcc_struct, packed))
#else
#define QEMU_PACKED __attribute__((packed))
#endif

struct etrace_entry32 {
  uint32_t duration;
  uint32_t start, end;
};

struct etrace_entry64 {
  uint32_t duration;
  uint64_t start, end;
} QEMU_PACKED;

struct etracer {
  const char *filename;
  FILE *fp;
  unsigned int arch_bits;

#define EXEC_CACHE_SIZE (16 * 1024)
  uint64_t exec_start;
  uint64_t exec_end;
  bool exec_start_valid;
  int64_t exec_start_time;
  struct {
    union {
      struct etrace_entry64 t64[EXEC_CACHE_SIZE];
      struct etrace_entry32 t32[2 * EXEC_CACHE_SIZE];
    };
    uint64_t start_time;
    unsigned int pos;
    unsigned int unit_id;
  } exec_cache;
};

/* Still under development.  */
#define ETRACE_VERSION_MAJOR 0
#define ETRACE_VERSION_MINOR 0

enum {
  TYPE_EXEC = 1,
  TYPE_TB = 2,
  TYPE_NOTE = 3,
  TYPE_MEM = 4,
  TYPE_ARCH = 5,
  TYPE_BARRIER = 6,
  TYPE_OLD_EVENT_U64 = 7,
  TYPE_EVENT_U64 = 8,
  TYPE_INFO = 0x4554,
};

struct etrace_hdr {
  uint16_t type;
  uint16_t unit_id;
  uint32_t len;
} QEMU_PACKED;

struct etrace_info_data {
  uint64_t attr;
  struct {
    uint16_t major;
    uint16_t minor;
  } version;
} QEMU_PACKED;

struct etrace_arch {
  struct {
    uint32_t arch_id;
    uint8_t arch_bits;
    uint8_t big_endian;
  } guest, host;
} QEMU_PACKED;

struct etrace_exec {
  uint64_t start_time;
} QEMU_PACKED;

typedef struct {
  uint32_t start;
  uint32_t end;
  uint16_t size;
} tb_entry;

struct etracer qemu_etracer = {0};
bool plugin_init_enabled;

QEMU_PLUGIN_EXPORT int qemu_plugin_version = QEMU_PLUGIN_VERSION;

static FILE *etrace_open(const char *descr) {
  FILE *fp = NULL;

  if (descr == NULL) {
    return NULL;
  }
  fp = fopen(descr, "w");
  return fp;
}

static void etrace_write(struct etracer *t, const void *buf, size_t len) {

  size_t r;
  r = fwrite(buf, 1, len, t->fp);

  if (feof(t->fp) || ferror(t->fp)) {
    fprintf(stderr, "EOF/disconnected!\n");
    fclose(t->fp);
    t->fp = NULL;
    exit(1);
  }
  /* FIXME: Make this more robust.  */
  assert(r == len);
}

static void etrace_write_header(struct etracer *t, uint16_t type,
                                uint16_t unit_id, uint32_t len) {

  struct etrace_hdr hdr = {.type = type, .unit_id = unit_id, .len = len};
  etrace_write(t, &hdr, sizeof hdr);
}

static bool plugin_init(struct etracer *t, const char *filename,
                        unsigned int arch_id, unsigned int arch_bits) {
  struct etrace_info_data id;
  struct etrace_arch arch;

  memset(t, 0, sizeof *t);
  t->fp = etrace_open(filename);
  if (!t->fp) {
    return false;
  }

  memset(&id, 0, sizeof id);
  id.version.major = ETRACE_VERSION_MAJOR;
  id.version.minor = ETRACE_VERSION_MINOR;
  id.attr = 0;
  etrace_write_header(t, TYPE_INFO, 0, sizeof id);
  etrace_write(t, &id, sizeof id);

  /* Pass info about host.  */
  arch.host.arch_id = 0;
  arch.host.arch_bits = 0;
  arch.host.big_endian = 0;
  /* Pass info about guest.  */
  arch.guest.big_endian = 0;
  arch.guest.arch_id = arch_id;
#ifdef TARGET_WORDS_BIGENDIAN
  arch.guest.big_endian = 1;
#endif

  t->arch_bits = arch.guest.arch_bits = arch_bits;

  etrace_write_header(t, TYPE_ARCH, 0, sizeof arch);
  etrace_write(t, &arch, sizeof arch);
  return true;
}

static void etrace_flush_exec_cache(struct etracer *t) {

  size_t size64 = t->exec_cache.pos * sizeof t->exec_cache.t64[0];
  size_t size32 = t->exec_cache.pos * sizeof t->exec_cache.t32[0];
  size_t size = t->arch_bits == 32 ? size32 : size64;

  struct etrace_exec ex;
  if (!size) {
    return;
  }

  ex.start_time = t->exec_cache.start_time;

  etrace_write_header(t, TYPE_EXEC, t->exec_cache.unit_id, size + sizeof ex);
  etrace_write(t, &ex, sizeof ex);
  etrace_write(t, &t->exec_cache.t64[0], size);
  t->exec_cache.pos = 0;
  memset(&t->exec_cache.t64[0], 0, sizeof t->exec_cache.t64);

  /* A barrier indicates that the other side can assume order across the
     the barrier.  */
  etrace_write_header(t, TYPE_BARRIER, t->exec_cache.unit_id, 0);
}

static void vcpu_tb_exec(unsigned int cpu_index, void *udata) {

  tb_entry *bb = (tb_entry *)udata;

  struct etracer *t = &qemu_etracer;

  unsigned int pos;

  if (cpu_index != t->exec_cache.unit_id) {
    etrace_flush_exec_cache(t);
    t->exec_cache.unit_id = cpu_index;
  }

  pos = t->exec_cache.pos;
  if (pos == 0) {
    t->exec_cache.start_time = t->exec_start_time;
  }

  assert(t->arch_bits == 32 || t->arch_bits == 64);
  if (t->arch_bits == 64) {
    t->exec_cache.pos += 1;
    t->exec_cache.t64[pos].start = bb->start;
    t->exec_cache.t64[pos].duration = 0; // tdiff
    t->exec_cache.t64[pos].end = bb->end;
  } else {
    t->exec_cache.pos += 1;
    t->exec_cache.t32[pos].start = bb->start;
    t->exec_cache.t32[pos].duration = 0; // tdiff
    t->exec_cache.t32[pos].end = bb->end;
  }
  if (t->exec_cache.pos == EXEC_CACHE_SIZE) {
    etrace_flush_exec_cache(t);
  }
}

static void vcpu_tb_trans(qemu_plugin_id_t id, struct qemu_plugin_tb *tb) {
  uint64_t pc = qemu_plugin_tb_vaddr(tb);
  size_t n = qemu_plugin_tb_n_insns(tb);

  tb_entry *bb = calloc(1, sizeof *bb); // #fixme free this later

  for (int i = 0; i < n; i++) {
    bb->size += qemu_plugin_insn_size(qemu_plugin_tb_get_insn(tb, i));
  }
  bb->start = pc;
  bb->end = bb->start + bb->size;
  qemu_plugin_register_vcpu_tb_exec_cb(tb, vcpu_tb_exec, QEMU_PLUGIN_CB_NO_REGS,
                                       (void *)bb);
}

static void etrace_close(struct etracer *t) {
  if (t->fp) {
    etrace_flush_exec_cache(t);
    fclose(t->fp);
  }
}

static void plugin_exit(qemu_plugin_id_t id, void *p) {
  etrace_close(&qemu_etracer);
}

QEMU_PLUGIN_EXPORT
int qemu_plugin_install(qemu_plugin_id_t id, const qemu_info_t *info, int argc,
                        char **argv) {
  /* Default trace argument */
  const char *filename = "/local/mnt/workspace/tmp/gcp-log";
  unsigned int arch_id = 0;
  unsigned int arch_bits = 64;

  for (int i = 0; i < argc; i++) {
    char *opt = argv[i];
    g_autofree char **tokens = g_strsplit(opt, "=", 2);
    if (g_strcmp0(tokens[0], "filename") == 0) {
      filename = tokens[1];
    } else if (g_strcmp0(tokens[0], "arch_id") == 0) {
      arch_id = strtol(tokens[1], NULL, 0);
    } else if (g_strcmp0(tokens[0], "arch_bits") == 0) {
      arch_bits = strtol(tokens[1], NULL, 0);
    } else {
      fprintf(stderr, "option parsing failed: %s\n", opt);
      return -1;
    }
  }
  plugin_init_enabled =
      plugin_init(&qemu_etracer, filename, arch_id, arch_bits);
  if (!plugin_init_enabled) {
    perror("gcp-plugin arguments invalid");
    exit(1);
  }
  qemu_plugin_register_vcpu_tb_trans_cb(id, vcpu_tb_trans);
  qemu_plugin_register_atexit_cb(id, plugin_exit, NULL);
  return 0;
}