      �   �  `   ����           ��������             ,      ��������D   z         Z   �   ��������u   @  SCA500-8000-0D.type SCA500-8000-0D.severity SCA500-8000-0D.response SCA500-8000-0D.impact SCA500-8000-0D.description SCA500-8000-0D.action Fault Major The module has been disabled and un-registered as a hardware provider to the Solaris Cryptographic Framework. The card will no longer function as a cryptographic accelerator. The Solaris Fault Manager has detected a hardware failure on the Sun Crypto Accelerator 500 card.  Refer to %s for more information. Ensure that the board is securely installed on the system.

Use the cryptoadm(1M) command

cryptoadm list -p


to check whether 'dca/x' (where x is a number) is listed under 'kernel hardware
providers'. If so, run diagnostics on the hardware provider using SUNVTS
following the procedures described in the Sun Crypto Accelerator 1000 Board
User's Guide.

Use the fmdump(1M) command

fmdump -vu event-id


to view the results of diagnosis and the specific Field Replaceable
Unit (FRU) identified for replacement.

The event-id can be found in the EVENT-ID field of the message.
For example:


EVENT-ID: 39b30371-f009-c76c-90ee-b245784d2277
 