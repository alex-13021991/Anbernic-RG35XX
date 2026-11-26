/*
 * Minimal QEMU TCG plugin: counts the ARM instructions the guest actually executes.
 *
 * qemu 8.2's contrib/plugins has no "insn" counter (only hotblocks/howvec/...), and the
 * timing qemu-arm reports is emulation speed on the host, not Cortex-A7 speed. A dynamic
 * instruction count is the one number that IS real, and dividing it by an assumed A7 IPC
 * and clock gives a far better anchor than extrapolating from an x86 run.
 *
 * Counting is done with an inline TB-exec add, so the plugin itself costs almost nothing.
 */
#include <inttypes.h>
#include <stdio.h>
#include <glib.h>
#include <qemu-plugin.h>

QEMU_PLUGIN_EXPORT int qemu_plugin_version = QEMU_PLUGIN_VERSION;

static uint64_t insn_count;

static void vcpu_tb_trans(qemu_plugin_id_t id, struct qemu_plugin_tb *tb)
{
    size_t n = qemu_plugin_tb_n_insns(tb);
    qemu_plugin_register_vcpu_tb_exec_inline(tb, QEMU_PLUGIN_INLINE_ADD_U64, &insn_count, n);
}

static void plugin_exit(qemu_plugin_id_t id, void *p)
{
    g_autoptr(GString) out = g_string_new(NULL);
    g_string_printf(out, "[INSN] executed: %" PRIu64 "\n", insn_count);
    qemu_plugin_outs(out->str);
}

QEMU_PLUGIN_EXPORT int qemu_plugin_install(qemu_plugin_id_t id,
                                           const qemu_info_t *info,
                                           int argc, char **argv)
{
    qemu_plugin_register_vcpu_tb_trans_cb(id, vcpu_tb_trans);
    qemu_plugin_register_atexit_cb(id, plugin_exit, NULL);
    return 0;
}
