#ifndef _TOPO_FRUHASH_H
#define _TOPO_FRUHASH_H


#ifdef __cplusplus
extern "C" {
#endif


#define TOPO_FRUHASH_BUCKETS 100


struct topo_fru {
	struct topo_fru *tf_next;	/* Next module in hash chain */
	char *tf_name;			/* Basename of module */
	time_t tf_time;			/* Full pathname of module file */
	int		tf_status;
	int		tf_ignore;
	uint32_t	err_count;
	uint32_t	nor_count;
	char		*slotid;
	char		*encid;
	char		*diskname;
	char		*product;
        int 		flag;
};

struct topo_fruhash {
	pthread_mutex_t fh_lock;	/* hash lock */
	struct topo_fru **fh_hash;	/* hash bucket array */
	uint_t fh_hashlen;		/* size of hash bucket array */
	uint_t fh_nelems;		/* number of modules in hash */
};

typedef struct topo_fru topo_fru_t;
typedef struct topo_fruhash topo_fruhash_t;

topo_fruhash_t *topo_get_fruhash(void);
topo_fru_t * topo_fru_hash_clear_flag(const char *name);
topo_fru_t *topo_fru_setime(const char *name, int status, char *diskname,
		char *slotid, char *encid, char *product);
topo_fru_t *topo_fru_hash_lookup(const char *name);
topo_fru_t *topo_fru_cleartime(const char *name, int status);
void topo_fru_hash_create(void);
void topo_fru_hash_destroy(void);
void cm_alarm_cmd(char *buf,int size,const char *format, ...);

typedef enum cm_alarm_id {
	
	AMARM_ID_RESERVE = 0,

	/* disk */
	AMARM_ID_DISK_LOST,			/*  1  */
	AMARM_ID_DISK_RECOVER,		/*  2  */
	
	AMARM_ID_SLOW_DISK = 21,	/*  21 */
	AMARM_ID_DERR_DISK,			/*  22 */
	AMARM_ID_MERR_DISK,			/*  23  */
	AMARM_ID_NORESPONSE_DISK,	/*  24  */
	AMARM_ID_PREDICTURE_DISK,	/*  25  */
	AMARM_ID_OVERTEMP_DISK,		/*  26  */
	AMARM_ID_SELFTEST_FAIL_DISK,/*  27  */

	/* sensor */
	AMARM_ID_PSU,			/*  28 */
	AMARM_ID_FAN,			/*  29 */
	AMARM_ID_LINK,			/*  30 */
	AMARM_ID_CPU_TEMP,		/*  31 */
	AMARM_ID_CPU_CORE,		/*  32 */
	AMARM_ID_CPU_DIMM,		/*  33 */
	AMARM_ID_SYS_TEMP,		/*  34 */
	AMARM_ID_P_DIMM,		/*  35 */

	/* link */
	AMARM_ID_FC_LINK,		/*  36 */
	AMARM_ID_ETH_LINK,		/*  37 */
	AMARM_ID_HEART_LINK,	/*  38 */
	AMARM_ID_SAS_LINK,		/*  39 */

	/* pool */
	AMARM_ID_POOL_STATE,	/*  40 */

	/* quota */
	AMARM_ID_USER_QUOTA,	/*  41 */
	AMARM_ID_QUOTA,			/*  42 */

	/* thlin lun */
	AMARM_ID_POOL_THINLUN,	/*  43 */
	AMARM_ID_ZFS_THINLUN,	/*  44 */

	/* AVS */
	AMARM_ID_AVS,			/*  45 */
	
	AMARM_ID_ERROR,			/*  46 */
	/* release pool */

    AMARM_ID_POOL_RELEASE_FAIL_DOWN_IP, /*  47 */ 
    AMARM_ID_POOL_RELEASE_FAIL_UP_IP,   /*  48 */
    AMARM_ID_POOL_RELEASE_FAIL_IMPROT,  /*  49 */
    AMARM_ID_POOL_RELEASE_FAIL_EXPROT,  /*  50 */
    /* fail switch release pool */
    AMARM_ID_FPOOL_RELEASE_FAIL_IMPROT, /*  51 */

    AMARM_ID_CLUSTER_LINK_ERR,  /*  52 */
    AMARM_ID_ZFS_IS_DEADLOCK,   /*  53 */

	AMARM_ID_POOL_SCRUB,   /*  54 */
	AMARM_ID_DIR_QUOTA = 55,   /*  54 */

	
	AMARM_ID_RPC_FLCT_OPEN = 56,   /*  56 */
	AMARM_ID_RPC_FLCT_CLOSE, 
    
    /* EVENT ID */
    AMARM_ID_POOL_RELEASE_SUCCESS = 40000002,
    AMARM_ID_FPOOL_RELEASE_SUCCESS = 40000003
} cm_alarm_id_t;

#define	CM_ALARM_CMD_T "ceres_cmd alarm "
#define CM_ALARM_CMD_LEN 1024


#ifdef	__cplusplus
}
#endif

#endif		
