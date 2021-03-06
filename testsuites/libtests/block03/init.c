/**
 * @file
 *
 * @ingroup test_bdbuf
 *
 * @brief Bdbuf test.
 */

/*
 * Copyright (c) 2009-2012 embedded brains GmbH.  All rights reserved.
 *
 *  embedded brains GmbH
 *  Obere Lagerstr. 30
 *  82178 Puchheim
 *  Germany
 *  <rtems@embedded-brains.de>
 *
 * The license and distribution terms for this file may be
 * found in the file LICENSE in this distribution or at
 * http://www.rtems.org/license/LICENSE.
 */

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#define TESTS_USE_PRINTK
#include "tmacros.h"

#include <rtems.h>
#include <rtems/ramdisk.h>
#include <rtems/bdbuf.h>
#include <rtems/diskdevs.h>

const char rtems_test_name[] = "BLOCK 3";

/* forward declarations to avoid warnings */
static rtems_task Init(rtems_task_argument argument);

#define ASSERT_SC(sc) rtems_test_assert((sc) == RTEMS_SUCCESSFUL)

#define PRIORITY_INIT 10

#define PRIORITY_SWAPOUT 50

#define PRIORITY_HIGH 30

#define PRIORITY_LOW 40

#define BLOCK_SIZE 512

#define BLOCK_COUNT 2

static rtems_disk_device *dd;

static volatile bool sync_done = false;

static rtems_id task_id_low;

static rtems_id task_id_high;

static void task_low(rtems_task_argument arg)
{
  rtems_status_code sc = RTEMS_SUCCESSFUL;
  rtems_bdbuf_buffer *bd = NULL;

  rtems_test_assert(!sync_done);

  printk("L: try access: 0\n");

  sc = rtems_bdbuf_get(dd, 0, &bd);
  ASSERT_SC(sc);

  rtems_test_assert(sync_done);

  printk("L: access: 0\n");

  rtems_test_assert(bd->block == 0);

  TEST_END();

  exit(0);
}

static void task_high(rtems_task_argument arg)
{
  rtems_status_code sc = RTEMS_SUCCESSFUL;
  rtems_bdbuf_buffer *bd = NULL;

  rtems_test_assert(!sync_done);

  printk("H: try access: 0\n");

  sc = rtems_bdbuf_get(dd, 0, &bd);
  ASSERT_SC(sc);

  rtems_test_assert(sync_done);

  printk("H: access: 0\n");

  printk("H: release: 0\n");

  sc = rtems_bdbuf_release(bd);
  ASSERT_SC(sc);

  printk("H: release done: 0\n");

  printk("H: try access: 1\n");

  sc = rtems_bdbuf_get(dd, 1, &bd);
  ASSERT_SC(sc);

  printk("H: access: 1\n");

  /* If we reach this code, we have likely a bug */

  printk("H: release: 1\n");

  sc = rtems_bdbuf_release(bd);
  ASSERT_SC(sc);

  printk("H: release done: 1\n");

  rtems_task_delete(RTEMS_SELF);
}

static rtems_task Init(rtems_task_argument argument)
{
  rtems_status_code sc = RTEMS_SUCCESSFUL;
  rtems_bdbuf_buffer *bd = NULL;
  dev_t dev = 0;

  TEST_BEGIN();

  sc = rtems_disk_io_initialize();
  ASSERT_SC(sc);

  sc = ramdisk_register(BLOCK_SIZE, BLOCK_COUNT, false, "/dev/rda", &dev);
  ASSERT_SC(sc);

  dd = rtems_disk_obtain(dev);
  rtems_test_assert(dd != NULL);

  sc = rtems_task_create(
    rtems_build_name(' ', 'L', 'O', 'W'),
    PRIORITY_LOW,
    0,
    RTEMS_DEFAULT_MODES,
    RTEMS_DEFAULT_ATTRIBUTES,
    &task_id_low
  );
  ASSERT_SC(sc);

  sc = rtems_task_start(task_id_low, task_low, 0);
  ASSERT_SC(sc);

  sc = rtems_task_create(
    rtems_build_name('H', 'I', 'G', 'H'),
    PRIORITY_HIGH,
    0,
    RTEMS_DEFAULT_MODES,
    RTEMS_DEFAULT_ATTRIBUTES,
    &task_id_high
  );
  ASSERT_SC(sc);

  sc = rtems_task_start(task_id_high, task_high, 0);
  ASSERT_SC(sc);

  sc = rtems_bdbuf_get(dd, 0, &bd);
  ASSERT_SC(sc);

  sc = rtems_bdbuf_sync(bd);
  ASSERT_SC(sc);

  printk("I: sync done: 0\n");

  sync_done = true;

  sc = rtems_task_suspend(RTEMS_SELF);
  ASSERT_SC(sc);
}

#define CONFIGURE_INIT

#define CONFIGURE_APPLICATION_DOES_NOT_NEED_CLOCK_DRIVER
#define CONFIGURE_APPLICATION_NEEDS_CONSOLE_DRIVER
#define CONFIGURE_APPLICATION_NEEDS_LIBBLOCK

#define CONFIGURE_MAXIMUM_TASKS 3
#define CONFIGURE_MAXIMUM_DRIVERS 2

#define CONFIGURE_INITIAL_EXTENSIONS RTEMS_TEST_INITIAL_EXTENSION

#define CONFIGURE_RTEMS_INIT_TASKS_TABLE

#define CONFIGURE_INIT_TASK_PRIORITY PRIORITY_INIT
#define CONFIGURE_INIT_TASK_ATTRIBUTES RTEMS_DEFAULT_ATTRIBUTES
#define CONFIGURE_INIT_TASK_INITIAL_MODES RTEMS_DEFAULT_MODES

#define CONFIGURE_SWAPOUT_TASK_PRIORITY PRIORITY_SWAPOUT

#define CONFIGURE_BDBUF_BUFFER_MIN_SIZE BLOCK_SIZE
#define CONFIGURE_BDBUF_BUFFER_MAX_SIZE BLOCK_SIZE
#define CONFIGURE_BDBUF_CACHE_MEMORY_SIZE BLOCK_SIZE

#include <rtems/confdefs.h>
