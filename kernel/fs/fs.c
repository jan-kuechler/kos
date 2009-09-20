#include <errno.h>
#include <string.h>
#include <kos/syscalln.h>
#include "debug.h"
#include "syscall.h"
#include "tty.h"
#include "fs/fs.h"
#include "mm/util.h"
#include "util/list.h"

/* A list of all registered filesystems */
static list_t *fslist = NULL;

static struct inode root = {
	.name = "/",
	.flags = FS_DIR,
};
struct inode *fs_root = &root;
int fs_error = -ENOSYS;

static inline struct inode *proc_cwd(struct proc *proc)
{
	return proc->fs_data->cwd->inode;
}

int vfs_register(struct fstype *type)
{
	if (!type)
		return -EINVAL;

	if (!fslist)
		fslist = list_create();

	list_add_back(fslist, type);

	return 0;
}

int vfs_unregister(struct fstype *type)
{
	if (!type)
		return -EINVAL;

	list_entry_t *e;
	list_iterate(e, fslist) {
		if (e->data == type) {
			list_del_entry(fslist, e);
			return 0;
		}
	}
	return -EINVAL;
}

struct fstype *vfs_gettype(char *name)
{
	if (!name) {
		fs_error = -EINVAL;
		return NULL;
	}

	list_entry_t *e;
	list_iterate(e, fslist) {
		struct fstype *type = e->data;
		if (strcmp(type->name, name) == 0)
			return type;
	}
	fs_error = -ENOENT;
	return NULL;
}

struct fs_proc_data *vfs_create_procdata()
{
	struct fs_proc_data *data = kmalloc(sizeof(*data));

	data->cwd = kmalloc(sizeof(struct dirent));
	strcpy(data->cwd->name, "/");
	data->cwd->inode = fs_root;

	data->numfiles = 32;
	data->files = kcalloc(data->numfiles, sizeof(struct file*));

	return data;
}

struct fs_proc_data *vfs_clone_procdata(struct fs_proc_data *data)
{
	struct fs_proc_data *newdata = kmalloc(sizeof(*newdata));

	newdata->cwd = kmalloc(sizeof(struct dirent));
	strcpy(newdata->cwd->name, data->cwd->name);
	newdata->cwd->inode = data->cwd->inode;

	newdata->numfiles = data->numfiles;
	newdata->files = kcalloc(data->numfiles, sizeof(struct file*));
	int i=0;
	for (; i < newdata->numfiles; ++i) {
		if (data->files[i]) {
			newdata->files[i] = vfs_dup(data->files[i]);
		}
	}

	return newdata;
}

void vfs_free_procdata(struct fs_proc_data *data)
{
	int i=0;
	for (; i < data->numfiles; ++i) {
		if (data->files[i]) {
			vfs_close(data->files[i]);
		}
	}
	kfree(data->cwd);
	kfree(data->files);
	kfree(data);
}

int vfs_change_dir(struct proc *proc, const char *dir)
{
	if (!dir)
		return -EINVAL;

	struct dirent *old = proc->fs_data->cwd;

	struct dirent *newd = kmalloc(sizeof(*newd));
	strncpy(newd->name, dir, FS_MAX_NAME);
	newd->name[FS_MAX_NAME - 1] = '\0';
	newd->inode = vfs_lookup(newd->name, fs_root);

	if (!newd->inode) {
		kfree(newd);
		return vfs_geterror();
	}

	kfree(old);
	proc->fs_data->cwd = newd;

	return 0;
}

/* FS Syscalls */

static inline struct file *fd2file(struct proc *proc, int fd)
{
	struct fs_proc_data *data = proc->fs_data;

	if (fd >= data->numfiles) {
		return NULL;
	}
	return data->files[fd];
}

static inline int newfd(struct proc *proc)
{
	struct fs_proc_data *data = proc->fs_data;

	int i=0;
	for (; i < data->numfiles; ++i) {
		if (!data->files[i]) {
			return i;
		}
	}
	/* no free slots, make some */
	uint32_t oldnum = data->numfiles;
	data->numfiles += 32;
	data->files = krealloc(data->files, data->numfiles * sizeof(struct file*));
	memset(data->files + oldnum, 0, (data->numfiles - oldnum) * sizeof(struct file*));

	return data->numfiles - 32;
}

int32_t sys_open(int32_t fname, int32_t flags)
{
	dbg_vprintf(DBG_FS, "sys_open(%p, %d) \n", fname, flags);

	size_t namelen = 0;
	char *name = vm_map_string(syscall_proc->as->pdir, (vaddr_t)fname, &namelen);
	int result = -1;

	dbg_printf(DBG_SC, "sys_open(\"%s\", %d)\n", name, flags);

	struct inode *inode = vfs_lookup(name, proc_cwd(syscall_proc));

	if (!inode) {
		result = vfs_geterror();
		goto end;
	}

	struct file *file = vfs_open(inode, flags);
	if (!file) {
		result = vfs_geterror();
		goto end;
	}

	int fd = newfd(syscall_proc);
	syscall_proc->fs_data->files[fd] = file;
	result = fd;
end:
	km_free_addr(name, namelen);

	dbg_printf(DBG_SC, "sys_open: result is %d\n", result);

	return result;
}

int32_t sys_close(int fd)
{
	struct file *file = fd2file(syscall_proc, fd);

	if (!file)
		return (dword)-ENOENT;

	int err = vfs_close(file);
	if (!err) {
		syscall_proc->fs_data->files[fd] = NULL;
	}

	return err;
}

