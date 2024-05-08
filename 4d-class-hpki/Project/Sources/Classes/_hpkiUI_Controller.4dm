Class extends _hpki_Controller

Class constructor($CLI : cs:C1710._CLI)
	
	Super:C1705($CLI)
	
Function onDataError($worker : 4D:C1709.SystemWorker; $params : Object)
	
	Super:C1706.onDataError($worker; $params)
	
Function onData($worker : 4D:C1709.SystemWorker; $params : Object)
	
	Super:C1706.onData($worker; $params)
	
Function onResponse($worker : 4D:C1709.SystemWorker; $params : Object)
	
	Super:C1706.onResponse($worker; $params)
	
	If (Form:C1466#Null:C1517)
		
		var $response : Object
		$response:=JSON Parse:C1218($worker.response; Is object:K8:27)
		
		If ($response.success)
			If ($response.readers#Null:C1517)
				Form:C1466.readers.values:=$response.readers
				If (Form:C1466.readers.values.length#0)
					Form:C1466.readers.index:=0
				End if 
			End if 
			
			For each ($prop; ["myNumber"; \
				"address"; "commonName"; "gender"; "dateOfBirth"; \
				"cardType"; \
				"version"; "subject"; "serialNumber"; "pem"; "der"; "issuer"; "notAfter"; "notBefore"; "digestInfo"; "signature"])
				If ($response[$prop]#Null:C1517)
					Form:C1466[$prop]:=$response[$prop]
				End if 
				
				If ($response.certificate[$prop]#Null:C1517)
					Form:C1466[$prop]:=$response.certificate[$prop]
				End if 
				
			End for each 
			
		End if 
		
	End if 
	
Function onTerminate($worker : 4D:C1709.SystemWorker; $params : Object)
	
	If (Form:C1466#Null:C1517)
		
		If (This:C1470.complete)
			
		End if 
		
	End if 