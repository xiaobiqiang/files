   0   �  :     ����           ��������             (      ����   >   �   ��������R   �         k     ����      }  ���������   �     	   �   �  ����
   �     ���������   I        �   �  ����   �   �  ��������  �        &  �  ����   <  �  ��������P          i  �  ����   }  �  ���������  �        �  �  ����   �  !  ���������  �     #   �  k  ����   �  �  ��������  �        $  �  ����   :    ��������N  R         g  �  ����   {  u  ���������  {     !   �  �  ����"   �  �  ���������  �     )   �  �  ����%   �  V	  ��������  \	  $   '   "  b	  ����(   8  i	  ��������L  �	  &   ,   e  k
  ����+   y  �
  ���������  �
  *   .   �    ���������    -   /   �  ^  ���������  �  DISK-8000-74.type DISK-8000-74.severity DISK-8000-74.response DISK-8000-74.impact DISK-8000-74.description DISK-8000-74.action DISK-8000-6R.type DISK-8000-6R.severity DISK-8000-6R.response DISK-8000-6R.impact DISK-8000-6R.description DISK-8000-6R.action DISK-8000-5C.type DISK-8000-5C.severity DISK-8000-5C.response DISK-8000-5C.impact DISK-8000-5C.description DISK-8000-5C.action DISK-8000-4Q.type DISK-8000-4Q.severity DISK-8000-4Q.response DISK-8000-4Q.impact DISK-8000-4Q.description DISK-8000-4Q.action DISK-8000-3E.type DISK-8000-3E.severity DISK-8000-3E.response DISK-8000-3E.impact DISK-8000-3E.description DISK-8000-3E.action DISK-8000-2J.type DISK-8000-2J.severity DISK-8000-2J.response DISK-8000-2J.impact DISK-8000-2J.description DISK-8000-2J.action DISK-8000-12.type DISK-8000-12.severity DISK-8000-12.response DISK-8000-12.impact DISK-8000-12.description DISK-8000-12.action DISK-8000-0X.type DISK-8000-0X.severity DISK-8000-0X.response DISK-8000-0X.impact DISK-8000-0X.description DISK-8000-0X.action Fault Critical An attempt has been made to take the disk out of service. If the device is part of an active ZFS pool, the device still may be in service.
 Optionally degraded redundancy and performance.
 A disk device is experiencing too many slows IOs above its configured threshold.
 Determine the cause of the fault using fmdump -v -u <EVENT_ID> and schedule a repair service.
 Fault Critical An attempt has been made to take the disk out of service. If the device is part of an active ZFS pool, the device still may be in service.
 Optionally degraded redundancy and performance.
 A disk device has experienced too many device errors.
 Determine the cause of the fault using fmdump -v -u <EVENT_ID> and schedule a repair service.
 Fault Critical None.
 The device will not be usable.
 A disk device failed to attach. Either the device is broken, or it is just taking unusually long to respond. The latter is often the case with battery-backed RAM disk devices when the battery needs to be charged.
 Schedule a repair procedure to replace the affected device. Use 'fmadm faulty' to find the affected disk. If this is a battery-backed RAM disk or similar device, you can also try to reactivate the device using 'fmadm acquit' once the battery is charged
 Fault Critical The device may be offlined or degraded.
 It is likely that continued operation will result in data corruption, which may eventually cause the loss of service or the service degradation.
 The command was terminated with a non-recovered error condition that may have been caused by a flaw in the media or an error in the recorded data. 
  Refer to %s for more information. Schedule a repair procedure to replace the affected device. Use 'fmadm faulty' to find the affected disk.
 Fault Critical The device may be offlined or degraded.
 The device has failed. The service may have been lost or degraded.
 A non-recoverable hardware failure was detected by the device while performing a command.
  Refer to %s for more information. Ensure that the latest drivers and patches are installed. Schedule a repair procedure to replace the affected
device. Use 'fmadm faulty' to find the affected disk.
 Fault Critical None.
 The disk has failed.
 One or more disk self tests failed.
  Refer to %s for more information. Schedule a repair procedure to replace the affected disk.
Use fmdump -v -u <EVENT_ID> to identify the disk.
 Fault Major None.
 Performance degradation is likely and continued disk operation
beyond the temperature threshold can result in disk
damage and potential data loss.
 A disk's temperature exceeded the limits established by
its manufacturer.
  Refer to %s for more information. Ensure that the system is properly cooled, that all fans are
functional, and that there are no obstructions of airflow to the affected
disk.
 Fault Major None.
 It is likely that the continued operation of
this disk will result in data loss.
 SMART health-monitoring firmware reported that a disk
failure is imminent.
  Refer to %s for more information. Schedule a repair procedure to replace the affected disk.
Use fmdump -v -u <EVENT_ID> to identify the disk.
 