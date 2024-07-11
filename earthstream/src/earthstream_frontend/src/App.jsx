import { earthstream_backend } from 'declarations/earthstream_backend';
import React, { useState, useEffect, useRef } from 'react'
import { AuthClient } from "@dfinity/auth-client"

const _font = '"Courier Prime", monospace'
window.cnt = 0

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
  const [serialCom, setSerialCom] = useState('')
  const [page, setPage] = useState('setup')
  const [sensorData, setSensorData] = useState([])
  const [lastTS, setLastTS] = useState(0)



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
        //console.log('sending principal')
        (async () => {
          
          principal.current = await authClient.getIdentity()
          if(principal.current) {
            console.log('Principal:')
            let _principal = principal.current.getPrincipal().toString()
            console.log(_principal)
            console.log('sending principal')
            let msg = 'ESMSG02' + _principal + '\n'
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
                    setSerialCom(_data)
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
                    } else if(msg.startsWith('04')){
                      if(msg.length >= 2) {
                        console.log(msg.substring(2))
                        return ''
                      } else {
                        return _data
                      }
                    }else {
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
    if(!principal.current)setTimeout(sendPrincipal,5000)
    const intervalId2 = setInterval(()=>{
      
        
        console.log('reading sensor data')
        earthstream_backend.get_parsed_data(lastTS+1).then((data)=>{
          data = data.map((d)=>{d.ts = parseInt(d.ts.toString()); return d})
          setSensorData((prev)=>{
            let combined = [...prev, ...data]
            combined = combined.slice(-10)
            return combined
          })
          if(data.length>0) setLastTS(parseInt(data[data.length-1].ts))
        })
      /*
        
        let s = [{"ts":"1720531970","latitude":"","signature":"3dd844a33c4a718f6695ade75bfa705d99fdbee1c1f3be46acabaf17d45d03cd70fe9ba007894f4983feeb2fd0abfb357030502709a3b11f89d02e34a67ef162","light_intensity":"1875","public_key":"80aa87bedf28aafe89de3caaf718c4945bf0a93eb3e8b88c42e28d8766163c528cb89969fd5288026b79d3575fcbf9242cea0f2de26063c62d062c832f0382f3","temp":"","altitude":"","humidity":"","longitude":""},{"ts":"1720532000","latitude":"","signature":"a25842e8fe67693272d6029f55ed22b575bf402928af4b2179cb6023ae9e8a8df8674ec151bfaf5bff9982dea4f651831a3cf9ee6506f50e39028bfedb523ac8","light_intensity":"2055","public_key":"80aa87bedf28aafe89de3caaf718c4945bf0a93eb3e8b88c42e28d8766163c528cb89969fd5288026b79d3575fcbf9242cea0f2de26063c62d062c832f0382f3","temp":"16.799999","altitude":"","humidity":"74.900002","longitude":""},{"ts":"1720532030","latitude":"","signature":"cbcd3cdc955acf74386bc351d82ea275d677007c09f88fc20f7f94d6d12996b295000555f810e9fac413296b16700e80f03ec7ce39734cb884f0971be65e00d7","light_intensity":"1952","public_key":"80aa87bedf28aafe89de3caaf718c4945bf0a93eb3e8b88c42e28d8766163c528cb89969fd5288026b79d3575fcbf9242cea0f2de26063c62d062c832f0382f3","temp":"16.799999","altitude":"","humidity":"74.900002","longitude":""},{"ts":"1720544819","latitude":"","signature":"1a9ab6c5282178e1fdd1506038c0ae69941443a1d1ff2c861fe468674fa2506224cf6740ec0f600387fc00c583c81dea4d18af608df9f7a08013d61e8882599d","light_intensity":"69","public_key":"80aa87bedf28aafe89de3caaf718c4945bf0a93eb3e8b88c42e28d8766163c528cb89969fd5288026b79d3575fcbf9242cea0f2de26063c62d062c832f0382f3","temp":"","altitude":"","humidity":"","longitude":""},{"ts":"1720544850","latitude":"","signature":"b2320f07df6a0689e1e12a1290bb386cc7dc8cf9206d4c22793374bd2020cdefc061aacbaab4326775a99c8402d6bd49fad2c7f51704fbec97198e8e1c5b5644","light_intensity":"87","public_key":"80aa87bedf28aafe89de3caaf718c4945bf0a93eb3e8b88c42e28d8766163c528cb89969fd5288026b79d3575fcbf9242cea0f2de26063c62d062c832f0382f3","temp":"16.400000","altitude":"","humidity":"74.900002","longitude":""},{"ts":"1720544880","latitude":"","signature":"3d75965b7de73c3e5cf8db04e7b1c59a6d1af4bb400f4cd647d8541339cd94331b28818cd9fcb0886ccb6c33808434d9129384188f62e0449d24bfbcd8065407","light_intensity":"73","public_key":"80aa87bedf28aafe89de3caaf718c4945bf0a93eb3e8b88c42e28d8766163c528cb89969fd5288026b79d3575fcbf9242cea0f2de26063c62d062c832f0382f3","temp":"16.400000","altitude":"","humidity":"74.800003","longitude":""},{"ts":"1720544910","latitude":"","signature":"c81ca8c58f76bcc8466aa8f13c4f17e3418527ade6605d5904e8b51101e63eca70c4792342ada0d677d5c37f5a43f23cbe7d815a97e4d8f1d5cca54e79c5cceb","light_intensity":"68","public_key":"80aa87bedf28aafe89de3caaf718c4945bf0a93eb3e8b88c42e28d8766163c528cb89969fd5288026b79d3575fcbf9242cea0f2de26063c62d062c832f0382f3","temp":"16.500000","altitude":"","humidity":"74.800003","longitude":""},{"ts":"1720544941","latitude":"","signature":"335243efce81b490bd2d5080c9b663c65098f6d29e89551ce78f47f0abc2f76d02bd949486259f778f29e7d43cad0b1e3f88d82b230aa7b7fda5a54f0a16ec5d","light_intensity":"77","public_key":"80aa87bedf28aafe89de3caaf718c4945bf0a93eb3e8b88c42e28d8766163c528cb89969fd5288026b79d3575fcbf9242cea0f2de26063c62d062c832f0382f3","temp":"16.500000","altitude":"","humidity":"74.699997","longitude":""},{"ts":"1720546283","latitude":"","signature":"845175b1959af77039c2a07d0377bd0c0a28395934a01e0629651fa04dd058920a401de0fa17d46a5195c73d80c679a3f83a6b2424584c999d43f2870f5cd66a","light_intensity":"73","public_key":"80aa87bedf28aafe89de3caaf718c4945bf0a93eb3e8b88c42e28d8766163c528cb89969fd5288026b79d3575fcbf9242cea0f2de26063c62d062c832f0382f3","temp":"","altitude":"","humidity":"","longitude":""},{"ts":"1720546313","latitude":"","signature":"4ab62097adb1e4eda385a27b195013a77db360c327301848e97b91ef135db9f9b6c98f8ba3e454ad2da2e99673bdc0caa2f4ff89ddba415d60ea15ccb53ae7e4","light_intensity":"69","public_key":"80aa87bedf28aafe89de3caaf718c4945bf0a93eb3e8b88c42e28d8766163c528cb89969fd5288026b79d3575fcbf9242cea0f2de26063c62d062c832f0382f3","temp":"16.299999","altitude":"","humidity":"75.699997","longitude":""},{"ts":"1720546398","latitude":"","signature":"ed05e78d4b1cebbd3cbfc9f7bebee92b211efb33d55b955412425f0b33bc6cfa286ff4e1ae945fc575a6a8bc7ba0e299aa458726b807d1002113472858c26f5e","light_intensity":"68","public_key":"80aa87bedf28aafe89de3caaf718c4945bf0a93eb3e8b88c42e28d8766163c528cb89969fd5288026b79d3575fcbf9242cea0f2de26063c62d062c832f0382f3","temp":"16.299999","altitude":"","humidity":"75.599998","longitude":""},{"ts":"1720547134","latitude":"","signature":"b1735117c1839911d783801cce5976b1a2e4be0acd69fd9421098cdbb498dc427f6dda8cddbd96256cd1315aaa1aa44c3d83ded0e095564e9a6538118463cb55","light_intensity":"83","public_key":"80aa87bedf28aafe89de3caaf718c4945bf0a93eb3e8b88c42e28d8766163c528cb89969fd5288026b79d3575fcbf9242cea0f2de26063c62d062c832f0382f3","temp":"16.400000","altitude":"","humidity":"74.800003","longitude":""},{"ts":"1720547194","latitude":"","signature":"8df09cfdcf3afe1df6be0be86a007e7b6e57afab7f416f14ca76bd0e949ce3b9baa76665142f136af6aa23297cd8cd6ffd03ca1a52c2aa47777ebc1b43632ea3","light_intensity":"83","public_key":"80aa87bedf28aafe89de3caaf718c4945bf0a93eb3e8b88c42e28d8766163c528cb89969fd5288026b79d3575fcbf9242cea0f2de26063c62d062c832f0382f3","temp":"16.500000","altitude":"","humidity":"74.599998","longitude":""},{"ts":"1720547255","latitude":"","signature":"031a4f30d9ec5bd4c71348aacbd45cb6614a29dc34cf61cc367fd324bb920b304fc49e269ae7dda6b804a9261a70ebe9e71628138680990ad1e9410d77ee4f30","light_intensity":"182","public_key":"80aa87bedf28aafe89de3caaf718c4945bf0a93eb3e8b88c42e28d8766163c528cb89969fd5288026b79d3575fcbf9242cea0f2de26063c62d062c832f0382f3","temp":"16.500000","altitude":"","humidity":"74.500000","longitude":""},{"ts":"1720547316","latitude":"","signature":"d42d76932efe62197333ed13ecfbe5f518e61c9a5793e15d152fd54cff164eb7491bbca74bdac41b576870ba4e4b38ae1e7b465b2312b1bb71a24c6fdf7d3d0a","light_intensity":"197","public_key":"80aa87bedf28aafe89de3caaf718c4945bf0a93eb3e8b88c42e28d8766163c528cb89969fd5288026b79d3575fcbf9242cea0f2de26063c62d062c832f0382f3","temp":"16.600000","altitude":"","humidity":"74.400002","longitude":""},{"ts":"1720547376","latitude":"","signature":"47da87508e97578ce9f9fb08c50c6bb9ec4f0e557a914c894b1bc4df4cea16b9695b267afec524f67c9674d66323c12d3c235c2a80062634267b3cd67e25bc81","light_intensity":"208","public_key":"80aa87bedf28aafe89de3caaf718c4945bf0a93eb3e8b88c42e28d8766163c528cb89969fd5288026b79d3575fcbf9242cea0f2de26063c62d062c832f0382f3","temp":"16.600000","altitude":"","humidity":"74.199997","longitude":""},{"ts":"1720547438","latitude":"","signature":"6702947ab0f0733f4e0545476b76868d1c0ba72f77e405266b13f80b448da800ce121e7c93d2c992770a0e431e7d283384cb38c02d209a82ee3dc42b4f00a02b","light_intensity":"197","public_key":"80aa87bedf28aafe89de3caaf718c4945bf0a93eb3e8b88c42e28d8766163c528cb89969fd5288026b79d3575fcbf9242cea0f2de26063c62d062c832f0382f3","temp":"16.600000","altitude":"","humidity":"74.099998","longitude":""},{"ts":"1720547497","latitude":"","signature":"bdbe494e1b53e777da5dadf02af51530418faacd5cb7f9220486b2829168a3fbef5e7c460f208509df27381f82c88efefd1d17e52ca930fa1eb9aca2259cfaee","light_intensity":"192","public_key":"80aa87bedf28aafe89de3caaf718c4945bf0a93eb3e8b88c42e28d8766163c528cb89969fd5288026b79d3575fcbf9242cea0f2de26063c62d062c832f0382f3","temp":"16.600000","altitude":"","humidity":"74.199997","longitude":""},{"ts":"1720592694","latitude":"","signature":"fab2cb4168f0ad7ccfc00158fc91edeb638ff41e0b174826662709224e24ff3885577906342b88decbe3dbbe25f073156f56a3d9ca5ce9fb78242d6766498396","light_intensity":"263","public_key":"80aa87bedf28aafe89de3caaf718c4945bf0a93eb3e8b88c42e28d8766163c528cb89969fd5288026b79d3575fcbf9242cea0f2de26063c62d062c832f0382f3","temp":"16.000000","altitude":"","humidity":"74.300003","longitude":""},{"ts":"1720592753","latitude":"","signature":"a6d49d284d1733a9a6174fdbed2267c6e9190906b0ab6168e2764e77efc6d06c9d0a8033b1d0ef4ff91b73c9766e28e2ff0c0eb5f80c3542b3032162f46515c8","light_intensity":"269","public_key":"80aa87bedf28aafe89de3caaf718c4945bf0a93eb3e8b88c42e28d8766163c528cb89969fd5288026b79d3575fcbf9242cea0f2de26063c62d062c832f0382f3","temp":"16.000000","altitude":"","humidity":"74.199997","longitude":""},{"ts":"1720601737","latitude":"","signature":"3881085e1f88703b199d0b8cb00c4a4d4944c3d3bfc0bd109ceaed411c8feb880f96941c7fd4dc1fe5c218aad50cbd900f6eb3bb82c2fd037acad1aae1f93f3c","light_intensity":"1566","public_key":"80aa87bedf28aafe89de3caaf718c4945bf0a93eb3e8b88c42e28d8766163c528cb89969fd5288026b79d3575fcbf9242cea0f2de26063c62d062c832f0382f3","temp":"","altitude":"","humidity":"","longitude":""},{"ts":"1720601798","latitude":"","signature":"501e17b27cf0a0b50eb1864eebd3f9c47225459d9ec4900bdedebd79a8d6403d4da5bee585a9ef99d98d33b68740e55b8646c0979814ff059abaca7b1dcb04fe","light_intensity":"1986","public_key":"80aa87bedf28aafe89de3caaf718c4945bf0a93eb3e8b88c42e28d8766163c528cb89969fd5288026b79d3575fcbf9242cea0f2de26063c62d062c832f0382f3","temp":"16.200001","altitude":"","humidity":"78.199997","longitude":""},{"ts":"1720601857","latitude":"","signature":"7c913d8cf04db456656b1f651dcbf0c6bba7b1d1ccd7e44e5cee0ba6f9c34ba50d6ffe5538525d870605013cc19b123902bd3f6ed43f5abac0e9f2fdf6935d3c","light_intensity":"2240","public_key":"80aa87bedf28aafe89de3caaf718c4945bf0a93eb3e8b88c42e28d8766163c528cb89969fd5288026b79d3575fcbf9242cea0f2de26063c62d062c832f0382f3","temp":"16.299999","altitude":"","humidity":"78.099998","longitude":""},{"ts":"1720601918","latitude":"","signature":"fe42b4c07d7a532f63c0b065b845a021758977f960b6db5f413dda0664e0fb524470d98bebf9667908e46bc56c9791e04b0da7d5a0db27fe763ec64c74bb9892","light_intensity":"2234","public_key":"80aa87bedf28aafe89de3caaf718c4945bf0a93eb3e8b88c42e28d8766163c528cb89969fd5288026b79d3575fcbf9242cea0f2de26063c62d062c832f0382f3","temp":"16.299999","altitude":"","humidity":"78.199997","longitude":""},{"ts":"1720601979","latitude":"","signature":"4f531b027dddb912bc48cc45df4ca2bbe674a63873a4f290de5963de9d1664299e86371bfdfcdddfb4e86e44db0317f65228259bf93ac5900279682b0c0cd9d9","light_intensity":"2464","public_key":"80aa87bedf28aafe89de3caaf718c4945bf0a93eb3e8b88c42e28d8766163c528cb89969fd5288026b79d3575fcbf9242cea0f2de26063c62d062c832f0382f3","temp":"16.299999","altitude":"","humidity":"78.099998","longitude":""},{"ts":"1720602039","latitude":"","signature":"69110d079ccdf68f52939f3aa9a02ef16b169250f2a13792a322c656c0c8fbfb85a89eac44d2a810dcb305174294fa4d684e256fdbfc94e9a4783b0041358e52","light_intensity":"2057","public_key":"80aa87bedf28aafe89de3caaf718c4945bf0a93eb3e8b88c42e28d8766163c528cb89969fd5288026b79d3575fcbf9242cea0f2de26063c62d062c832f0382f3","temp":"16.299999","altitude":"","humidity":"77.800003","longitude":""},{"ts":"1720602120","latitude":"","signature":"ab81fe0b771ae65f31ae44a16887af4d33673396c20c9cb2cfd42d19e9951d749b285597fdd51bf4f5c4724c02e706b68180e84e1f90c1638f47235da42ea202","light_intensity":"3052","public_key":"80aa87bedf28aafe89de3caaf718c4945bf0a93eb3e8b88c42e28d8766163c528cb89969fd5288026b79d3575fcbf9242cea0f2de26063c62d062c832f0382f3","temp":"16.299999","altitude":"","humidity":"77.699997","longitude":""},{"ts":"1720602179","latitude":"","signature":"33056b59826e095d2ae93d7fdc18be25422249159048b7633aecb34ce5ec295d3ccaf89d8f109829e6ff09156d1c0e91e5397c18e26096f877dc9b69c751e931","light_intensity":"2701","public_key":"80aa87bedf28aafe89de3caaf718c4945bf0a93eb3e8b88c42e28d8766163c528cb89969fd5288026b79d3575fcbf9242cea0f2de26063c62d062c832f0382f3","temp":"16.299999","altitude":"","humidity":"77.800003","longitude":""},{"ts":"1720602239","latitude":"","signature":"f42e31b7ff88aa6b1b81dab6105d98a464e4a6de6b347ecfad9462ee20f427f833c909126de15ff24b82500473852478e34c23ba6c7758e3c67336ff5c326ecd","light_intensity":"2130","public_key":"80aa87bedf28aafe89de3caaf718c4945bf0a93eb3e8b88c42e28d8766163c528cb89969fd5288026b79d3575fcbf9242cea0f2de26063c62d062c832f0382f3","temp":"16.400000","altitude":"","humidity":"77.800003","longitude":""},{"ts":"1720602299","latitude":"","signature":"a48e5ead41eda8fc0bf318b7f49922ad9617ab2c3481ecb5e29493dd9e091f7eb7b2ddb26c36365636a272c45fb9718e5b70dcbdc113e756c64dff672095ea92","light_intensity":"2361","public_key":"80aa87bedf28aafe89de3caaf718c4945bf0a93eb3e8b88c42e28d8766163c528cb89969fd5288026b79d3575fcbf9242cea0f2de26063c62d062c832f0382f3","temp":"16.400000","altitude":"","humidity":"77.699997","longitude":""},{"ts":"1720602360","latitude":"","signature":"1dbf28224af78175544bb423f4eda7f84af0cb17f89f98253c37db5809882e79a55392e89ff0e632277b2a077b2ef9818a638b6f24d886c28d3a8a59c0b9add8","light_intensity":"3102","public_key":"80aa87bedf28aafe89de3caaf718c4945bf0a93eb3e8b88c42e28d8766163c528cb89969fd5288026b79d3575fcbf9242cea0f2de26063c62d062c832f0382f3","temp":"16.500000","altitude":"","humidity":"77.500000","longitude":""},{"ts":"1720602394","latitude":"","signature":"9c9e00a6030cc1d268dcd56fd6b93df20dc716c7013d49d197963cc72a8e0b7a2601388a9a5fc0e31126f3658fe950ed7213563f052624d21f25749ee90fdd13","light_intensity":"4075","public_key":"80aa87bedf28aafe89de3caaf718c4945bf0a93eb3e8b88c42e28d8766163c528cb89969fd5288026b79d3575fcbf9242cea0f2de26063c62d062c832f0382f3","temp":"16.600000","altitude":"","humidity":"77.000000","longitude":""}]
       if(window.cnt<s.length)window.cnt++
        s = s.map((d)=>{
          d.ts = parseInt(d.ts.toString()); 
          return d}
        )
        let ar = []
        for(let i=0;i<window.cnt;i++){
          ar[i] = s[i]
        }
        ar = ar.slice(-10)
        setSensorData(ar)
        */
        
    }, 30000)
    const intervalId = setInterval(read, 1000)
    return () => {
      clearInterval(intervalId)
      clearInterval(intervalId2)
    }
  })
  
  

  const connect = async () => {
    try {
      if(connectStatus > 0) return
      setConnectStatus(1)
      const _port = await navigator.serial.requestPort()
      window.port = _port
      await _port.open({ baudRate: 9600 }) //115200
      setPort(_port)
      setReader(_port.readable.getReader())
      setConnectStatus(2)
    } catch (error) {
      console.error('Error connecting to the serial port:', error)
      setConnectStatus(0)
      setStatus('')
      setData('')
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
      case '08':
        return 'GOT_PRINCIPAL'
      case 'F1':
        return 'ERROR_WIFI_NOT_CONNECTED'
      case 'F2':
        return 'ERROR_PRINCIPAL_MISMATCH'
      case 'F3':
        return 'ERROR_FETCH_LOCAL_TIME'
      case '':
        return 'DISCONNECTED'
      default:
        //console.log('UNKNOWN STATUS:', status)
        return 'UNKNOWN STATUS'
    }
  }

  function formatTimestamp(ts) {
    let date = new Date(ts);
    let day = date.getDate();
    let month = date.toLocaleString('default', { month: 'short' });
    let year = date.getFullYear().toString().substring(2);
    let hours = date.getHours();
    let minutes = date.getMinutes();
    minutes = minutes < 10 ? '0'+minutes : minutes; // add leading zero if minutes < 10
    return `${day} ${month} '${year} ${hours}:${minutes}`;
  }

  function formatSensorData () {
    let style = {display: 'inline-block', padding: 5, margin: 5, fontSize: 12}
    let tsStyle = Object.assign({}, style, {width: 120})
    let tempStyle = Object.assign({}, style, {width: 80})
    let humidityStyle = Object.assign({}, style, {width: 120})
    let lightStyle = Object.assign({}, style, {width: 80})
    let sensorIDStyle = Object.assign({}, style, {width: 170})
    let _data = sensorData.map((d,i)=>{
      return (
        <div key={d.ts} style={{backgroundColor:(i%2===0)?'#f5f5f5':'transparent'}}>
          <div style={tsStyle}>{formatTimestamp(parseInt(d.ts)*1000)}</div>
          <div style={sensorIDStyle}>SensorID: {d.public_key.substring(0,5) + '...' + d.public_key.substring(d.public_key.length-5)}</div>
          <div style={tempStyle}>Temp: {d.temp.substring(0,5)}</div>
          <div style={humidityStyle}>Humidity: {d.humidity.substring(0,5)}%</div>
          <div style={lightStyle}>Light: {d.light_intensity}</div>
        </div>
      )
    })
    return _data
  }

  return (
    <div className="App" style={{marginLeft: 20, marginRight: 20, fontFamily: _font}}>
      <header className="App-header">
        <h2 style={{textAlign: 'center'}}>{page==='setup'?'Earthstream Gateway Setup':'Earthstream Sensor Data'}</h2>
        {!signedIn &&
          <div style={{textAlign: 'center'}}><button style={{marginTop: 150, fontSize: 18, height: 40, width: 100, fontFamily: _font}} onClick={async ()=>{
            const _authClient = await AuthClient.create()
            _authClient.login({
              // 30 mins
              defaultExpiry: 60 * 60 * 24,
              onSuccess: async () => {
                setSignedIn(true)
                setAuthClient(_authClient)
              },
            })
          }}>Sign in</button></div>
        }
      </header>
      
      {signedIn &&
        <div style={{fontSize: 15}}>
          <div style={{textAlign:"center", fontFamily: _font}}><button onClick={()=>setPage("setup")} style={{margin: 5, fontFamily: _font, fontWeight: (page==='setup')?'bold':'normal'}}>Setup Gateway</button><button onClick={()=>setPage("sensor")} style={{margin: 5, fontFamily: _font, fontWeight: (page==='sensor')?'bold':'normal'}}>Sensor Data</button></div>
          {page === 'setup' && 
            <div style={{display: 'block', width: 600, marginLeft: 'auto', marginRight: 'auto', textAlign: 'center', padding: 10, marginTop: 20}}>
              {connectStatus === 0 &&
                <button onClick={connect} style={{fontFamily: _font}}>Connect to Gateway</button>
              }
              {connectStatus === 1 && 
                <div style={{textAlign: 'left', display: 'block', marginLeft: 'auto', marginRight: 'auto', width: 270}}>Connection status: CONNECTING</div>
              }
              {connectStatus === 2 && 
                <div style={{textAlign: 'left', display: 'block', marginLeft: 'auto', marginRight: 'auto', width: 270}}>Connection status: CONNECTED</div>
              }
              {connectStatus === 2 &&
                <>
                <div style={{textAlign: 'left', display: 'block', marginLeft: 'auto', marginRight: 'auto', width: 270, marginTop: 10, marginBottom: 10}}>Gateway status: {showStatus()}</div>
                
                
                <div style={{marginTop: 30, marginBottom: 20, fontWeight: 'bold'}}>Gateway Wifi Setup: </div>
                <div style={{marginBottom: 3, marginTop: 5}}>WIFI Name: <input type="text" onChange={(e) => setSSID(e.target.value)} value={ssid} /></div>
                <div style={{marginBottom: 3}}>Password: <input type="text" onChange={(e) => setPassword(e.target.value)} value={password} /></div>
                <div><button onClick={setWifi} style={{marginBottom: 10, marginTop: 15, fontFamily: _font}}>Update Sensor WIFI settings</button></div>
                
                </>
              }
            </div>
          } 

          {page === 'sensor' &&
              <div style={{display: 'block', width: 700, marginLeft: 'auto', marginRight: 'auto', paddingTop: 20, marginTop: 20}}>
                
                <b>Sensor Data</b>
                <div style={{marginTop:10}}>
                  {formatSensorData(sensorData)}
                </div>
                
              </div>
          }
        </div>
      }
    </div>
  )
}

export default App