int32_t sys_readwrite(int fd, int32_t buffer, int32_t count)
{
	dbg_vprintf(DBG_MM, "sys_readwrite(%d, %p, %d) for %d\n", fd, buffer, count, syscall_proc->pid);
	void *kbuf = vm_user_to_kernel(syscall_proc->as->pdir, (vaddr_t)buffer, count);
	dbg_vprintf(DBG_MM, " get file\n");
	struct file *file = fd2file(syscall_proc, fd);
	dbg_vprintf(DBG_MM, " got file!\n");
	int result = -ENOSYS;

	if (!file) {
		result = -ENOENT;
		goto end;
	}

	if (syscall_proc->sc_regs->eax == SC_READ) {
		dbg_vprintf(DBG_MM, "  reading\n");
		result = vfs_read(file, kbuf, count, file->pos);
	}
	else {
		dbg_vprintf(DBG_MM, "  writing\n");
		result = vfs_write(file, kbuf, count, file->pos);
	}

end:
	if (result != -EAGAIN)
		km_free_addr(kbuf, count);
	dbg_vprintf(DBG_MM, "Result is %d\n", result);
	return (int32_t)result;
}

int32_t sys_getcwd(void *bufaddr, uint32_t count)
{
	char *buffer = vm_user_to_kernel(syscall_proc->as->pdir, bufaddr, count);

	strncpy(buffer, proc_cwd(syscall_proc)->name, count);
	buffer[count-1] = '\0';

	km_free_addr(buffer, count);

	return (int32_t)bufaddr;
}

int32_t sys_stat(int32_t path, int32_t sbuf)
{
	return -ENOSYS;
}

int32_t sys_isatty(int fd)
{
	struct file *file = fd2file(syscall_proc, fd);
	if (!file)
		return 0;

	return tty_isatty(file);
}

int32_t sys_readdir(uint32_t path, uint32_t userbuf, int index)
{
	int err = 0;

	size_t namelen = 0;
	char *name = vm_map_string(syscall_proc->as->pdir, (vaddr_t)path, &namelen);
	size_t size = 0;
	char *buffer =  vm_map_string(syscall_proc->as->pdir, userbuf, &size);

	if (!name || !buffer) {
		err = -ENOMEM;
		goto end;
	}

	dbg_printf(DBG_FS, "readdir %s %d\n", name, index);


	struct inode *dir = vfs_lookup(name, proc_cwd(syscall_proc));
	if (!dir) {
		err = -ENOENT;
		goto end;
	}

	struct dirent *dirent = vfs_readdir(dir, index);
	if (!dirent) {
		err = vfs_geterror();
		goto end;
	}

	strncpy(buffer, dirent->name, size > FS_MAX_NAME ? FS_MAX_NAME : size);

end:
	if (name)
		km_free_addr(name, namelen);
	if (buffer)
		km_free_addr(buffer, size);

	return err;
}

int32_t sys_mount(int32_t mountp, int32_t ftype, int32_t device)
{
	if (!mountp || !ftype)
		return (dword)-EINVAL;

	size_t pathlen = 0;
	char *path = vm_map_string(syscall_proc->as->pdir, (vaddr_t)mountp, &pathlen);
	size_t typelen = 0;
	char *type = vm_map_string(syscall_proc->as->pdir, (vaddr_t)ftype, &typelen);

	size_t devlen = 0;
	char *dev  = NULL;
	if (device != 0)
		dev = vm_map_string(syscall_proc->as->pdir, (vaddr_t)device, &devlen);

	struct fstype *driver = vfs_gettype(type);
	struct inode *inode = vfs_lookup(path, proc_cwd(syscall_proc));

	int err = 0;

	if (!driver || !inode) {
		err = -EINVAL;
		goto end;
	}

	err = vfs_mount(driver, inode, dev, 0);

end:
	km_free_addr(path, pathlen);
	km_free_addr(type, typelen);
	if (dev)
		km_free_addr(dev, devlen);
	return (dword)err;
}

int32_t sys_open_std()
{
	dbg_printf(DBG_SC, "sys_open_std()\n");

	const char *tty = syscall_proc->tty;

	if (!tty) {
		// TODO: open /dev/null instead
		return 0;
	}

	struct inode *inode = vfs_lookup(tty, proc_cwd(syscall_proc));
	int n=0;

	struct fs_proc_data *data = syscall_proc->fs_data;
	if (!data->files[0]) {
		data->files[0] = vfs_open(inode, FSO_READ);
		n++;
	}

	if (!data->files[1]) {
		data->files[1] = vfs_open(inode, FSO_WRITE);
		n++;
	}

	if (!data->files[2]) {
		data->files[2] = vfs_open(inode, FSO_WRITE);
		n++;
	}

	return n;
}

void init_fs(void)
{
	dbg_printf(DBG_FS, "\nRegistering fs syscalls... ");

	syscall_register(SC_OPEN,  sys_open, 2);
	syscall_register(SC_CLOSE, sys_close, 1);
	syscall_register(SC_READ,  sys_readwrite, 3);
	syscall_register(SC_WRITE, sys_readwrite, 3);
	syscall_register(SC_ISATTY, sys_isatty, 1);

	syscall_register(SC_STAT, sys_stat, 2);

	syscall_register(SC_GETCWD, sys_getcwd, 2);
	syscall_register(SC_READDIR, sys_readdir, 3);

	syscall_register(SC_OPEN_STD, sys_open_std, 0);

	dbg_printf(DBG_FS, "done\n");

	//init_devfs();
}

