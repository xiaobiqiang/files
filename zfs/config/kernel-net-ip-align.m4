AC_DEFUN([ZFS_AC_SPEC_NET_IP_ALIGN], [
	AC_MSG_CHECKING([whether special net ip align is enabled])
	AC_ARG_ENABLE([spec-net-ip-align],
		[AS_HELP_STRING([--enable-spec-net-ip-align],
		[Enable special net ip align])],
		[],
		[enable_spec_net_ip_align=no])

	AS_IF([test "x$enable_spec_net_ip_align" = xyes],
	[
		AC_DEFINE(SPEC_NET_IP_ALIGN, 1, [special net ip align])
	])

	AC_MSG_RESULT([$enable_spec_net_ip_align])
])
