#include <sys/mltsas/mlt_sas.h>

#define Mlsas_Sig_Kill		SIGHUP

static int __Mlsas_Thread_Run_impl(Mlsas_thread_t *thi);
static void ____Mlsas_Thread_Stop(Mlsas_thread_t *thi, boolean_t wait);

void __Mlsas_Thread_Init(Mlsas_thread_t *thi,
	int (*fn) (Mlsas_thread_t *), const char *name)
{
	spin_lock_init(&thi->Mt_lock);
	thi->Mt_w    = NULL;
	thi->Mt_state = Mt_None;
	thi->Mt_fn = fn;
	thi->Mt_name = name;
}

void __Mlsas_Thread_Start(Mlsas_thread_t *thi)
{
	unsigned long flags;
	struct task_struct *nt = NULL

	spin_lock_irqsave(&thi->Mt_lock, flags);
	switch (thi->Mt_state) {
	case Mt_None:
		cmn_err(CE_NOTE, "%s Start Thread(%s).",
			__func__, thi->Mt_name);

		init_completion(&thi->Ml_stop);
		thi->Mt_state = Mt_Run;
		spin_unlock_irqrestore(&thi->Mt_lock, flags);
		
		flush_signals(current);
		
		VERIFY(!IS_ERR_OR_NULL(nt = kthread_create(
			__Mlsas_Thread_Run_impl,
			(void *)thi, thi->name)));
		thi->Mt_w = nt;
		wake_up_process(nt);
		break;
	case Mt_Exit:
		thi->Mt_state = Mt_Restart;
		cmn_err(CE_NOTE, "%s Restart Thread(%s)."
			__func__, thi->Mt_name);
		/* FULLTHROUGH */
	case Mt_Run:
	case Mt_Restart:
	default:
		spin_unlock_irqrestore(&thi->Mt_lock, flags);
		break;
	}
}

void __Mlsas_Thread_Stop(Mlsas_thread_t *thi)
{
	____Mlsas_Thread_Stop(thi, B_TRUE);
}

void __Mlsas_Thread_Stop_nowait(Mlsas_thread_t *thi)
{
	____Mlsas_Thread_Stop(thi, B_FALSE);
}

static void ____Mlsas_Thread_Stop(Mlsas_thread_t *thi, boolean_t wait)
{
	unsigned long flags;

	spin_lock_irqsave(&thi->Mt_lock, flags);
	switch (thi->Mt_state) {
	case Mt_Restart:
	case Mt_Run:
		thi->Mt_state = Mt_Exit;
		init_completion(&thi->Ml_stop);
		if (thi->Mt_w && thi->Mt_w != current)
			force_sig(Mlsas_Sig_Kill, thi->Mt_w);
		/* FULLTHROUGH */
	case Mt_Exit:
	case Mt_None:
		spin_unlock_irqrestore(&thi->Mt_lock, flags);
		break;
	}

	if (wait)
		wait_for_completion(&thi->Ml_stop);
}

static int __Mlsas_Thread_Run_impl(Mlsas_thread_t *thi)
{
	int rval;
	unsigned long flags;

again:
	rval = thi->Mt_fn(thi);
	
	spin_lock_irqsave(&thi->Mt_lock, flags);
	if (thi->Mt_state == Mt_Restart) {
		thi->Mt_state = Mt_Run;
		spin_unlock_irqrestore(&thi->Mt_lock, flags);
		cmn_err(CE_NOTE, "%s Restarting Thread(%s)",
			__func__, thi->Mt_name);
		goto again;
	}

	thi->Mt_state = Mt_None;
	thi->Mt_w = NULL;
	complete_all(&thi->Ml_stop);
	spin_unlock_irqrestore(&thi->Mt_lock, flags);
}