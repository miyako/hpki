Class extends _Form

property data : Variant
property 医師会カード : Boolean

Class constructor
	
	Super:C1705()
	
Function onLoad()
	
	Form:C1466.hpki:=cs:C1710.hpki.new(cs:C1710._hpkiUI_Controller)
	
	Form:C1466.readers:={values: []; index: -1; currentValue: Null:C1517}
	
	Form:C1466.data:=Null:C1517
	Form:C1466.医師会カード:=False:C215
	
	Form:C1466.list()
	Form:C1466.getSecrets()
	
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
	
	//ここに4桁暗証番号を保存している前提
	Form:C1466.hpkiFile:=Folder:C1567(fk home folder:K87:24).file("hpki")
	
	OBJECT SET ENABLED:C1123(*; "@で署名"; False:C215)
	
	return Form:C1466
	
Function clear()
	
	For each ($prop; ["myNumber"; "address"; "commonName"; "gender"; "dateOfBirth"; "cardType"; "version"; "subject"; "serialNumber"; "pem"; "der"; "issuer"; "notAfter"; "notBefore"; "digestInfo"; "signature"])
		Form:C1466[$prop]:=Null:C1517
	End for each 
	
	return Form:C1466
	
Function obscure($in : Variant) : Text
	
	return Value type:C1509($in)=Is text:K8:3 ? "*"*Length:C16($in) : ""
	
Function getData() : Variant
	
	return Null:C1517  //virtual
	
Function readidentitycertificate()
	
	$pin4:=Form:C1466.pin4File.exists ? Form:C1466.pin4File.getText() : Null:C1517
	
	Form:C1466.hpki.cert_i({pin4: $pin4; reader: Form:C1466.readers.currentValue})
	
	return Form:C1466
	
Function readsignaturecertificate()
	
	$pin6:=Form:C1466.pin4File.exists ? Form:C1466.pin6File.getText() : Null:C1517
	
	Form:C1466.hpki.cert_s({pin6: $pin6; reader: Form:C1466.readers.currentValue})
	
	return Form:C1466
	
Function signwithidentity()
	
	If (Form:C1466.医師会カード)
		$pin4:=Form:C1466.hpkiFile.exists ? Form:C1466.hpkiFile.getText() : Null:C1517
	Else 
		$pin4:=Form:C1466.pin4File.exists ? Form:C1466.pin4File.getText() : Null:C1517
	End if 
	
	Form:C1466.hpki.sign_i({pin4: $pin4; reader: Form:C1466.readers.currentValue; file: Form:C1466.getData()})
	
	return Form:C1466
	
Function signwithsignature()
	
	$pin6:=Form:C1466.pin6File.exists ? Form:C1466.pin6File.getText() : Null:C1517
	
	If (Form:C1466.医師会カード)
		$pin4:=Form:C1466.hpkiFile.exists ? Form:C1466.hpkiFile.getText() : Null:C1517
	Else 
		$pin4:=Form:C1466.pin4File.exists ? Form:C1466.pin4File.getText() : Null:C1517
	End if 
	
	Form:C1466.hpki.sign_s({pin4: $pin4; pin6: $pin6; reader: Form:C1466.readers.currentValue; file: Form:C1466.getData()})
	
	return Form:C1466
	
Function readmynumber()
	
	$pin4:=Form:C1466.pin4File.exists ? Form:C1466.pin4File.getText() : Null:C1517
	
	Form:C1466.hpki.mynumber({pin4: $pin4; reader: Form:C1466.readers.currentValue})
	
	return Form:C1466