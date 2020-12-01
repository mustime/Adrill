// code from AOSP

#include <unistd.h>
#include <fcntl.h>
#include <string.h>
#include <stdlib.h>
#include <errno.h>
#include <ctype.h>
#include <stdio.h>
#include <dlfcn.h>

#ifdef DARWIN
#include <sys/param.h>
#include <sys/mount.h>
#else
#include <sys/vfs.h>
#endif

#include <stdint.h>
#include <limits.h>

#include "selinux.h"

char* SELinux::_selinux_mnt = nullptr;

void SELinux::init() {
	char buf[BUFSIZ], *p;
	FILE *fp = nullptr;
	struct statfs sfbuf;
	int rc;
	char *bufp;
	int exists = 0;

	if (_selinux_mnt)
		return;

	/* We check to see if the preferred mount point for selinux file
	 * system has a selinuxfs. */
	do {
		rc = statfs(SELINUXMNT, &sfbuf);
	} while (rc < 0 && errno == EINTR);
	if (rc == 0) {
		if ((uint32_t)sfbuf.f_type == (uint32_t)SELINUX_MAGIC) {
			_selinux_mnt = strdup(SELINUXMNT);
			return;
		}
	}

	/* Drop back to detecting it the long way. */
	fp = fopen("/proc/filesystems", "r");
	if (!fp) return;

	while ((bufp = fgets(buf, sizeof buf - 1, fp)) != nullptr) {
		if (strstr(buf, "selinuxfs")) {
			exists = 1;
			break;
		}
	}

	if (!exists)
		goto out;

	fclose(fp);

	/* At this point, the usual spot doesn't have an selinuxfs so
	 * we look around for it */
	fp = fopen("/proc/mounts", "r");
	if (!fp)
		goto out;

	while ((bufp = fgets(buf, sizeof buf - 1, fp)) != nullptr) {
		char *tmp;
		p = strchr(buf, ' ');
		if (!p)
			goto out;
		p++;
		tmp = strchr(p, ' ');
		if (!tmp)
			goto out;
		if (!strncmp(tmp + 1, "selinuxfs ", 10)) {
			*tmp = '\0';
			break;
		}
	}

	/* If we found something, dup it */
	if (bufp)
		_selinux_mnt = strdup(p);

      out:
	if (fp)
		fclose(fp);
	return;
}

SELinuxStatus SELinux::getEnforce() {
	SELinuxStatus enforce = SELinuxStatus::UNKNOWN;
	int fd, ret;
	char path[PATH_MAX];
	char buf[20];

	if (!_selinux_mnt) {
		errno = ENOENT;
		return enforce;
	}

	snprintf(path, sizeof path, "%s/enforce", _selinux_mnt);
	fd = open(path, O_RDONLY);
	if (fd < 0)
		return enforce;

	memset(buf, 0, sizeof buf);
	ret = read(fd, buf, sizeof buf - 1);
	close(fd);
	if (ret < 0)
		return enforce;

	if (sscanf(buf, "%d", (int*)&enforce) != 1)
		return enforce;

	return enforce;
}

bool SELinux::setEnforce(SELinuxStatus value) {
	bool succ = true;
	int fd;
	char path[PATH_MAX];
	char buf[20];

	if (!_selinux_mnt) {
		errno = ENOENT;
		return -1;
	}

	snprintf(path, sizeof path, "%s/enforce", _selinux_mnt);
	fd = open(path, O_RDWR);
	if (fd < 0)
		return -1;

	snprintf(buf, sizeof buf, "%d", (int)value);
	int ret = write(fd, buf, strlen(buf));
	close(fd);
	if (ret < 0)
		succ = false;
	return succ;
}
