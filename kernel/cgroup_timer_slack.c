#include <linux/cgroup.h>
#include <linux/slab.h>
#include <linux/err.h>

struct cgroup_subsys timer_slack_subsys;
struct tslack_cgroup {
	struct cgroup_subsys_state css;
	unsigned long min_slack_ns;
};

static struct tslack_cgroup *cgroup_to_tslack(struct cgroup *cgroup)
{
	struct cgroup_subsys_state *css;

	css = cgroup_subsys_state(cgroup, timer_slack_subsys.subsys_id);
	return container_of(css, struct tslack_cgroup, css);
}

static struct cgroup_subsys_state *tslack_create(struct cgroup_subsys *subsys,
		struct cgroup *cgroup)
{
	struct tslack_cgroup *tslack_cgroup;

	tslack_cgroup = kmalloc(sizeof(*tslack_cgroup), GFP_KERNEL);
	if (!tslack_cgroup)
		return ERR_PTR(-ENOMEM);

	if (cgroup->parent) {
		struct tslack_cgroup *parent = cgroup_to_tslack(cgroup->parent);
		tslack_cgroup->min_slack_ns = parent->min_slack_ns;
	} else
		tslack_cgroup->min_slack_ns = 0UL;

	return &tslack_cgroup->css;
}

static void tslack_destroy(struct cgroup_subsys *tslack_cgroup,
		struct cgroup *cgroup)
{
	kfree(cgroup_to_tslack(cgroup));
}

static u64 tslack_read_min(struct cgroup *cgroup, struct cftype *cft)
{
	return cgroup_to_tslack(cgroup)->min_slack_ns;
}

static int tslack_write_min(struct cgroup *cgroup, struct cftype *cft, u64 val)
{
	struct cgroup *cur;

	if (val > ULONG_MAX)
		return -EINVAL;

	/* the min timer slack value should be more or equal than parent's */
	if (cgroup->parent) {
		struct tslack_cgroup *parent = cgroup_to_tslack(cgroup->parent);
		if (parent->min_slack_ns > val)
			return -EPERM;
	}

	cgroup_to_tslack(cgroup)->min_slack_ns = val;

	/* update children's min slack value if needed */
	list_for_each_entry(cur, &cgroup->children, sibling) {
		struct tslack_cgroup *child = cgroup_to_tslack(cur);
		if (val > child->min_slack_ns)
			tslack_write_min(cur, cft, val);
	}

	return 0;
}

static struct cftype files[] = {
	{
		.name = "min_slack_ns",
		.read_u64 = tslack_read_min,
		.write_u64 = tslack_write_min,
	}
};

static int tslack_populate(struct cgroup_subsys *subsys, struct cgroup *cgroup)
{
	return cgroup_add_files(cgroup, subsys, files, ARRAY_SIZE(files));
}

struct cgroup_subsys timer_slack_subsys = {
	.name		= "timer_slack",
	.subsys_id	= timer_slack_subsys_id,
	.create		= tslack_create,
	.destroy	= tslack_destroy,
	.populate	= tslack_populate,
};

unsigned long get_task_timer_slack(struct task_struct *tsk)
{
	struct cgroup_subsys_state *css;
	struct tslack_cgroup *tslack_cgroup;
	unsigned long ret;

	rcu_read_lock();
	css = task_subsys_state(tsk, timer_slack_subsys.subsys_id);
	tslack_cgroup = container_of(css, struct tslack_cgroup, css);
	ret = max(tsk->timer_slack_ns, tslack_cgroup->min_slack_ns);
	rcu_read_unlock();

	return ret;
}
