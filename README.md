# A Linux kernel module to modify a running process' supplementary group list

Linux is a very powerful multi-user operating system in which users can run programs both locally (as a PC) or remotely (as a server). In a typical multi-user environment, very often, we encounter the group-permission-not-updated issue for already launched processeses when we create a new group and added some existing users to the new group. That is because the Linux kernel credential module has a major security defect that if a user is added to or removed from a supplementary group (not the user's primary group defined in `/etc/passwd`), that user's existing processes will not have its supplementary group permission updated.

For example, if a user has a `tmux` (or `screen`) session running some background jobs, and the user is added to a new group whose members have access to some secure folder, then the user will not be able to access the secure folder in his/her `tmux` session, even if he spawns new shell panes/windows in his `tmux` session or creates new `tmux` sessions. The only two workarounds are:
1. close all his `tmux` sessions so that his `tmux` server stops and then relaunch `tmux`.
2. create a new `tmux` session under a different `tmux` socket.

In another example, if a user quits a project and the system admin removes him from the project group (`/etc/group`), then all that user's existing programs and `tmux` sessions will continue to access files and folders that are only accessible by that project group members. Moreover, the user can use his existing `tmux` (or `screen`) session to spawn new processes (including new `tmux` sessions) that continue to enjoy the old supplementary group permission, so that he can continue to access those files and folders on which his permission has been withdrawn. This can lead to a data security problem.

Since this defect lies in the Linux kernel, up to today, no user-level programs and softwares can tackle it directly.

This kernel-module program is created to add or remove a supplementary group (by GID) to and from a process (by PID).

## How to compile
1. Login as root (e.g., `sudo su` or `su`)
2. `cd` into the project folder and run `make`
3. If compilation is successful, `supgroup.ko` will be generated
4. `sudo make` will NOT work (unless you append the option `Defaults env_keep += "PWD"` to `/etc/sudoers` which is not secure)

## How to run
1. To add a supplementary group GID to a process PID:
```
sync && insmod supgroup.ko arg_pid=<PID> arg_gid=<GID> arg_act='add' && rmmod supgroup
```
in which `arg_act='add'` is optional (default argument).

2. To remove a supplementary group GID from a process PID:
```
sync && insmod supgroup.ko arg_pid=<PID> arg_gid=<GID> arg_act='remove' && rmmod supgroup
```

3. To enquire whether a GID is inside the process' supplementary group list:
```
sync && insmod supgroup.ko arg_pid=<PID> arg_gid=<GID> arg_act='query' && rmmod supgroup
```

4. To list all GIDs under the process PID:
```
sync && insmod supgroup.ko arg_pid=<PID> arg_act='list' && rmmod supgroup
```

You can also view this list at `/proc/<PID>/status`.

**Since this is a kernel module, all verbose output can only be seen by `dmesg -t | tail`.**