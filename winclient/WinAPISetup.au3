
#include-once

#Include <WinAPIConstants.au3>

Global $__g_hSetupApiDllHandle = DllOpen("Setupapi.dll")

Global Const $tagSP_DEVICE_INTERFACE_DATA = "DWORD cbSize;STRUCT;DWORD InterfaceClassGuidData1;WORD InterfaceClassGuidData2;WORD InterfaceClassGuidData3;BYTE InterfaceClassGuidData4[8];ENDSTRUCT;DWORD Flags;ULONG_PTR Reserved";

Global Const $DIGCF_DEFAULT = 1
Global Const $DIGCF_PRESENT = 2
Global Const $DIGCF_ALLCLASSES = 4
Global Const $DIGCF_PROFILE = 8
Global Const $DIGCF_DEVICEINTERFACE = 16

Func _SetupAPI_SetupDiGetClassDevs($tClassGuid, $sEnumerator, $hHwndParent, $iFlags)
   $vRet = DllCall($__g_hSetupApiDllHandle, "HANDLE", "SetupDiGetClassDevsW", "PTR", DllStructGetPtr($tClassGuid), "WSTR", $sEnumerator, "HWND", $hHwndParent, "DWORD", $iFlags)
   If @error Then
	  Return Null
   EndIf

   $hDeviceInfoSet = $vRet[0]
   If $hDeviceInfoSet == $INVALID_HANDLE_VALUE Then
	  return SetError(1, 0, Null)
   EndIf

   Return $hDeviceInfoSet
EndFunc

Func _SetupAPI_SetupDiEnumDeviceInterfaces($hDeviceInfoSet, $tDeviceInfoData, $hInterfaceClassGuid, $iMemberIndex, $tDeviceInterfaceData)
   $vRet = DllCall($__g_hSetupApiDllHandle, "boolean", "SetupDiEnumDeviceInterfaces", "handle", $hDeviceInfoSet, "ptr", DllStructGetPtr($tDeviceInfoData), "ptr", DllStructGetPtr($hInterfaceClassGuid), "dword", $iMemberIndex, "ptr", DllStructGetPtr($tDeviceInterfaceData))
   If @error Then
	  Return Null
   EndIf

   Return $vRet[0]
EndFunc

Func _SetupAPI_SetupDiGetDeviceInterfaceDetail($hDeviceInfoSet, $tDeviceInterfaceData, $tDeviceInfoData)
   $vRet = DllCall($__g_hSetupApiDllHandle, "boolean", "SetupDiGetDeviceInterfaceDetailW", "handle", $hDeviceInfoSet, "ptr", DllStructGetPtr($tDeviceInterfaceData), "ptr", Null, "dword", 0, "dword*", 0, "ptr", DllStructGetPtr($tDeviceInfoData))
   If @error Then
	  Return Null
   EndIf

   ; Subtract 4 (cbSize) and divide by 2 (each wchar is two bytes)
   $iRequiredSize = $vRet[5]
   $iPathLen = ($iRequiredSize - 4) / 2
   $tDeviceInterfaceDetailData = DllStructCreate("dword cbSize; align 0; wchar DevicePath[" & $iPathLen  & "]")
   DllStructSetData($tDeviceInterfaceDetailData, "cbSize", 6)

   $vRet = DllCall($__g_hSetupApiDllHandle, "boolean", "SetupDiGetDeviceInterfaceDetailW", "handle", $hDeviceInfoSet, "ptr", DllStructGetPtr($tDeviceInterfaceData), "ptr", DllStructGetPtr($tDeviceInterfaceDetailData), "dword", $iRequiredSize, "ptr", Null, "ptr", DllStructGetPtr($tDeviceInfoData))
   If @error Then
	  Return Null
   EndIf

   Return $tDeviceInterfaceDetailData
EndFunc
