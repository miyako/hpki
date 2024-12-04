Class extends _CLI

Class constructor($controller : 4D:C1709.Class)
	
	Super:C1705("hpki"; $controller)
	
Function onResponse($files : Collection; $parameters : Collection)
	
Function terminate()
	
	This:C1470.controller.terminate()
	
Function _path($item : Object) : Text
	
	return OB Class:C1730($item).new($item.platformPath; fk platform path:K87:2).path
	
Function cert_i($params : Object)->$this : cs:C1710.hpki
	
	$this:=This:C1470
	
	var $commands; $options : Collection
	$commands:=[]
	
	$command:=This:C1470.escape(This:C1470._executablePath)+" --certificate identity"
	
	If ($params.pin4#Null:C1517) && (Value type:C1509($params.pin4)=Is text:K8:3)
		$command+=" --pin4 "+This:C1470.escape($params.pin4)
	End if 
	
	If ($params.reader#Null:C1517) && (Value type:C1509($params.reader)=Is text:K8:3)
		$command+=" --reader "+This:C1470.escape($params.reader)
		$commands.push($command)
	End if 
	
	This:C1470.controller.execute($commands)
	
Function cert_s($params : Object)->$this : cs:C1710.hpki
	
	$this:=This:C1470
	
	var $commands; $options : Collection
	$commands:=[]
	
	$command:=This:C1470.escape(This:C1470._executablePath)+" --certificate signature"
	
	If ($params.pin6#Null:C1517) && (Value type:C1509($params.pin6)=Is text:K8:3)
		$command+=" --pin6 "+This:C1470.escape($params.pin6)
	End if 
	
	If ($params.reader#Null:C1517) && (Value type:C1509($params.reader)=Is text:K8:3)
		$command+=" --reader "+This:C1470.escape($params.reader)
		$commands.push($command)
	End if 
	
	This:C1470.controller.execute($commands)
	
Function sign_i($params : Object)->$this : cs:C1710.hpki
	
	$this:=This:C1470
	
	var $commands; $options : Collection
	$commands:=[]
	
	$command:=This:C1470.escape(This:C1470._executablePath)+" --sign identity"
	
	If ($params.pin4#Null:C1517) && (Value type:C1509($params.pin4)=Is text:K8:3)
		$command+=" --pin4 "+This:C1470.escape($params.pin4)
	End if 
	
	If ($params.reader#Null:C1517) && (Value type:C1509($params.reader)=Is text:K8:3)\
		 && ($params.file#Null:C1517) && (OB Instance of:C1731($params.file; 4D:C1709.File)) && ($params.file.exists)
		$command+=" --reader "+This:C1470.escape($params.reader)
		$command+=" "+This:C1470.escape(This:C1470._path($params.file))
		$commands.push($command)
	End if 
	
	This:C1470.controller.execute($commands)
	
Function sign_s($params : Object)->$this : cs:C1710.hpki
	
	$this:=This:C1470
	
	var $commands; $options : Collection
	$commands:=[]
	
	$command:=This:C1470.escape(This:C1470._executablePath)+" --sign signature"
	
	If ($params.pin4#Null:C1517) && (Value type:C1509($params.pin4)=Is text:K8:3)
		$command+=" --pin4 "+This:C1470.escape($params.pin4)
	End if 
	
	If ($params.pin6#Null:C1517) && (Value type:C1509($params.pin6)=Is text:K8:3)
		$command+=" --pin6 "+This:C1470.escape($params.pin6)
	End if 
	
	If ($params.reader#Null:C1517) && (Value type:C1509($params.reader)=Is text:K8:3)\
		 && ($params.file#Null:C1517) && (OB Instance of:C1731($params.file; 4D:C1709.File)) && ($params.file.exists)
		$command+=" --reader "+This:C1470.escape($params.reader)
		$command+=" "+This:C1470.escape(This:C1470._path($params.file))
		$commands.push($command)
	End if 
	
	This:C1470.controller.execute($commands)
	
Function mynumber($params : Object)->$this : cs:C1710.hpki
	
	$this:=This:C1470
	
	var $commands; $options : Collection
	$commands:=[]
	
	$command:=This:C1470.escape(This:C1470._executablePath)+" --mynumber --myinfo"
	
	If ($params.pin4#Null:C1517) && (Value type:C1509($params.pin4)=Is text:K8:3)
		$command+=" --pin4 "+This:C1470.escape($params.pin4)
		If ($params.reader#Null:C1517) && (Value type:C1509($params.reader)=Is text:K8:3)
			$command+=" --reader "+This:C1470.escape($params.reader)
			$commands.push($command)
		End if 
	End if 
	
	This:C1470.controller.execute($commands)
	
Function list()->$this : cs:C1710.hpki
	
	$this:=This:C1470
	
	var $commands; $options : Collection
	$commands:=[]
	
	$command:=This:C1470.escape(This:C1470._executablePath)+" --list"
	
	$commands.push($command)
	
	This:C1470.controller.execute($commands)