Class extends pkiForm

property targetFolder : 4D:C1709.Folder
property source : 4D:C1709.File

Class constructor
	
	Super:C1705()
	
	This:C1470.targetFolder:=Folder:C1567(fk desktop folder:K87:19)
	
	$window:=Open form window:C675("oldHpki")
	DIALOG:C40("oldHpki"; This:C1470; *)
	
Function getData() : 4D:C1709.File
	
	return Form:C1466.source
	
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