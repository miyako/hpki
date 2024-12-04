Class extends _CLI_Controller

property _stdOUt : Collection
property _stdErr : Collection

Class constructor($CLI : cs:C1710._CLI)
	
	Super:C1705($CLI)
	
	This:C1470.init()
	
Function init() : cs:C1710._hpki_Controller
	
	This:C1470._stdOut:=[]
	This:C1470._stdErr:=[]
	
	return This:C1470
	
Function onDataError($worker : 4D:C1709.SystemWorker; $params : Object)
	
	This:C1470._stdErr.push($params.data)
	
Function onData($worker : 4D:C1709.SystemWorker; $params : Object)
	
	This:C1470._stdOut.push($params.data)
	
Function onResponse($worker : 4D:C1709.SystemWorker; $params : Object)
	
	If (This:C1470.complete)
		//end of the queue
		This:C1470.instance.data:=$worker.response
		This:C1470.instance.error:=$worker.responseError
	End if 
	
Function onError($worker : 4D:C1709.SystemWorker; $params : Object)
	
Function onTerminate($worker : 4D:C1709.SystemWorker; $params : Object)
	