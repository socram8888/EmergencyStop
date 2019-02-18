
#include "EmergencyButton.au3"
#include <MsgBoxConstants.au3>
#include <AutoItConstants.au3>

TraySetToolTip("Emergency button screen lock")

While True
   _EmergencyButton_WaitStatus(0)
   _EmergencyButton_WaitStatus(1)

   DllCall("user32.dll", "bool", "LockWorkStation")
WEnd
