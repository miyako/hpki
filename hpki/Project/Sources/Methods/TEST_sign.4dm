//%attributes = {}
$hpki:=cs:C1710.hpki.new()
$slots:=$hpki.list()

If ($slots.success)
	$reader:=$slots.readers[0]
	$pin4:=Folder:C1567(fk home folder:K87:24).file("pin4").getText()
	$pin6:=Folder:C1567(fk home folder:K87:24).file("pin6").getText()
	$file:=Folder:C1567(Temporary folder:C486; fk platform path:K87:2).file(Generate UUID:C1066)
	$file.setText("abcde"; "utf-8-no-bom")
	$status:=$hpki.sign_s({pin4: $pin4; pin6: $pin6; reader: $reader; file: $file})
	If ($status.success)
		var $signature : Text
		var $data : Blob
		$data:=$status._signature
		BASE64 ENCODE:C895($data; $signature)
		SET TEXT TO PASTEBOARD:C523($signature)
/*
XpaLI0L0yPT+QSlL2inQam1MY6dx27VZfOgmX8DhcD/ESLlDkVYtaGxHDI53/q6
x6efvjWPqNKBdkNoDwWVTAeLCvAG7LkUIpkW/rAbdhHlnTRTztAgLXa7rsqD7FX
qyKdijtpgm3VvF/7Wq6K/2D4nvNzSr7UVbuSPdL2e5ghyCrEJ7ukmbAQ4BWQ3CQ
l3F1ox58JJCnnVbgygCM4J3g6F2DEcJvBR/KuqmtMFEXI/I34gdi56joeM3JopCL+S
ArsYA/qRWfiFAVGjPJr6Wg3dnPzx4VNr11aMmSBhqg/LCYDzD0n5ecgoPWPCujsvR
tf8Z5pSqp+Jg0Nv0OozSLQ==
*/
	End if 
End if 
