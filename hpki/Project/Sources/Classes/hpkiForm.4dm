Class extends _Form

property targetFolder : 4D:C1709.Folder

Class constructor
	
	Super:C1705()
	
	This:C1470.targetFolder:=Folder:C1567(fk desktop folder:K87:19)
	
	$window:=Open form window:C675("hpki")
	DIALOG:C40("hpki"; This:C1470; *)
	
Function onLoad()
	
	Form:C1466.hpki:=cs:C1710.hpki.new(cs:C1710._hpkiUI_Controller)
	
	Form:C1466.readers:={values: []; index: -1; currentValue: Null:C1517}
	
	Form:C1466.list().getSecrets()
	
Function onUnload()
	
	Form:C1466.hpki.terminate()
	
Function list()
	
	Form:C1466.hpki.list()
	
	return Form:C1466
	
Function getSecrets()
	
	//ここに4桁暗証番号を保存している前提
	Form:C1466.pin4File:=Folder:C1567(fk home folder:K87:24).file("pin4")
	
	//ここに6+桁暗証番号を保存している前提
	Form:C1466.pin6File:=Folder:C1567(fk home folder:K87:24).file("pin6")
	
	OBJECT SET ENABLED:C1123(*; "@で署名"; False:C215)
	
	return Form:C1466
	
Function clear()
	
	For each ($prop; ["myNumber"; "address"; "commonName"; "gender"; "dateOfBirth"; "cardType"; "version"; "subject"; "serialNumber"; "pem"; "der"; "issuer"; "notAfter"; "notBefore"; "digestInfo"; "signature"])
		Form:C1466[$prop]:=Null:C1517
	End for each 
	
	return Form:C1466
	
Function obscure($in : Variant) : Text
	
	return Value type:C1509($in)=Is text:K8:3 ? "*"*Length:C16($in) : ""
	
Function readidentitycertificate()
	
	$pin4:=Form:C1466.pin4File.exists ? Form:C1466.pin4File.getText() : Null:C1517
	
	Form:C1466.hpki.cert_i({pin4: $pin4; reader: Form:C1466.readers.currentValue})
	
	return Form:C1466
	
Function readsignaturecertificate()
	
	$pin6:=Form:C1466.pin4File.exists ? Form:C1466.pin6File.getText() : Null:C1517
	
	Form:C1466.hpki.cert_s({pin6: $pin6; reader: Form:C1466.readers.currentValue})
	
	return Form:C1466
	
Function signwithidentity()
	
	$pin4:=Form:C1466.pin4File.exists ? Form:C1466.pin4File.getText() : Null:C1517
	
	If (Macintosh command down:C546)
		$pin4:="9999"  //hpkiテストカード
	End if 
	
	Form:C1466.hpki.sign_i({pin4: $pin4; reader: Form:C1466.readers.currentValue; file: Form:C1466.source})
	
	return Form:C1466
	
Function signwithsignature()
	
	$pin4:=Form:C1466.pin4File.exists ? Form:C1466.pin4File.getText() : Null:C1517
	$pin6:=Form:C1466.pin6File.exists ? Form:C1466.pin6File.getText() : Null:C1517
	
	If (Macintosh command down:C546)
		$pin4:="9999"  //hpkiテストカード
	End if 
	
	Form:C1466.hpki.sign_s({pin4: $pin4; pin6: $pin6; reader: Form:C1466.readers.currentValue; file: Form:C1466.source})
	
	return Form:C1466
	
Function readmynumber()
	
	$pin4:=Form:C1466.pin4File.exists ? Form:C1466.pin4File.getText() : Null:C1517
	
	Form:C1466.hpki.mynumber({pin4: $pin4; reader: Form:C1466.readers.currentValue})
	
	return Form:C1466
	
Function onSourceDragOver()
	
	$path:=Get file from pasteboard:C976(1)
	
	If ($path#"") && (Test path name:C476($path)=Is a document:K24:1)
		$0:=0
	Else 
		$0:=-1
	End if 
	
Function onSourceDrop()
	
	$path:=Get file from pasteboard:C976(1)
	
	var $class : 4D:C1709.Class
	
	Case of 
		: (Test path name:C476($path)=Is a document:K24:1)
			$class:=4D:C1709.File
	End case 
	
	If ($class#Null:C1517)
		
		Form:C1466.source:=$class.new($path; fk platform path:K87:2)
		
		If (Is macOS:C1572)
			Form:C1466.sourceIcon:=Form:C1466.source.getIcon()
		Else 
			$icon:=Form:C1466.source.getIcon(256)
			Form:C1466.sourceIcon:=$icon
		End if 
		
	End if 
	
	OBJECT SET ENABLED:C1123(*; "@で署名"; Form:C1466.source#Null:C1517)
	
	return Form:C1466