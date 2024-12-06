//%attributes = {}
$hpki:=cs:C1710.hpki.new()
$slots:=$hpki.list()

If ($slots.success)
	$reader:=$slots.readers[0]
	$pin4:=Folder:C1567(fk home folder:K87:24).file("pin4").getText()
	$status:=$hpki.mynumber({pin4: $pin4; reader: $reader})
End if 
