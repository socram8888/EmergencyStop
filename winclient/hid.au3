
#include <WinAPIFiles.au3>
#include "WinAPIHID.au3"
#include "WinAPISetup.au3"

Func _HIDListDevices()
   Local $aDevices[0]

   ; Get HID device info
   $hHidDevInfo = _SetupAPI_SetupDiGetClassDevs($HID_INTERFACE_GUID, Null, Null, BitOR($DIGCF_PRESENT, $DIGCF_DEVICEINTERFACE))

   ; Allocate device interface data
   $tDevData = DllStructCreate($tagSP_DEVICE_INTERFACE_DATA)
   DllStructSetData($tDevData, "cbSize", DllStructGetSize($tDevData))

   ; Iterate through all HID devices
   $iDevIdx = 0
   While _SetupAPI_SetupDiEnumDeviceInterfaces($hHidDevInfo, Null, $HID_INTERFACE_GUID, $iDevIdx, $tDevData)
	  ; Get required struct size. This call will fail but will update $iRequiredSize with required size
	  Local $iRequiredSize = 0
	  $tInterfaceDetail = _SetupAPI_SetupDiGetDeviceInterfaceDetail($hHidDevInfo, $tDevData, Null)

	  If $tInterfaceDetail <> Null Then
		 ReDim $aDevices[UBound($aDevices) + 1]
		 $aDevices[UBound($aDevices) - 1] = DllStructGetData($tInterfaceDetail, "DevicePath")
	  EndIf

	  $iDevIdx += 1
   WEnd

   Return $aDevices
EndFunc
