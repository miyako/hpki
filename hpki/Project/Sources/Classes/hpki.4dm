Class extends _CLI

Class constructor($controller : 4D:C1709.Class)
	
	Super:C1705("hpki"; $controller=Null:C1517 ? cs:C1710._hpki_Controller : $controller)
	
	//This.controller.timeout:=5
	
Function get worker() : 4D:C1709.SystemWorker
	
	return This:C1470.controller.worker
	
Function get controller() : cs:C1710._hpki_Controller
	
	return This:C1470._controller
	
Function terminate()
	
	This:C1470.controller.terminate()
	
Function _path($item : Object) : Text
	
	return OB Class:C1730($item).new($item.platformPath; fk platform path:K87:2).path
	
Function cert_i($params : Object) : Object
	
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
	
	If (OB Instance of:C1731(This:C1470.controller; cs:C1710._hpkiUI_Controller))
		
	Else 
		This:C1470.worker.wait()
		If (This:C1470.data#"")
			return JSON Parse:C1218(This:C1470.data; Is object:K8:27)
		End if 
		return {success: False:C215}
	End if 
	
Function cert_s($params : Object) : Object
	
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
	
	If (OB Instance of:C1731(This:C1470.controller; cs:C1710._hpkiUI_Controller))
		
	Else 
		This:C1470.worker.wait()
		If (This:C1470.data#"")
			return JSON Parse:C1218(This:C1470.data; Is object:K8:27)
		End if 
		return {success: False:C215}
	End if 
	
Function sign_i($params : Object) : Object
	
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
	
	If (OB Instance of:C1731(This:C1470.controller; cs:C1710._hpkiUI_Controller))
		
	Else 
		This:C1470.worker.wait()
		If (This:C1470.data#"")
			$status:=JSON Parse:C1218(This:C1470.data; Is object:K8:27)
			If ($status.success)
				$digestInfo:=$status.digestInfo
				$signature:=$status.signature
				ARRAY LONGINT:C221($pos; 0)
				ARRAY LONGINT:C221($len; 0)
				var $i; $n : Integer
				var $hash : Blob
				$i:=1
				$n:=0
				SET BLOB SIZE:C606($hash; Length:C16($digestInfo)/2)
				While (Match regex:C1019("([:hex_digit:]{2})"; $digestInfo; $i; $pos; $len))
					$hash{$n}:=Formula from string:C1601("0x"+Substring:C12($digestInfo; $pos{1}; $len{1})).call()
					$n+=1
					$i:=$pos{1}+$len{1}
				End while 
				$status._digestInfo:=$hash
				$i:=1
				$n:=0
				SET BLOB SIZE:C606($hash; Length:C16($signature)/2)
				While (Match regex:C1019("([:hex_digit:]{2})"; $signature; $i; $pos; $len))
					$hash{$n}:=Formula from string:C1601("0x"+Substring:C12($signature; $pos{1}; $len{1})).call()
					$n+=1
					$i:=$pos{1}+$len{1}
				End while 
				$status._signature:=$hash
			End if 
			return $status
		End if 
		return {success: False:C215}
	End if 
	
Function sign_s($params : Object) : Object
	
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
	
	If (OB Instance of:C1731(This:C1470.controller; cs:C1710._hpkiUI_Controller))
		
	Else 
		This:C1470.worker.wait()
		If (This:C1470.data#"")
			$status:=JSON Parse:C1218(This:C1470.data; Is object:K8:27)
			If ($status.success)
				$digestInfo:=$status.digestInfo
				$signature:=$status.signature
				ARRAY LONGINT:C221($pos; 0)
				ARRAY LONGINT:C221($len; 0)
				var $i; $n : Integer
				var $hash : Blob
				$i:=1
				$n:=0
				SET BLOB SIZE:C606($hash; Length:C16($digestInfo)/2)
				While (Match regex:C1019("([:hex_digit:]{2})"; $digestInfo; $i; $pos; $len))
					$hash{$n}:=Formula from string:C1601("0x"+Substring:C12($digestInfo; $pos{1}; $len{1})).call()
					$n+=1
					$i:=$pos{1}+$len{1}
				End while 
				$status._digestInfo:=$hash
				$i:=1
				$n:=0
				SET BLOB SIZE:C606($hash; Length:C16($signature)/2)
				While (Match regex:C1019("([:hex_digit:]{2})"; $signature; $i; $pos; $len))
					$hash{$n}:=Formula from string:C1601("0x"+Substring:C12($signature; $pos{1}; $len{1})).call()
					$n+=1
					$i:=$pos{1}+$len{1}
				End while 
				$status._signature:=$hash
			End if 
			return $status
		End if 
		return {success: False:C215}
	End if 
	
Function mynumber($params : Object) : Text
	
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
	
	If (OB Instance of:C1731(This:C1470.controller; cs:C1710._hpkiUI_Controller))
		
	Else 
		This:C1470.worker.wait()
		If (This:C1470.data#"")
			return JSON Parse:C1218(This:C1470.data; Is object:K8:27)
		End if 
		return {success: False:C215}
	End if 
	
Function list() : Object
	
	var $commands; $options : Collection
	$commands:=[]
	
	$command:=This:C1470.escape(This:C1470._executablePath)+" --list"
	
	$commands.push($command)
	
	This:C1470.controller.init().execute($commands)
	
	If (OB Instance of:C1731(This:C1470.controller; cs:C1710._hpkiUI_Controller))
		
	Else 
		This:C1470.worker.wait()
		If (This:C1470.data#"")
			return JSON Parse:C1218(This:C1470.data; Is object:K8:27)
		End if 
		return {success: False:C215}
	End if 