import React, { useState, useEffect, useRef } from 'react'
import { AuthClient } from "@dfinity/auth-client"

function App() {
  const [port, setPort] = useState(null)
  const [reader, setReader] = useState(null)
  const [data, setData] = useState('')
  const [connectStatus, setConnectStatus] = useState(0)
  const [status, setStatus] = useState('')
  const [ssid, setSSID] = useState('')
  const [password, setPassword] = useState('')
  const [signedIn, setSignedIn] = useState(false)
  const [authClient, setAuthClient] = useState(null)
  const principal = useRef(null)




  async function  setWifi() {
    if (port!==null && ssid.length > 0 && password.length > 0) {
      console.log('Setting WIFI')
      let lenSSID = (ssid.length).toString(16).padStart(2, '0')
      let lenPassword = (password.length).toString(16).padStart(2, '0')
      let msg = 'ESMSG01' + lenSSID + lenPassword + ssid + password + '\n'
      console.log('msg:', msg)
      const encoder = new TextEncoder()
      const writer = port.writable.getWriter()
      await writer.write(encoder.encode(msg))
      writer.releaseLock()
    }
  }
  
  useEffect(() => {
    function sendPrincipal() {
      if (port!==null) {
        (async () => {
          principal.current = await authClient.getIdentity()
          if(principal.current) {
            console.log('Principal:')
            let _principal = principal.current.getPrincipal().toString()
            console.log(_principal)
            
            let msg = 'ESMSG02' + principal + '\n'
            console.log('msg:', msg)
            const encoder = new TextEncoder()
            const writer = port.writable.getWriter()
            await writer.write(encoder.encode(msg))
            writer.releaseLock()
            
          }

        })()
      }
    }
    

    function read() {
      if (port ) {
        const encoder = new TextEncoder()
        const writer = port.writable.getWriter()
        writer.write(encoder.encode('ESMSG00\n')).then(() => writer.releaseLock())
      }
      if (reader !== null) {
        
          (async () => {
              try{
                while (true) {
                  
                  const { value, done } = await reader.read()
                  if (done) {
                    break
                  }

                  setData((prevOutput) => {
                    let _data = prevOutput + new TextDecoder().decode(value)
                    let ar = _data.split('ESMSG')
                    
                    let msg = ar[ar.length-1]
                    if(msg.indexOf('\n') === -1) return _data
                    msg = msg.split('\n')[0]
                    if(msg.startsWith('00')) {
                      if(msg.length >= 4) {
                        let _status = msg.substring(2,4)
                        //console.log('status: ' + _status)
                        setStatus(_status)
                        return ''
                      } else {
                        return _data
                      }
                    } else if(msg.startsWith('02')){
                      if(msg.length >= 2) {
                        console.log('log: ' + msg)
                        return ''
                      } else {
                        return _data
                      }
                    } else {
                      return _data
                    }
                    
                  })
                  //console.log(data)
                }
              } catch (error) {
                console.log(error)
                setReader(null)
                setPort(null)
                setConnectStatus(0)
              }
            }
          )()
        
        
      }
    }
    if(!principal.current)sendPrincipal()
    const intervalId = setInterval(read, 1000)
    return () => clearInterval(intervalId)
  })
  
  

  const connect = async () => {
    try {
      if(connectStatus > 0) return
      setConnectStatus(1)
      const port = await navigator.serial.requestPort()
      await port.open({ baudRate: 9600 }) //115200
      setPort(port)
      setReader(port.readable.getReader())
      setConnectStatus(2)
      
      
    } catch (error) {
      console.error('Error connecting to the serial port:', error)
      setConnectStatus(0)
      setStatus('')
    }
  }

  function showStatus () {
    switch(status){
      case '00':
        return 'INITIALIZING'
      case '01':
        return 'INITIALIZED'
      case '02':
        return 'INITIALIZE_WITH_WIFI'
      case '03':
        return 'WIFI_CONNECTED'
      case '04':
        return 'CHECKING_REGISTERED'
      case '05':  
        return 'NOT REGISTERED'
      case '06':
        return 'REGISTERING'
      case '07':
        return 'REGISTERED'
      case '':
        return 'DISCONNECTED'
      default:
        //console.log('UNKNOWN STATUS:', status)
        return 'UNKNOWN STATUS'
    }
  }

  

  return (
    <div className="App">
      <header className="App-header">
        <h1>Earthstream Sensor Setup</h1>
        {!signedIn &&
          <div style={{textAlign: 'center'}}><button style={{marginTop: 150, fontSize: 18, height: 40, width: 100}} onClick={async ()=>{
            const _authClient = await AuthClient.create()
            _authClient.login({
              // 30 mins
              maxTimeToLive: Number(30 * 60 * 1000 * 1000 * 1000),
              onSuccess: async () => {
                setSignedIn(true)
                setAuthClient(_authClient)
              },
            })
          }}>Sign in</button></div>
        }
      </header>
      {signedIn &&
        <div>
          {connectStatus === 0 &&
            <button onClick={connect}>Connect</button>
          }
          {connectStatus === 1 && 
            <div>Connecting...</div>
          }
          {connectStatus === 2 && 
            <div>Connected</div>
          }
          <div>Gateway status: {showStatus()}</div>
          
          <div style={{marginBottom: 3, marginTop: 5}}>WIFI Name: <input type="text" onChange={(e) => setSSID(e.target.value)} value={ssid} /></div>
          <div style={{marginBottom: 3}}>Password: <input type="text" onChange={(e) => setPassword(e.target.value)} value={password} /></div>
          <div><button onClick={setWifi}>Update Sensor WIFI settings</button></div>
          
        </div>
      }
    </div>
  )
}

export default App
