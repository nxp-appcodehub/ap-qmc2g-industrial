## Webservice



The webservice is implemented by an instance of the core IoT httpd webserver in the webserver directory.

Requests are handled by the following ruleset from `source/webservice/webservice.c`

### Ruleset

| PATTERN                       | METHODS                            | PLUG              | FLAGS       | OPTIONS            | DESCRIPTION                                                  |
| ----------------------------- | ---------------------------------- | ----------------- | ----------- | ------------------ | ------------------------------------------------------------ |
| `/index.html?$`               | GET                                | redirect          | 301         | "/"                | preform a permanent redirect to "/"                          |
| `/$`                          | GET                                | rewrite_path      | 0           | "/index.html"      | serve "/" from "index.html"                                  |
| `/logout$`                    | GET                                | rewrite_path      | HTTP_DELETE | "/api/session      | rewrite to DELETE "/api/session"                             |
| `\|/%w+$`                     | GET                                | redirect          | 302         | "/"                | redirect single word urls to "/" (temporary redirect to the web application) |
| `/api%f[/]`                   | GET<br />POST<br />PUT<br />DELETE | qmc_authorize     | 0           | NULL               | Authorization on /api/,<br />Adds header, validates tokens.<br />no need to succeed |
| `./motd/?$`                   | GET                                | json              | 0           | json_motd_api      | /api/motd<br />get the system usage message (single string)  |
| `./session%f[/%z]/?%f[%d%z]`  | GET<br />POST<br />DELETE          | json              | 128         | json_session_api   | /api/session<br />/api/session/*n*<br />session API, optionally authorized.<br />match on /api/session |
| `/api%f[/]`                   | ANY                                | qmc_check_session | 0           | NULL               | Checks authorization from qmc_authorize, fails if  no session is established. |
| `./motors%f[/%z]/?%f[%d%z]`   | GET<br />POST<br />PUT             | json              | *128*       | json_motor_api     | match /api/motor, /api/motor/1 ...                           |
| `./log$`                      | GET                                | json              | 0           | json_log_api       | /api/log<br />/api/log?last=*n*<br />/api/log?pre=*n*        |
| `./time$`                     | GET<br />PUT<br />POST             | json              | 128         | json_time_api      | /api/time                                                    |
| `./reset$`                    | POST                               | json              | 0           | json_reset_api     | /api/reset                                                   |
| `./settings%f[/%z]/?%f[%w%z]` | GET<br />PUT                       | json              | 512         | json_settings_api  | /api/settings (list of common settings)<br />/api/settings/*name* <br />(settings by name) |
| `./firmware$`                 | POST                               | qmc_fw_upload     | 0           | NULL               | firmware upload plug on /api /firmware                       |
| `./system/?$`                 | GET<br />PUT<br />POST             | json              | 128         | json_system_api    | system state management api on /api/system                   |
| `./users/?$`                  | GET                                | json              | 0           | json_user_list_api | /api/users<br />list of all users                            |
| `./users/`                    | GET<br />PUT<br />POST<br />DELETE | json              | 512         | json_user_api      | /api/users/*username*                                        |
| `/`                           | GET                                | zip_fs            | 404         | webroot.zip        | GET on any path will be looked up within webroot.zip in the firmware.<br />404 if not found |
|                               | ANY                                | plug_status       | 405         | NULL               | unsupported method error for all other requests.             |

rules are processed from top to bottom.

### API Endpoints

| ENDPOINT            | Method | Notes                                                        | Message                                                      |
| :------------------ | ------ | ------------------------------------------------------------ | :----------------------------------------------------------- |
| /logout             | GET    | invokes DELETE on /api/session                               |                                                              |
| /api/session        | GET    | list of session tokens payloads of active sessions           | [<session_token>,...]                                        |
|                     | POST   | login                                                        | {"user":"*string*", <br />"passphrase": "string"}I           |
|                     | DELETE | logout                                                       |                                                              |
| /api/session/*sid*  | DELETE | logout session id *sid* maintenance head only                |                                                              |
| /api/motors         | GET    | status message for all motors                                | list of motors status messages for all configured motors     |
|                     | POST   | motor command                                                | motor command message (see below)                            |
| /api/motors/*n*     | GET    | motor number *n* status                                      | motor status message for motor *n*                           |
|                     |        |                                                              |                                                              |
| /api/time/          | GET    | get system time                                              | {"time":"123456789"}  time string in milliseconds            |
|                     | PUT    | set system time<br />maintenance head only                   | {"time":"123456789"} time string in milliseconds             |
| /api/settings       | GET    | list of common settings                                      | [...] list of common settings with description               |
| /api/settings/*key* | GET    | settings for a specific key<br />does not publish user account records. | settings value message                                       |
|                     | PUT    | change a configuration<br />maintenance head only            | settings value message                                       |
| /api/users          | GET    | list of accounts with role<br />maintenance head only        | [{"user":"*user name*","role":"*role name*"}...] list of users |
|                     | POST   | create a new user<br />maintenance head only                 | {"user":"*username*",<br />"role":"*role name*", <br />"new_passphrase":"*new users passphrase*"<br />"passphrase": "*current maintenance  head users passphrase*"} |
|                     | PUT    | change users passphrase.<br />extends account validity.<br />maintenance head only | {"user":"*user name*",<br />"passphrase":"**current maintenance head user  passphrase*", "new_passphrase":"*new passphrase*"} |
|                     | DELETE | delete an account<br />maintenance head only                 |                                                              |
| /api/log            | GET    | get some of the current log messages                         | list of last 20 log entries                                  |
| /api/log?last=*id*  | GET    | get newer messages (after id)                                | return messages only if they are newer than *id*             |
| /api/log?pre=*id*   | GET    | get older messages                                           | messages preceding given *id* (combinable with last= as well) |
| /api/system         | GET    | firmware info                                                | {deviceId:"...", fwVersion:"1.2.3", "lifecycle":"operational"} |
|                     | PUT    | change lifecycle<br />maintenance head only                  | {"lifecycle":"maintenance"}                                  |
| /api/firmware       | POST   | firmware upload<br />staging area is filled with signed image.<br /><br />maintenance head only | Content-Type: application/octet-stream<br />responds with<br />{"bytes":\<count>, <br />"sha256":\<cheksum>} |

## Messages

#### Session 

 ```json
  {
   "sid": 0, // session id and token
   "token": "eyJhbGciOiJIUzI1NiIsInR5cCI6IkpXVCIsImtpZCI6MH0.eyJzaWQiOjAsImlhdCI6IjkzNyIs
 ImV4cCI6IjgxMzciLCJyb2xlIjoibWFpbnRlbmFuY2UiLCJpc3MiOiJGQTc2M0ZDRjEzNUY4NjdBMUQwMUJDRkJD
 NzJGM0IzRDhEQkNDRjU3MUU2OEI4QjVDOTI1RDlDQTBBREQwQzREIiwic3ViIjoiYWRtaW4ifQ.pn24iqGyy5uPM
 Echq90C3d1gFeyyobU5ZTME3kuzuHs",
   "payload": { // the actuall token payload 
     "sid": 0, 
     "iat": "937", // issueing time
     "exp": "8137", // token expiry time
     "role": "maintenance", 
     "iss": "FA763FCF135F867A1D01BCFBC72F3B3D8DBCCF571E68B8B5C925D9CA0ADD0C4D", // system id
     "sub": "admin" // user name
   }
 }
 
 ```



#### Motor Status - GET

```json
            {
                "id": 1,
                "fast": {
                  "state": "fault",
                  "faultStatus": "noFaultMC",
                  "ia": 0,
                  "ib": 0,
                  "ic": 0,
                  "valpha": 0,
                  "vbeta": 0,
                  "vDcBus": 0
                },
                "slow": {
                  "app": "off",
                  "speed": 0,
                  "motorPosition": {
                    "numOfTurns": 0,
                    "rotorPosition": 0
                  }
                }
            }
```

#### Motor Command - PUT

```json
        {
            "app":<enum>,  // optional appswitch enum ["off","on","freze","freezeAndStop"], default current
            "controlMethod": <enum>, // ["scalar","speed","position"]
            // for "scalar"
            "gain":<float>,
            "frequency":<float>,
            // for "position"
            "isRandom":<boolean>,
            "motorPosition": {
                "numOfTurns": <int16>,
                "rotorPosition": <int16>
            }
            // for "speed" 
            "speed":<float>
        }
```

#### Log Entry

```javascript
 {
        "id":12, 
        "ts":"1669364248.037", // timestamp in unix time / or system startup time. seconds with milliseconds separated by a dot. (64bit seconds)
        "src":"faultHandling", 
        "cat":"fault", 
        "code":1,   // event code, event is a text representation
        "event":"AfePsbCommunicationError", // can be "" if the description is missing
        "motor":3 // optional, only set for motor specific faults
        "uid":3   // user id,for all default log entries, but not for the motor faults
    }

```

#### Settings Value Message

currently all settings are "binary", and transferred using a hex-string.

trailing zeros dropped in a setting value message.

```json
{"key":"TSN_VLAN_ID", "format":"binary", "value":"02"} // returned from /api/settings/TSN_VLAN_ID
{"format":"binary", "value":"02"}                      // for a post the key is not required
{"key":"User 1", "format":"hidden"}                    // some configuration keys are hidden from retrieval
```

#### Settings Listing 

The settings endpoint returns a list of predefined settings (not necessary all available settings)

```json
[
{"key":"IP_config", "description":"IP address", "format":"binary", "value":"..."},
{"key":"IP_mask_config", "description":"IP mask", "format":"binary", "value":"FFFFFF"},
{"key":"IP_gateway", "description":"Default gateway", "format":"binary", "value":"..."},
{"key":"IP_DNS", "description":"DNS server address", "format":"binary", "value":"..."},
{"key":"MAC_address", "description":"Ethernet MAC address", "format":"binary", "value":"02"},
{"key":"TSN_VLAN_ID", "description":"TSN vLAN ID", "format":"binary", "value":"02"},
{"key":"TSN_RX_Stream_MAC", "description":"TSN RX stream MAC address", "format":"binary", "value":"..."},
{"key":"TSN_TX_Stream_MAC", "description":"TSN TX stream MAC address", "format":"binary", "value":"..."},
{"key":"MOTD", "description":"System Usage Message", "format":"binary", "value":"51756164204D6F746F7220436F6E74726F6C20326E642047656E2E0A456E7375726520796F75206861766520746865206E656564656420747261696E696E6720746F206F706572617465207468652073797374656D2E0A"},
{"key":"AZURE_IOTHUB_HubName", "description":"Azure IoT Hub name", "format":"binary", "value":"516D633267487562"}
]
```

#### Firmware Upload

The firmware upload endpoint expects two headers:

```http

Accept: application/json
Content-Type: application/octet-stream
```

The return value contains the checksum, byte count  and sector write information.

```json
{
    "bytes":1966964,"sha256":"15EDA94D5D2A0E90E2B9A0706FEC750AA2A812B1638C8B04D64DD49AF60E8150", 
 	"sector_writes":481, 
 	"sector_retry_count":0
}

```



## Website

The QMC webservice serves webpages via GET requests from a ZIP file (`webroot.zip`) embedded into the firmware.

GET Requests not found in the ZIP archive are answered with a 404 "Not Found" response.
all other requests are answered with an 405 "Method not Implemented" Error.

The files might be transferred compressed if the browser supports it and the files are compressed in the zip file.