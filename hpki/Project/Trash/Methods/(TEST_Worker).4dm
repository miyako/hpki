//%attributes = {}
#DECLARE($params : Object)

If (Count parameters:C259=0)
	
	CALL WORKER:C1389(1; Current method name:C684; {})
	
Else 
	
	$hpki:=cs:C1710.hpki.new(cs:C1710._hpki_Controller)
	
End if 