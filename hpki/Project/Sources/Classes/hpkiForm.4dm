Class extends pkiForm

Class constructor
	
	Super:C1705()
	
	$window:=Open form window:C675("hpki")
	DIALOG:C40("hpki"; This:C1470; *)
	
Function onLoad()
	
	Super:C1706.onLoad()
	
	Form:C1466.data:=""
	Form:C1466.医師会カード:=True:C214
	
Function getData() : Text
	
	return (OBJECT Get name:C1087(Object with focus:K67:3)="data") ? Get edited text:C655 : Form:C1466.data
	
Function onAfterEdit()
	
	OBJECT SET ENABLED:C1123(*; "@で署名"; Form:C1466.getData()#"")
	
	return Form:C1466