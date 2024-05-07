#include <linux/cred.h>
#include <linux/module.h>
#include <linux/moduleparam.h>
#include <linux/pid.h>
#include <linux/pid_namespace.h>
#include <linux/printk.h>
#include <linux/sched.h>
#include <linux/uidgid.h>
#include <linux/kernel.h>
#include <linux/sched/signal.h>


static pid_t arg_pid=0;
static pid_t arg_gid=0;
static pid_t arg_uid=0;
static char *arg_act="add";

module_param(arg_pid, int, 0);
MODULE_PARM_DESC(arg_pid, "PID of the process");
module_param(arg_uid, int, 0);
MODULE_PARM_DESC(arg_uid, "UID of the user for setting all his processes");
module_param(arg_gid, int, 0);
MODULE_PARM_DESC(arg_gid, "GID of the group to add to PID's supplimentary groups or UID to set to (for set_uid)");
module_param(arg_act, charp, 0);
MODULE_PARM_DESC(arg_act, "action to perform: add/remove/list/query/set_uid/set_gid");

struct pid *pid_struct = NULL;
struct task_struct *task = NULL;
struct cred *real_cd = NULL;
struct cred *effe_cd = NULL;
struct group_info *gi = NULL;
struct group_info *new_gi = NULL;

bool query(int gid, struct group_info *gi){
	int x;
	for(x=0; x<gi->ngroups; ++x)
		if(gid==gi->gid[x].val) return true;
	return false;
}

int do_single(void){
	if(task->real_cred==NULL){
		pr_info("Error: task->real_cred == NULL\n");
		return 0;
	}
	gi = task->real_cred->group_info;
	if(gi==NULL){
		pr_info("Error: task->real_cred->group_info == NULL\n");
		return 0;
	}

	if(!strcmp(arg_act, "add")){
		if(query(arg_gid, gi)){
			pr_info("GID %d is already in PID %d's supplementary group list\n", arg_gid, arg_pid);
			return 0;
		}
		new_gi = groups_alloc(gi->ngroups+1);
		if(new_gi==NULL){
			pr_info("Error: groups_alloc() failed, out of kernel memory?\n");
			return 0;
		}
		int x;
		for(x=0; x<gi->ngroups; ++x)
			new_gi->gid[x] = gi->gid[x];
		new_gi->gid[gi->ngroups].val = arg_gid;
		groups_sort(new_gi);

		// increment reference count
		get_cred((const struct cred *)new_gi);
		*(struct group_info**)(&task->real_cred->group_info) = new_gi;
		*(struct group_info**)(&task->cred->group_info) = new_gi;

		pr_info("Added GID %d to PID %d's supplementary groups\n", arg_gid, arg_pid);
	}else if(!strcmp(arg_act, "remove")){
		if(!query(arg_gid, gi)){
			pr_info("GID %d is not in PID %d's supplementary group list\n", arg_gid, arg_pid);
			return 0;
		}
		new_gi = groups_alloc(gi->ngroups-1);
		if(new_gi==NULL){
			pr_info("Error: groups_alloc() failed, out of kernel memory?\n");
			return 0;
		}
		int x,y;
		for(x=0,y=0; x<gi->ngroups; ++x)
			if(gi->gid[x].val != arg_gid){
				new_gi->gid[y] = gi->gid[x];
				y++;
			}

		// forcefully set group_info
		get_cred((const struct cred *)new_gi);
		*(struct group_info**)(&task->real_cred->group_info) = new_gi;
		*(struct group_info**)(&task->cred->group_info) = new_gi;

		pr_info("Removed GID %d from PID %d's supplementary groups\n", arg_gid, arg_pid);
	}else if(!strcmp(arg_act, "list")){
		pr_info("Listing PID %d's supplementary groups' GIDs: ", arg_pid);
		int x;
		for(x=0; x<gi->ngroups; ++x)
			pr_info("%d ", gi->gid[x].val);
		pr_info("Done\n");
	}else if(!strcmp(arg_act, "query")){
		pr_info("GID %d is %s PID %d's supplementary group list\n", arg_gid, query(arg_gid, gi)?"in":"not in", arg_pid);
	}else if(!strcmp(arg_act, "set_uid")){
		((kuid_t*)(&task->real_cred->uid))->val = arg_gid;
		((kuid_t*)(&task->real_cred->suid))->val = arg_gid;
		((kuid_t*)(&task->real_cred->euid))->val = arg_gid;
		((kuid_t*)(&task->real_cred->fsuid))->val = arg_gid;
		if(task->real_cred != task->cred){
			((kuid_t*)(&task->cred->uid))->val = arg_gid;
			((kuid_t*)(&task->cred->suid))->val = arg_gid;
			((kuid_t*)(&task->cred->euid))->val = arg_gid;
			((kuid_t*)(&task->cred->fsuid))->val = arg_gid;
		}
		pr_info("Changed UID to %d for PID %d\n", arg_gid, arg_pid);
	}else if(!strcmp(arg_act, "set_gid")){
		((kuid_t*)(&task->real_cred->gid))->val = arg_gid;
		((kuid_t*)(&task->real_cred->sgid))->val = arg_gid;
		((kuid_t*)(&task->real_cred->egid))->val = arg_gid;
		((kuid_t*)(&task->real_cred->fsgid))->val = arg_gid;
		if(task->real_cred != task->cred){
			((kuid_t*)(&task->cred->gid))->val = arg_gid;
			((kuid_t*)(&task->cred->sgid))->val = arg_gid;
			((kuid_t*)(&task->cred->egid))->val = arg_gid;
			((kuid_t*)(&task->cred->fsgid))->val = arg_gid;
		}
		pr_info("Changed GID to %d for PID %d\n", arg_gid, arg_pid);
	}
	return 1;
}

int init_module(void) {
	if(arg_pid){
		pid_struct = find_get_pid(arg_pid);
		if(pid_struct==NULL){
			pr_info("Error: find_get_pid() failed\n");
			return EEXIST;
		}
		task = pid_task(pid_struct, PIDTYPE_PID);
		if(task==NULL){
			pr_info("Error: pid_task() failed\n");
			return EEXIST;
		}
		do_single();
	}else if(arg_uid){
		int n=0, N=0;
		for_each_process(task) {
			if(task->real_cred->uid.val == arg_uid){
				arg_pid = task->pid;
				N += 1;
				n += do_single();
			}
		}
		pr_info("chown-pid: in total, %d/%d processes belonging to UID %d are set successfully\n", n, N, arg_uid);
	}else{
		pr_info("Error: Usage: insmod chown-pid.ko arg_pid/arg_uid=## arg_gid=## (arg_act='add/remove/list/query/set_uid/set_gid') && rmmod chown-pid\n");
	}

    return -EEXIST;	// always return error to avoid running `rmmod` every time
}

void cleanup_module(void){}
MODULE_LICENSE("GPL");

