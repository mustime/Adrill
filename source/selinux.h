// code from AOSP

#ifndef __ADRILL_SE_LINUX_H__
#define __ADRILL_SE_LINUX_H__

/* Private definitions used internally by libselinux. */

/* xattr name for SELinux attributes. */
#define XATTR_NAME_SELINUX "security.selinux"

/* Initial length guess for getting contexts. */
#define INITCONTEXTLEN 255

/* selinuxfs magic number */
#define SELINUX_MAGIC 0xf97cff8c

/* Preferred selinuxfs mount point directory paths. */
#define SELINUXMNT "/sys/fs/selinux"
#define OLDSELINUXMNT "/selinux"

/* selinuxfs filesystem type string. */
#define SELINUXFS "selinuxfs"

/* First version of policy supported in mainline Linux. */
#define DEFAULT_POLICY_VERSION 15

enum class SELinuxStatus {
	UNKNOWN = -1,
	PERMISSIVE = 0,
	ENFORCING = 1
};

class SELinux {
public:
	static void init();
	static SELinuxStatus getEnforce();
	static bool setEnforce(SELinuxStatus value);

private:
	/* selinuxfs mount point determined at runtime */
	static char* _selinux_mnt;

};

#endif // __ADRILL_SE_LINUX_H__

