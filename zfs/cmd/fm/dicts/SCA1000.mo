      �   �  `   ����           ��������             .      ��������G   z         ^   �   ��������z   A  SCA1000-8000-0V.type SCA1000-8000-0V.severity SCA1000-8000-0V.response SCA1000-8000-0V.impact SCA1000-8000-0V.description SCA1000-8000-0V.action Fault Major The module has been disabled and un-registered as a hardware provider to the Solaris Cryptographic Framework. The card will no longer function as a cryptographic accelerator. The Solaris Fault Manager has detected a hardware failure on the Sun Crypto Accelerator 1000 card.  Refer to %s for more information. Ensure that the board is securely installed on the system.

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