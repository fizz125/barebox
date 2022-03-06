// SPDX-License-Identifier: GPL-2.0-or-later
/*
 * Copyright (C) 2022 David Dgien <dgienda125@gmail.com>
 */

#include <common.h>
#include <driver.h>
#include <fcntl.h>
#include <init.h>
#include <stdio.h>
#include <unistd.h>

static bool stdio_initialized = false;

#if (!defined(__PBL__) && !defined(CONFIG_CONSOLE_NONE)) || \
	(defined(__PBL__) && defined(CONFIG_PBL_CONSOLE))
int puts(const char *s)
{
	if (!stdio_initialized)
		return console_puts(CONSOLE_STDOUT, s);

	return write(STDOUT_FILENO, s, strlen(s));
}

void putchar(char c)
{
	if (!stdio_initialized)
		console_putc(CONSOLE_STDOUT, c);

	write(STDOUT_FILENO, &c, 1);
}
#endif

#ifndef CONFIG_CONSOLE_NONE
struct cdev bb_stdin;
struct cdev bb_stdout;
struct cdev bb_stderr;

int getchar(void)
{
	char ch;

	read(STDIN_FILENO, &ch, 1);

	return ch;
}

static ssize_t dummy_read(struct cdev*, void* buf, size_t count, loff_t offset, ulong flags)
{
	return 0;
}

static ssize_t dummy_write(struct cdev*, const void* buf, size_t count, loff_t offset, ulong flags)
{
	return 0;
}

static ssize_t stdin_read(struct cdev*, void* buf, size_t count, loff_t offset, ulong flags)
{
	char ch;
	if (count == 1) {
		ch = console_getchar();
		if (ch < 0) {
			return 0;
		}

		((char *)buf)[0] = ch;
		return 1;
	} else {
		/* When readline runs on it's own, it gets each character using getc ->
		 * read -> stdin_read. But if code tries to manually call read it would
		 * be preferable for that have readline semantics on its own, so that
		 * the user can see what they're typing. So we'll just call readline
		 * when count > 1
		 *
		 * The only downsides are that we'd really rather not have all of the
		 * features of readline (namely, the same history shared with the
		 * shell and tab-completion), or the awkward recursion that this
		 * introduces
		 */
		return readline("", (char *)buf, count);
		return strlen(buf);
	}

	return 0;
}

static ssize_t stdout_write(struct cdev*, const void* buf, size_t count, loff_t offset, ulong flags)
{
	/* console_puts mangles certain character codes emitted by readline
	 * (particularly when pressing backspace), so if count is only 1, use
	 * console_putc instead
	 */
	if (count == 1) {
		console_putc(CONSOLE_STDOUT, ((char *)buf)[0]);
		return 1;
	}
	return console_puts(CONSOLE_STDOUT, (const char *)buf);
}

static ssize_t stderr_write(struct cdev*, const void* buf, size_t count, loff_t offset, ulong flags)
{
	if (count == 1) {
		console_putc(CONSOLE_STDERR, ((char *)buf)[0]);
		return 1;
	}
	return console_puts(CONSOLE_STDERR, (const char *)buf);
}

static struct cdev_operations stdin_ops = {
	.read = stdin_read,
	.write = dummy_write,
};

static struct cdev_operations stdout_ops = {
	.read = dummy_read,
	.write = stdout_write,
};

static struct cdev_operations stderr_ops = {
	.read = dummy_read,
	.write = stderr_write,
};

static int create_dev(struct cdev *stdio_dev, struct cdev_operations *ops, const char *name)
{
	memset(stdio_dev, 0, sizeof(*stdio_dev));

	stdio_dev->name = strdup(name);
	stdio_dev->ops = ops;
	stdio_dev->flags = DEVFS_IS_CHARACTER_DEV;

	return devfs_create(stdio_dev);
}

static int create_stdio_devs(void)
{
	int ret, ret2 = 0;;

	ret = create_dev(&bb_stdin, &stdin_ops, "stdin");
	if (ret != 0) {
		ret2 = ret;
		pr_err("Unable to create 'stdin' cdev\n");
	}

	ret = create_dev(&bb_stdout, &stdout_ops, "stdout");
	if (ret != 0) {
		ret2 = ret;
		pr_err("Unable to create 'stdout' cdev\n");
	}

	ret = create_dev(&bb_stderr, &stderr_ops, "stderr");
	if (ret != 0) {
		ret2 = ret;
		pr_err("Unable to create 'stderr' cdev\n");
	}

	return ret2;
}
core_initcall(create_stdio_devs);
#endif /* !CONFIG_CONSOLE_NONE */

static int __init_stdio_fd(int fileno, const char *dev_path, int flags)
{
	int fd;
	int ret;

	fd = open(dev_path, flags);
	if (fd < 0) {
		pr_err("Unable to open '%s': %s\n", dev_path, strerror(-fd));
		return fd;
	}

	ret = dup2(fd, fileno);
	if (ret < 0) {
		pr_err("Unable to dup2 '%s'\n", dev_path);
		close(fd);
		return ret;
	}

	close(fd);

	return 0;
}

static int stdio_init(void)
{
	int ret, ret2 = 0;

	ret = __init_stdio_fd(STDIN_FILENO, "/dev/stdin", O_RDONLY);
	if (ret < 0)
		ret2 = ret;

	ret = __init_stdio_fd(STDOUT_FILENO, "/dev/stdout", O_WRONLY);
	if (ret < 0)
		ret2 = ret;

	ret = __init_stdio_fd(STDERR_FILENO, "/dev/stderr", O_WRONLY);
	if (ret < 0)
		ret2 = ret;

	if (ret2 == 0)
		stdio_initialized = true;

	return ret2;
}
device_initcall(stdio_init);
