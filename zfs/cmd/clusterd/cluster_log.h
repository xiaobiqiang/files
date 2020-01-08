#ifndef	_CLUSTER_LOG_H
#define	_CLUSTER_LOG_H

#ifdef	__cplusplus
extern "C" {
#endif
extern void cluster_log_init(const char *execname);
extern void cluster_log_framework(int severity, const char *format, ...);

#define	c_log	cluster_log_framework

#ifdef	__cplusplus
}
#endif

#endif	/* _CLUSTER_LOG_H */
