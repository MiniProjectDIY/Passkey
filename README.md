# Passkey
use your finterprint to send password via virtual keyboard

# ME
Moduleholder: using screw pillar M2*4*1.5+2.5*1, M2*7 screws
Passkey Case: using 4xM2(screw diameter)*3(Length)*3.5(outter diameter) screw inserts,M2*8 screws

# SW
using project:passkey_tune_and_set to :
1.get module sn
2.register your finterprint
3.change module address(do this at last)

using python project PassKey_utils to encrypt your password with your module sn

using project passkey_final_run to run as final code,
remember to change the module address,the macro define and the encrypted password per your own condition.

# HW
using folder production for PCB/SMT process,or handsolder by yourself.
![photo_2025-08-31_11-42-20](https://github.com/user-attachments/assets/3c6aa11a-e135-4779-8811-b63279ef5f80)
