#ifndef SCREENSHOT_MANAGER_RELEASE_STRINGS_H
#define SCREENSHOT_MANAGER_RELEASE_STRINGS_H

/* Keep the string order and the four padded fields of the original 0.1.0 ELF.
   layout.ld places these sections after the ordinary pooled string literals. */
#define RELEASE_SLOT(order) __attribute__((unused, aligned(1), section(".rodata.sm." #order)))

static const char sm_menu_title[26] RELEASE_SLOT(18) = "Screenshot Manager";
static const char sm_service_heading[] RELEASE_SLOT(19) = "Служба скриншотов";
static const char sm_reboot_hint[] RELEASE_SLOT(20) = "После перезагрузки включи её снова.";
static const char sm_b_btn[] RELEASE_SLOT(21) = "B";
static const char sm_font_file[] RELEASE_SLOT(22) = "res/FreeSans.ttf";
static const char sm_start_error[68] RELEASE_SLOT(23) = "Не удалось запустить screenshot.";
static const char sm_check_action[] RELEASE_SLOT(24) = "Проверить";
static const char sm_daemon_cwd[20] RELEASE_SLOT(25) = ".";
static const char sm_unknown[] RELEASE_SLOT(26) = "Статус неизвестен";
static const char sm_read_mode[] RELEASE_SLOT(27) = "r";
static const char sm_proc_exe[] RELEASE_SLOT(28) = "/proc/%ld/exe";
static const char sm_daemon_check[31] RELEASE_SLOT(29) = "./screenshot";
static const char sm_disabled[] RELEASE_SLOT(30) = "Выключена";
static const char sm_capture_hint[] RELEASE_SLOT(31) = "Сделать снимок экрана";
static const char sm_r2_btn[] RELEASE_SLOT(32) = "R2";
static const char sm_disable_action[] RELEASE_SLOT(33) = "Выключить";
static const char sm_comm_name[] RELEASE_SLOT(34) = "screenshot\n";
static const char sm_stop_error[] RELEASE_SLOT(35) = "Не удалось остановить службу.";
static const char sm_l2_btn[] RELEASE_SLOT(36) = "L2";
static const char sm_init_error[] RELEASE_SLOT(37) = "UI initialization: %s / %s\n";
static const char sm_exit_action[] RELEASE_SLOT(38) = "Выход";
static const char sm_daemon_arg[] RELEASE_SLOT(39) = "screenshot";
static const char sm_null_device[] RELEASE_SLOT(40) = "/dev/null";

#undef RELEASE_SLOT
#endif
