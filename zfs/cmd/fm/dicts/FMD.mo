   ,   �  f  �  ��������                  ;   ����   1   �   ��������B   �         W     ����   l   a  ��������   �        �     ����	   �   X  ���������   _        �   e  ���������   �        �   !  ����     �  ��������#  �        4  �  ����   I  �  ��������^  Y        q  �  ����   �    ���������  T  
       �  [  ���������  a        �  �  ����   �  &  ��������  �          %  ����   &  ,  ��������;  2        P  �  ����   c  �  ��������{  ~     &   �  �  ����"   �  �  ���������  �  !   $   �  2	  ����%   �  �	  ���������  ,
  #   )     �
  ����(     �
  ��������-  �
  '   *   B    ����+   U  T  ��������m  �  syslog-msgs-pointer syslog-msgs-message-template FMD-8000-6U.type FMD-8000-6U.severity FMD-8000-6U.response FMD-8000-6U.impact FMD-8000-6U.description FMD-8000-6U.action FMD-8000-58.type FMD-8000-58.severity FMD-8000-58.response FMD-8000-58.impact FMD-8000-58.description FMD-8000-58.action FMD-8000-4M.type FMD-8000-4M.severity FMD-8000-4M.response FMD-8000-4M.impact FMD-8000-4M.description FMD-8000-4M.action FMD-8000-3F.type FMD-8000-3F.severity FMD-8000-3F.response FMD-8000-3F.impact FMD-8000-3F.description FMD-8000-3F.action FMD-8000-2K.type FMD-8000-2K.severity FMD-8000-2K.response FMD-8000-2K.impact FMD-8000-2K.description FMD-8000-2K.action FMD-8000-11.type FMD-8000-11.severity FMD-8000-11.response FMD-8000-11.impact FMD-8000-11.description FMD-8000-11.action FMD-8000-0W.type FMD-8000-0W.severity FMD-8000-0W.response FMD-8000-0W.impact FMD-8000-0W.description FMD-8000-0W.action 
...
See fmadm faulty -u <EVENT-ID> for full information.
 SUNW-MSG-ID: %s, TYPE: %s, VER: 1, SEVERITY: %s
EVENT-TIME: %s
PLATFORM: %s, CSN: %s, HOSTNAME: %s
SOURCE: %s, REV: %s
EVENT-ID: %s
DESC: %s
AUTO-RESPONSE: %s
IMPACT: %s
REC-ACTION: %s
 Resolved Minor All system components offlined because of the original fault have been brought back online.
 Performance degradation of the system due to the original fault has been recovered.
 All faults associated with an event id have been addressed.
  Refer to %s for more information. Use fmdump -v -u <EVENT-ID> to identify the repaired components.
 Update Minor Some system components offlined because of the original fault may have been brought back online.
 Performance degradation of the system due to the original fault may have been recovered.
 Some faults associated with an event id have been addressed.
  Refer to %s for more information. Use fmadm faulty to identify the repaired components, and any suspects that still need to be repaired.
 Repair Minor Some system components offlined because of the original fault may have been brought back online.
 Performance degradation of the system due to the original fault may have been recovered.
 All faults associated with an event id have been addressed.
  Refer to %s for more information. Use fmdump -v -u <EVENT-ID> to identify the repaired components. Defect Minor The module has been disabled.  Events destined for the module will be saved for manual diagnosis. Automated diagnosis and response for subsequent events associated with this module will not occur. A Solaris Fault Manager component could not load due to an erroroneous configuration file.  Refer to %s for more information. Use fmdump -v -u <EVENT-ID> to locate the module.  Use fmadm load <module> to load the module after repairing its configuration. Defect Minor The module has been disabled.  Events destined for the module will be saved for manual diagnosis. Automated diagnosis and response for subsequent events associated with this module will not occur. A Solaris Fault Manager component has experienced an error that required the module to be disabled.  Refer to %s for more information. Use fmdump -v -u <EVENT-ID> to locate the module.  Use fmadm reset <module> to reset the module. Defect Minor The diagnosis has been saved in the fault log for examination by Sun. The fault log will need to be manually examined using fmdump(1M) in order to determine if any human response is required. A Solaris Fault Manager component generated a diagnosis for which no message summary exists.  Refer to %s for more information. Use fmdump -v -u <EVENT-ID> to view the diagnosis result.  Run pkgchk -n SUNWfmd to ensure that fault management software is installed properly. Defect Minor Error reports from the component will be logged for examination by Sun. Automated diagnosis and response for these events will not occur. The Solaris Fault Manager received an event from a component to which no automated diagnosis software is currently subscribed.  Refer to %s for more information. Run pkgchk -n SUNWfmd to ensure that fault management software is installed properly.  Contact Sun for support. 