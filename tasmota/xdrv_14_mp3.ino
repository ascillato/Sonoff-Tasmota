/*
  xdrv_14_mp3.ino - MP3 support for Tasmota

  Copyright (C) 2021  gemu2015, mike2nl and Theo Arends

  This program is free software: you can redistribute it and/or modify
  it under the terms of the GNU General Public License as published by
  the Free Software Foundation, either version 3 of the License, or
  (at your option) any later version.

  This program is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
  GNU General Public License for more details.

  You should have received a copy of the GNU General Public License
  along with this program.  If not, see <http://www.gnu.org/licenses/>.

  --------------------------------------------------------------------------------------------
  Version yyyymmdd  Action    Description
  --------------------------------------------------------------------------------------------
  1.0.0.5 20210121  added   - support for DY_SV17F Player (#define USE_DY_SV17F)
                            - cmds supported:
                            - track
                            - stop
                            - volume
                            - play
                            - play /path

  1.0.0.4 20181003  added   - MP3Reset command in case that the player do rare things
                            - and needs a reset, the default volume will be set again too
                    added   - MP3_CMD_RESET_VALUE for the player reset function
                    cleaned - some comments and added function text header
                    fixed   - missing void's in function calls
                    added   - MP3_CMD_DAC command to switch off/on the dac outputs
                    tested  - works with MP3Device 1 = USB STick, or MP3Device 2 = SD-Card
                            - after power and/or reset the SD-Card(2) is the default device
                            - DAC looks working too on a headset. Had no amplifier for test
  ---
  1.0.0.3 20180915  added   - select device for SD-Card or USB Stick, default will be SD-Card
                    tested  - works by MP3Device 1 = USB STick, or MP3Device 2 = SD-Card
                            - after power and/or reset the SD-Card(2) is the default device
  ---
  1.0.0.2 20180912  added   - again some if-commands to switch() because of new commands
  ---
  1.0.0.1 20180911  added   - command eq (equalizer 0..5)
                    tested  - works in console with MP3EQ 1, the value can be 0..5
                    added   - USB device selection via command in console
                    tested  - looks like it is working
                    erased  - code for USB device about some errors, will be added in a next release
  ---
  1.0.0.1 20180910  changed - command real MP3Stop in place of pause/stop used in the original version
                    changed - the command MP3Play e.g. 001 to MP3Track e.g. 001,
                    added   - new normal command MP3Play and MP3Pause
  ---
  1.0.0.0 20180907  merged  - by arendst
                    changed - the driver name from xdrv_91_mp3.ino to xdrv_14_mp3.ino
  ---
  0.9.0.3 20180906  request - Pull Request
                    changed - if-commands to switch() for faster response
  ---
  0.9.0.2 20180906  cleaned - source code for faster reading
  ---
  0.9.0.1 20180905  added   - #include <TasmotaSerial.h> because compiler error (Arduino IDE v1.8.5)
  ---
  0.9.0.0 20180901  started - further development by mike2nl  - https://github.com/mike2nl/Sonoff-Tasmota
                    base    - code base from gemu2015 ;-)     - https://github.com/gemu2015/Sonoff-Tasmota
                    forked  - from arendst/tasmota            - https://github.com/arendst/Tasmota

*/

#ifdef USE_MP3_PLAYER
/*********************************************************************************************\
 * MP3 control for RB-DFR-562 DFRobot mini MP3 player
 * https://www.dfrobot.com/wiki/index.php/DFPlayer_Mini_SKU:DFR0299
\*********************************************************************************************/

#define XDRV_14             14

#include <TasmotaSerial.h>

TasmotaSerial *MP3Player;

#define  MP3_MAX_TASK_NUMBER      12
uint8_t  MP3_TASK_LIST[MP3_MAX_TASK_NUMBER+1][2];   // first value: kind of task - second value: delay in x * 100ms
uint8_t  MP3_CURRENT_TASK_DELAY = 0;                // number of 100ms-cycles
uint8_t  MP3_LAST_COMMAND;                          // task command code 
uint8_t  MP3_RETRIES            = 3;                // residual code from the era of serial error, solved by gemu2015. This can probably be removed later
uint16_t MP3_CURRENT_PAYLOAD    = 0;                // payload of a supported command
uint16_t MP3_DEBUG_PAYLOAD      = 0;                // for debugging: set payload, after that launch a command code

bool     MP3_LOCK_SCAN          = false;            // don't publish a new JSON before we get a valid scan
bool     MP3_GUI_NEEDS_UPDATE   = false;            // only a flag for the webgui

#define  MP3_MAX_RX_BUF         35
char     MP3_RX_STRING[MP3_MAX_RX_BUF]      = {0};  // make a buffer bigger than the usual 10-byte-message


struct MP3_STATE{
  uint8_t tracksRootSD          = 0;
  uint8_t tracksInActiveFolder  = 0;
  uint8_t foldersRootSD         = 0;                // this are also /MP3 and /ADVERT, and incompatible folders 
  uint8_t Version               = 0;
  uint8_t Volume                = 0;
  uint8_t EQ                    = 0;
  uint8_t Track                 = 0;                // current track
  uint8_t activeFolder          = 0;                // selected folder for further operations
  uint8_t PBMode                = 0;                // stop/play/pause
  uint8_t Device                = 2;                // 1 = USB Stick, 2 = SD-Card
  uint8_t PlayMode              = 0;                // stop/play/pause
};

MP3_STATE MP3State;

#ifdef USE_WEBSERVER
#ifdef USE_MP3_PLAYER_UI
#define WEB_HANDLE_MP3 "mp3"
const char S_CONTROL_MP3[] PROGMEM          = "MP3: "; 
const char S_TASK_MP3[] PROGMEM             = "MP3_TASK -> ";

const char HTTP_BTN_MENU_MAIN_MP3[] PROGMEM = "<p><form action='" WEB_HANDLE_MP3 "' method='get'><button>MP3 Control</button></form></p>";

#define BUTTON_MP3_PLAY    "Play"
#define BUTTON_MP3_STOP    "Stop"
#define BUTTON_MP3_REPEAT  "Repeat"
#define BUTTON_MP3_SHUFFLE "Shuffle"
#define BUTTON_MP3_NEXT    "Next"
#define BUTTON_MP3_PREV    "Prev"
#define BUTTON_MP3_33      "style='width:33.3%%' type='button'"  //type button prevents page reload!

const char HTTP_MP3_1_SNS[] PROGMEM =
  "<fieldset>"
  "<legend><b> MP3 DFPlayer  </b> FWVer %u</legend>"
  "<form>"
  "<div id=on>"
  "<span id='pm'>loading ...</span><span>Track: </span>"
  "<select style='width:20%%' id='t' onchange='sendS(this)'>"
  ;

const char HTTP_MP3_2_SNS[] PROGMEM =
  "</select>"
  "<span> of </span><span id='nt'>%u</span><span> from </span><span id='de'>SD</span>"
  "</div>"
  "<div id=of>"
  "storage ejected !!"
  "</div><br>"
  "<span>EQ:</span>"
  "<select id=eq onchange='sendS(this)' style='width:33.3%%'>"
  "<option value=0>Normal</option>"
  "<option value=1>Pop</option>"
  "<option value=2>Rock</option>"
  "<option value=3>Jazz</option>"
  "<option value=4>Classic</option>"
  "<option value=5>Base</option>"
  "</select><span id ='pbm' class='q'> </span><br><br>"
  "<button " BUTTON_MP3_33 " onclick='bt(1)' id='pp'>" BUTTON_MP3_PLAY "</button>"
  "<button " BUTTON_MP3_33 " onclick='bt(2)'>" BUTTON_MP3_STOP "</button>"
  "<button " BUTTON_MP3_33 " onclick='bt(3)'>" BUTTON_MP3_REPEAT "</button>"
  "<br><br>"
  "<span>VOLUME: </span><span id='vol'>%u</span>"
  "<div><input type='range' min='0' max='100' value='%u' id='vsl' onchange='v(value)'></div>"
  ;

const char HTTP_MP3_3_SNS[] PROGMEM =
  "<br>"
  "<button " BUTTON_MP3_33 "onclick='bt(4)'>" BUTTON_MP3_SHUFFLE "</button>"
  "<button " BUTTON_MP3_33 "onclick='bt(5)'>" BUTTON_MP3_PREV "</button>"
  "<button " BUTTON_MP3_33 "onclick='bt(6)'>" BUTTON_MP3_NEXT "</button>"
  "<br><hr>"
  "<div id='exFo'><p>Found <span id='nf'>0</span> folders:</p><button  type='button' onclick='shEf()'>Folder Scan</button>"
  "<p>WARNING: long blocking task!</p></div>"
  "<progress style='width:100%%' hidden id='prog' value='0' max='99'></progress>"
  "<div id='exFo1' hidden>"
  "Folder: "
  "<select id='af' style='width:20%%' onchange='sendS(this)' style='width:33.3%%'>"     //active folder
  ;

const char HTTP_MP3_4_SNS[] PROGMEM =
  "</select>"
  " contains "
  "<span id='tf'>0</span>"
  " tracks"
  "<br><br>"
  "<button  type='button' onclick='bt(8)'>Play all files in selected folder</button>"
  "</div>"
  "</form>"
  "<hr>"
  "<p style='text-align:center;font-size:12px;'>FW: %s</p>"
  "</fieldset>"
  ;

const char HTTP_MP3_SCRIPT_1_SNS[] PROGMEM =
  "<script>"
  "var pm = ['Stop at ', 'Playing ', 'Paused '];"
  "var pbm = ['Normal Play', 'Folder Repeat', 'Track Repeat', 'Shuffle', 'Single Play'];"
  "var de = ['', 'USB', 'SD'];"
  ;

const char HTTP_MP3_SCRIPT_2_SNS[] PROGMEM =
  "function sendS(b) {"                          //send from selection
  "var s = b.id;"
  "var v = b.options[b.selectedIndex].value;"
  "rl(s,v);"
  "};"
  "function bt(v){var s = 'bt'; rl(s,v);};"
  "function v(v){var s = 'v'; rl(s,v);};"

  "setInterval(function() {"                
  "rl('rl',0);"                                  // 0 = do NOT force refresh
  "},500);"
  "function rl(s,v){"         //source, value
    "var xr=new XMLHttpRequest();"
    "xr.onreadystatechange=function(){"
      "if(xr.readyState==4&&xr.status==200){"
          "if(xr.responseText!=''){" //we always only expect the whole state in the JSON, it does not really matter if this fails here and there
            // "console.log(xr.responseText);" // for debugging only
            "try{upui(JSON.parse(xr.responseText));}catch(e){}" // if error do nothing
          " }"
        "};"
      "};"
    "xr.open('GET','/mp3?'+s+'='+v,true);"
    "xr.send();"
    "};"
  ;

const char HTTP_MP3_SCRIPT_3a_SNS[] PROGMEM =
  "var ffa = [];"         // keep track of found folders that exists and are not empty
  "var awFo; var reFo = 0; var scTi = null;"  //awaited folder, received folder, scan timer
   "function upui(b) {"   // JSON: MP3State.PlayMode, MP3State.Track, MP3State.tracksRootSD, Volume ,MP3State.EQ, MP3State.PBMode, Device, foldercount, tracksinactivefolder, active folder 
      "eb('pm').innerHTML = pm[b[0]];"
      "if(b[0]==1){eb('pp').setAttribute('class', 'button bgrn');eb('pp').innerHTML='Pause';}"
      "else{eb('pp').removeAttribute('class');eb('pp').innerHTML='Play';}"
      "eb('t').selectedIndex = b[1]-1;"
      "eb('nt').innerHTML = b[2];"
      "eb('vol').innerHTML = b[3];"
      "eb('vsl').value = b[3];"
      "eb('eq').selectedIndex = b[4];"
      "eb('pbm').innerHTML = pbm[b[5]];"
      "if(b[6] == 0) {eb('of').hidden = false;"
      "eb('on').hidden = true;}"
      "else{eb('on').hidden = false;"
   ;

const char HTTP_MP3_SCRIPT_3b_SNS[] PROGMEM =
      "eb('of').hidden = true}"
      "eb('de').innerHTML = de[b[6]];"
      "eb('nf').innerHTML = b[7];"
      "reFo = b[9];"
      "eb('prog').value = reFo;"
      "if (b[8] != 0){"
        "if (ffa.includes(reFo) == false){"
            "ffa.push(reFo);"
            "var o=document.createElement('option');"
            "o.value = reFo; o.text = reFo;"
            "eb('af').appendChild(o);"
         "}"
         "for ( var i=0; i < eb('af').options.length; i++ ){"
            "if (eb('af').options[i].value == reFo){eb('af').options[i].selected = true;}"
         "}" 
          "eb('tf').innerHTML = b[8];"
      "}"
      "if (reFo == 0){"
      "localStorage.setItem('ffs','');"
      "}"
  "};"
  "function shEf() {eb('prog').hidden = false; eb('exFo').hidden = true; scTi=setInterval(foSc,50);}" //button folder scan was pressed
  "</script>"
  ;

const char HTTP_MP3_SCRIPT_4_SNS[] PROGMEM =
    "<script>"
    "window.onload = function(){eb('prog').hidden = true;eb('exFo1').hidden = true;ffa=[];" // 1 = force refresh
    "try{ffa = JSON.parse(localStorage.getItem('ffs'));}catch(err){}; rl('rl',1);awFo=reFo;"
    "for (var i=0; i < ffa.length; i++ ){"
            "var o=document.createElement('option');"
            "o.value = ffa[i]; o.text = ffa[i];"
            "if(ffa[i]>0){eb('af').appendChild(o); eb('exFo').hidden = true; eb('exFo1').hidden = false;}" //do not add 0
         "}"
    "};"
    "function foSc() {"  //folder scan
    "if (reFo == awFo && reFo < 100){"
        "awFo++;"
        "rl('fs',awFo);" 
      "}"
    "if (reFo > 99){"
        "clearInterval(scTi);"
        "eb('prog').hidden = true; eb('exFo1').hidden = false; localStorage.setItem('ffs',JSON.stringify(ffa));"
      "}"
    "};"    
    "</script>"
  ;
#endif  // USE_MP3_PLAYER_UI
#endif  // USE_WEBSERVER

/*********************************************************************************************\
 * constants
\*********************************************************************************************/

#define D_CMND_MP3 "MP3"

const char S_JSON_MP3_COMMAND_NVALUE[] PROGMEM = "{\"" D_CMND_MP3 "%s\":%d}";
const char S_JSON_MP3_COMMAND[] PROGMEM        = "{\"" D_CMND_MP3 "%s\"}";
const char kMP3_Commands[] PROGMEM             = "Track|Play|Pause|Stop|Volume|EQ|Device|Reset|DAC|Next|Prev|Volup|Voldown|Pbmode|Shuffle|Advert|Command|Payload|Status";

/*********************************************************************************************\
 * enumerationsines
\*********************************************************************************************/

enum MP3_Commands {                                 // commands useable in console or rules
  CMND_MP3_TRACK,                                   // MP3Track 001...255
  CMND_MP3_PLAY,                                    // MP3Play, after pause or normal start to play
  CMND_MP3_PAUSE,                                   // MP3Pause
  CMND_MP3_STOP,                                    // MP3Stop, real stop, original version was pause function
  CMND_MP3_VOLUME,                                  // MP3Volume 0..100
  CMND_MP3_EQ,                                      // MP3EQ 0..5
  CMND_MP3_DEVICE,                                  // sd-card: 02, usb-stick: 01 ... also use to wake from sleep
  CMND_MP3_RESET,                                   // MP3Reset, a fresh and default restart
  CMND_MP3_DAC,                                     // set dac, 1=off, 0=on, DAC is turned on (0) by default
  CMND_MP3_NEXT,                                    // play next track
  CMND_MP3_PREV,                                    // playprevious track
  CMND_MP3_VOLUP,                                   // volume up
  CMND_MP3_VOLDOWN,                                 // volume down
  CMND_MP3_PBMODE,                                  // playbackmode (0: normal, 1:repeat folder, 2:repeat single, 3:repeat random) - brings unexpected results in some cases!!
  CMND_MP3_SHUFFLE,                                 // start play in shuffle mode, 
  CMND_MP3_ADVERT,                                  // start track with give number/name from /ADVERT, and return to played track
  CMND_MP3_COMMAND,                                 // insert code directly for debugging, i.e. MP3COMMAND 1 for next track
  CMND_MP3_PAYLOAD,                                 // set a payload for debugging purposes (first set payload, then launch command code)
  CMND_MP3_STATUS };                                // query Status

/*********************************************************************************************\
 * command defines
\*********************************************************************************************/

#define MP3_CMD_RESET_VALUE 0                       // mp3 reset command value
// player commands
#define MP3_CMD_NEXT        0x01                    // playback of the next track
#define MP3_CMD_PREV        0x02                    // playback of the prev track
#define MP3_CMD_TRACK       0x03                    // specify playback of a track, e.g. MP3Track 003
#define MP3_CMD_VOLUP       0x04                    // volume up one step
#define MP3_CMD_VOLDOWN     0x05                    // volume down one step
#define MP3_CMD_VOLUME      0x06                    // specifies the volume and means a console input as 0..100
#define MP3_CMD_EQ          0x07                    // specify EQ(0/1/2/3/4/5), 0:Normal, 1:Pop, 2:Rock, 3:Jazz, 4:Classic, 5:Bass
#define MP3_CMD_PBMODE      0x08                    // specify Playbackmode
#define MP3_CMD_DEVICE      0x09                    // specify playback device, USB=1, SD-Card=2, default is 2 also after reset or power down/up
#define MP3_CMD_RESET       0x0c                    // send a reset command to start fresh
#define MP3_CMD_PLAY        0x0d                    // Play, works as a normal play on a real MP3 Player, starts at 001.mp3 file on the selected device
#define MP3_CMD_PAUSE       0x0e                    // Pause, was original designed as stop, see data sheet
#define MP3_CMD_PLAY_T_F    0x0f      //??          // cmd ff tt = Play from folder ff=0-63, track=01-ff
#define MP3_CMD_VOL_GAIN    0x10      //??          // cmd 00 vv = gain vv=00-1f and cmd 01 vv = adjust vv=00-1F
#define MP3_CMD_REPEAT_PLAY 0x11                    // cmd 00 01 = repeat play and cmd 00 00 = normal mode(??)
#define MP3_CMD_TRACK_MP3   0x12                    // cmd tt tt = play track from folder MP3
#define MP3_CMD_TRACK_ADV   0x13                    // cmd tt tt = play track from folder ADVERT
#define MP3_CMD_PLAY_T_F_2  0x14                    // cmd ft tt = play folder f=1-F, track ttt=001-3E7
#define MP3_CMD_STOP_ADV    0x15                    // stop advert, resume original track
#define MP3_CMD_STOP        0x16                    // Stop, it's a real stop now, in the original version it was a pause command
#define MP3_CMD_REPEAT_F    0x17                    // cmd 00 ff =set playback to repeat of  folder f=0-FF
#define MP3_CMD_SHUFFLE     0x18                    // start shuffle playback
#define MP3_CMD_LOOP_T      0x19                    // cmd 00 00  start looping of current track, cmd 00 01 end looping of current track
#define MP3_CMD_DAC         0x1A                    // cmd 00 01 = mute, cmd 00 00 = cancel mute
// only "passive" receive codes
#define MP3_CMD_R_PUSHIN    0x3a                    // device returns push in of storage device
#define MP3_CMD_R_PULLOUT   0x3b                    // device returns pull out of storage device
#define MP3_CMD_R_USB_FIN   0x3c                    // device returns finished track on USB
#define MP3_CMD_R_SD_FIN    0x3d                    // device returns finished track on SD
#define MP3_CMD_R_STORAGE   0x3f                    // device returns online storage device
#define MP3_CMD_R_ERROR     0x40                    // device returns an error code
// codes to trigger a query
#define MP3_CMD_Q_STAT      0x42                    // query status (stop/play/pause)
#define MP3_CMD_Q_VOLUME    0x43                    // query Volume
#define MP3_CMD_Q_EQ        0x44                    // query EQ
#define MP3_CMD_Q_PBMODE    0x45                    // query playback mode
#define MP3_CMD_Q_VERSION   0x46                    // query software version
#define MP3_CMD_Q_USB_NUM   0x47                    // query number of tracks on root folder of USB
#define MP3_CMD_Q_SD_NUM    0x48                    // query number of tracks on root folder of micro sd card
#define MP3_CMD_Q_FL_NUM    0x49                    // query number of tracks on root folder of flash
#define MP3_CMD_Q_USB_TRACK 0x4b                    // query current track on USB
#define MP3_CMD_Q_SD_TRACK  0x4c                    // query current track on micro sd card
#define MP3_CMD_Q_FL_TRACK  0x4d                    // query current track on flash
#define MP3_CMD_Q_TRACK_F   0x4e                    // query number of tracks in folder
#define MP3_CMD_Q_FOLDERS   0x4f                    // query folder count on SD

/*********************************************************************************************\
 * return codes defines
\*********************************************************************************************/

#define MP3_ERR_BUSY          0x01
#define MP3_ERR_SLEEPING      0x02
#define MP3_ERR_WRONGSTACK    0x03
#define MP3_ERR_WRONGCHECKSUM 0x04
#define MP3_ERR_FILEINDEXOUT  0x05
#define MP3_ERR_FILEMISMATCH  0x06
#define MP3_ERR_ADVERTISE     0x07

/*********************************************************************************************\
 * Task codes defines
\*********************************************************************************************/

 #define TASK_MP3_NOTASK          0                         // nothing to be done
 #define TASK_MP3_FEEDBACK        1                         // check the feedback from the device
 #define TASK_MP3_RESET_VOLUME    2                         // reset the Volume to default in the next loop
 #define TASK_MP3_RESET_DEVICE    3                         // reset the Volume to default in the next loop
 #define TASK_MP3_PLAY            4                         // play in the next loop
 #define TASK_MP3_STOP            5                         // stop in the next loop
 #define TASK_MP3_PAUSE           6                         // pause in the next loop
 #define TASK_MP3_Q_STATUS        7                         // query in the next loop
 #define TASK_MP3_TRACK           8                         // select track in the next loop
 #define TASK_MP3_VOLUME          9                         // select volume in the next loop
 #define TASK_MP3_EQ              10                        // select EQ in the next loop
 #define TASK_MP3_DEVICE          11                        // select device in the next loop
 #define TASK_MP3_DAC             12                        // select DAC in the next loop
 #define TASK_MP3_Q_NUM_SD        13                        // query number of tracks in root of SD in the next loop
 #define TASK_MP3_NEXT            14                        // chose next track
 #define TASK_MP3_PREV            15                        // chose previous track
 #define TASK_MP3_VOLUP           16                        // volume up one unit
 #define TASK_MP3_VOLDOWN         17                        // volume down one unit
 #define TASK_MP3_COMMAND         18                        // inject command code - useful in conjunction with MP3PAYLOAD
 #define TASK_MP3_PBMODE          19                        // select playbackmode TODO: ceck if this really works  
 #define TASK_MP3_SHUFFLE         20                        // start play in shuffle mode
 #define TASK_MP3_Q_VERSION       21                        // read software version
 #define TASK_MP3_Q_PBMODE        22                        // query playbackmode TODO: ceck if this really works 
 #define TASK_MP3_Q_VOLUME        23                        // query volume in the next loop
 #define TASK_MP3_Q_EQ            24                        // query EQ in the next loop
 #define TASK_MP3_Q_TRACK         25                        // query TRACK in the next loop
 #define TASK_MP3_Q_VERSION_DATE  26                        // query the FW-Date-String
 #define TASK_MP3_Q_FOLDER_COUNT  27                        // query number of folders on storage
 #define TASK_MP3_Q_TRACKS_IN_F   28                        // query number of tracks in active folder
 #define TASK_MP3_B_SCAN_FOLDERS  29                        // scan for folders on SD with tracks from 01 .. 99
 #define TASK_MP3_REPEAT_F        30                        // repeat playback of active folder
 #define TASK_MP3_ADVERT          31                        // insert track from folder /ADVERT, which will stop after finished and continue
 #define TASK_MP3_SEND_AJAX       32                        // send state as JSON via AJAX to the WebGUI
 #define TASK_MP3_DONE            99                        // used, if there was a task in the slot or just to wait

/*********************************************************************************************\
 * Helper functions
\*********************************************************************************************/

void MP3_Launchtask(uint8_t task, uint8_t slot, uint8_t delay){
                          MP3_TASK_LIST[slot][0]   = task;
                          MP3_TASK_LIST[slot][1]   = delay;
                          MP3_TASK_LIST[slot+1][0] = TASK_MP3_NOTASK;           // the tasks must always be launched in ascending order!!
                          MP3_CURRENT_TASK_DELAY   = MP3_TASK_LIST[0][1];
}

void MP3_TaskReplaceInSlot(uint8_t task, uint8_t slot){
                          MP3_LAST_COMMAND         = MP3_TASK_LIST[slot][0];    // save command
                          MP3_TASK_LIST[slot][0]   = task;  
}

void MP3_Reset(void) {    MP3_LAST_COMMAND         = TASK_MP3_DONE;             // task command code 
                          MP3State.activeFolder    = 0;                         // see the line above
                          MP3_Launchtask(TASK_MP3_DONE,0,10);                   // just wait for some time , equals delay(1000) -> 10 * 100
                          MP3_Launchtask(TASK_MP3_RESET_DEVICE,   1,0);         // reset Device
                          MP3_Launchtask(TASK_MP3_Q_VERSION,      2,10);        // read SW Version at startup
                          MP3_Launchtask(TASK_MP3_Q_NUM_SD,       3,0);         // read number of tracks
                          MP3_Launchtask(TASK_MP3_Q_VERSION_DATE ,4,0);         // get FW-Date-String, seems not to be supported with FW < 8
                          MP3_Launchtask(TASK_MP3_Q_FOLDER_COUNT ,5,0);         // get number of folders
                          MP3_Launchtask(TASK_MP3_Q_STATUS ,      6,0);         // read playmode
                          MP3_Launchtask(TASK_MP3_Q_TRACK ,       7,0);         // get current track
                          MP3_Launchtask(TASK_MP3_Q_EQ,           8,0);         // read EQ setting
                          MP3_Launchtask(TASK_MP3_Q_PBMODE,       9,0);         // read Playback mode
                          MP3_Launchtask(TASK_MP3_RESET_VOLUME,   10,0);        // reset volume and set MP3_BOOT_COMPLETED to true
                          MP3_GUI_NEEDS_UPDATE      = true;}                    // ASAP

void MP3SendAJAXJSON(void){
      char tmp[50];
      sprintf (tmp, "[\"%u\",\"%u\",\"%u\",\"%u\",\"%u\",\"%u\",\"%u\",\"%u\",\"%u\",\"%u\"]", MP3State.PlayMode, MP3State.Track, MP3State.tracksRootSD, MP3State.Volume, MP3State.EQ, MP3State.PBMode, MP3State.Device, MP3State.foldersRootSD, MP3State.tracksInActiveFolder, MP3State.activeFolder);
      AddLog_P2(LOG_LEVEL_DEBUG, "%sAJAX-JSON: %s", S_CONTROL_MP3, tmp);
      WebServer->send(200,"text/html",tmp);
      MP3_GUI_NEEDS_UPDATE = false;
}

/*********************************************************************************************\
 * calculate the checksum
 * starts with cmd[1] with a length of 6 bytes
\*********************************************************************************************/

uint16_t MP3_Checksum(uint8_t *array)
{
  uint16_t checksum = 0;
  for (uint32_t i = 0; i < 6; i++) {
    checksum += array[i];
  }
  checksum = checksum^0xffff;
  return (checksum+1);
}

/*********************************************************************************************\
 * update and handle Web GUI
\*********************************************************************************************/

void MP3UpdateGUI(void){
  WSContentStart_P(S_CONTROL_MP3);
  WSContentSendStyle();
  WSContentSend_P(HTTP_MP3_SCRIPT_1_SNS);
  WSContentSend_P(HTTP_MP3_SCRIPT_2_SNS);
  WSContentSend_P(HTTP_MP3_SCRIPT_3a_SNS);
  WSContentSend_P(HTTP_MP3_SCRIPT_3b_SNS);
  WSContentSend_PD(HTTP_MP3_1_SNS, MP3State.Version);
  for (uint8_t i=0; i< MP3State.tracksRootSD;i++) {
    WSContentSend_PD("<option value=%u>%u</option>",i,i+1);
  }
  WSContentSend_PD(HTTP_MP3_2_SNS, MP3State.tracksRootSD, MP3State.Volume, MP3State.Volume, MP3State.PBMode);
  WSContentSend_PD(HTTP_MP3_3_SNS);
  // start unpopulated selection
  WSContentSend_PD(HTTP_MP3_4_SNS, MP3_RX_STRING);
  WSContentSpaceButton(BUTTON_MAIN);
  WSContentSend_P(HTTP_MP3_SCRIPT_4_SNS);             //always refresh the page with onload() at the end
  WSContentStop();
}

void MP3HandleWebGuiResponse(void){         
  char tmp[8];
  WebGetArg("rl", tmp, sizeof(tmp));                  // reload
  if (strlen(tmp)) {           
    uint8_t reload = atoi(tmp);
    if((MP3_GUI_NEEDS_UPDATE || reload == 1) && !MP3_LOCK_SCAN) {         // TODO: Maybe use two different JSON's in the future for frequent and non-frequent changes
        MP3SendAJAXJSON();
    }
    else {
      WebServer->send(200,"text/html","");
    }
    return;
  }
  WebGetArg("fs", tmp, sizeof(tmp));                  // folder scan
  if (strlen(tmp) && !MP3_LOCK_SCAN) {
    uint8_t folder        = atoi(tmp);
    MP3State.activeFolder = folder;
    MP3State.PlayMode     = 0;                      // stops as side effect
    MP3_LOCK_SCAN         = true;                   // wait for valid scan
    MP3_Launchtask(TASK_MP3_Q_TRACKS_IN_F,0,0);
    // MP3_Launchtask(TASK_MP3_SEND_AJAX,1,0);
    AddLog_P2(LOG_LEVEL_DEBUG, PSTR("%sscan folder is now %u"),S_CONTROL_MP3 ,folder);
    return;
  }
  WebGetArg("bt", tmp, sizeof(tmp));                  // button?
  if (strlen(tmp)) {
    uint8_t button = atoi(tmp);
    switch (button){  
    case 1:             //use button for play/pause
      if(MP3State.PlayMode ==1) {
      MP3_Launchtask(TASK_MP3_PAUSE,0,0);
      }
      else {
      MP3_Launchtask(TASK_MP3_PLAY,0,0);
      MP3_Launchtask(TASK_MP3_Q_TRACK,1,1);
      MP3_Launchtask(TASK_MP3_Q_PBMODE,2,1);
      }                      
      break;
    case 2:
      MP3_Launchtask(TASK_MP3_STOP,0,0);
      MP3_LOCK_SCAN = false;
      break;
    case 3:                                           // TODO: Find useful ways to toggle various repeat modes
      AddLog_P2(LOG_LEVEL_DEBUG, PSTR("%sRepeat button pressed"),S_CONTROL_MP3);
      switch(MP3State.PBMode) {
        case 0:
          MP3State.PBMode = 2;
          break;
        case 2:
          MP3State.PBMode = 4;
          break;
        case 4:
          MP3State.PBMode = 0;
          break;
      }
      MP3_Launchtask(TASK_MP3_Q_PBMODE,0,0);
      break;
    case 4:
      MP3_Launchtask(TASK_MP3_SHUFFLE,0,0);
      MP3_Launchtask(TASK_MP3_Q_TRACK,1,1);
      break;
     case 5:
      MP3_Launchtask(TASK_MP3_PREV,0,0);
      MP3_Launchtask(TASK_MP3_Q_PBMODE,1,1);
      // MP3_Launchtask(TASK_MP3_Q_STATUS ,2,1);
      break;
    case 6:
      MP3_Launchtask(TASK_MP3_NEXT,0,0);
      MP3_Launchtask(TASK_MP3_Q_PBMODE,1,1);
      // MP3_Launchtask(TASK_MP3_Q_STATUS ,2,1);
      break;
    // case 7:
    //   AddLog_P2(LOG_LEVEL_DEBUG, PSTR("%sstart long folder scan 01..99"),S_CONTROL_MP3);
    //   break;
    case 8:
      MP3State.PlayMode = 1;
      MP3State.PBMode   = 1;                          // folder repeat play
      MP3_Launchtask(TASK_MP3_REPEAT_F,0,0);
      break;
    }
    WebServer->send(200,"text/html","");
    return;
  }
  WebGetArg("v", tmp, sizeof(tmp));                   // volume
    if (strlen(tmp)) {
      MP3_CURRENT_PAYLOAD = atoi(tmp);
      AddLog_P2(LOG_LEVEL_DEBUG, PSTR("%sVolume from GUI %u"),S_CONTROL_MP3 ,MP3_CURRENT_PAYLOAD);
      MP3_Launchtask(TASK_MP3_VOLUME,0,0);
      WebServer->send(200,"text/html","");
      return;
    }
  WebGetArg("eq", tmp, sizeof(tmp));                  // eq
    if (strlen(tmp)) {
      uint8_t eq          = atoi(tmp);
      MP3_CURRENT_PAYLOAD = eq;
      MP3_Launchtask(TASK_MP3_EQ,0,0);
      WebServer->send(200,"text/html","");
      return;
    }
  WebGetArg("t", tmp, sizeof(tmp));                   // track
    if (strlen(tmp)) {
      uint8_t track       = atoi(tmp);
      MP3_CURRENT_PAYLOAD = track+1;
      MP3_Launchtask(TASK_MP3_TRACK,0,0);
      MP3State.PBMode     = 0;
      WebServer->send(200,"text/html","");
      return;
    }
  WebGetArg("af", tmp, sizeof(tmp));                  // active folder
    if (strlen(tmp)) {
      uint8_t folder        = atoi(tmp);
      MP3State.activeFolder = folder;
      MP3State.PlayMode     = 0;                      // stops as side effect
      MP3_Launchtask(TASK_MP3_Q_TRACKS_IN_F,0,0);
      AddLog_P2(LOG_LEVEL_DEBUG, PSTR("%sactive folder is now %u"),S_CONTROL_MP3 ,MP3State.activeFolder);
      WebServer->send(200,"text/html","");
      return;
    }
}

void MP3HandleWebGui(void){
  if (!HttpCheckPriviledgedAccess()) { return; }
  // AddLog_P(LOG_LEVEL_DEBUG, S_LOG_HTTP, S_CONTROL_MP3);
  MP3HandleWebGuiResponse();
  MP3UpdateGUI();
}

/*********************************************************************************************\
 * init player
 * define serial tx port fixed with 9600 baud
\*********************************************************************************************/

void MP3PlayerInit(void) {
  MP3Player = new TasmotaSerial(Pin(GPIO_RXD), Pin(GPIO_MP3_DFR562));                          //use RX from serial  as a little hack
  AddLog_P2(LOG_LEVEL_DEBUG, PSTR("%sstart serial communication fixed to 9600 baud"),S_CONTROL_MP3);
  if (MP3Player->begin(9600)) {
    MP3Player->flush();
    AddLog_P2(LOG_LEVEL_DEBUG, PSTR("%sflush done, now init MP3_TASK_LIST"),S_CONTROL_MP3);
    MP3_Reset();
    AddLog_P2(LOG_LEVEL_DEBUG, PSTR("%sMP3_TASK_LIST initialized, now return to main loop"),S_CONTROL_MP3);
  }
  return;
}

#ifdef USE_DY_SV17F

/*********************************************************************************************\
 * specific for DY_SV17F
 * create the MP3 commands payload, and send it via serial interface to the MP3 player
 * only track,play,stop and volume supported
\*********************************************************************************************/

void MP3_SendCmd(uint8_t *scmd, uint8_t len) {
uint16_t sum = 0;
  for (uint32_t cnt = 0; cnt < len; cnt++) {
    sum += scmd[cnt];
  }
  scmd[len] = sum;
  MP3Player->write(scmd, len + 1);
}

void MP3_CMD(uint8_t mp3cmd, uint16_t val) {
  uint8_t scmd[8];
  uint8_t len = 0;
  scmd[0]=0xAA;
  switch (mp3cmd) {
    case MP3_CMD_TRACK:
      scmd[1]=0x07;
      scmd[2]=0x02;
      scmd[3]=val>>8;
      scmd[4]=val;
      MP3_SendCmd(scmd, 5);
    case MP3_CMD_PLAY:
      scmd[1]=0x02;
      scmd[2]=0x00;
      scmd[3]=0xAC;
      len = 4;
      break;
    case MP3_CMD_STOP:
      scmd[1]=0x10;
      scmd[2]=0x00;
      scmd[3]=0xBA;
      len = 4;
      break;
    case MP3_CMD_VOLUME:
      scmd[1]=0x13;
      scmd[2]=0x01;
      scmd[3]=val;
      len = 4;
      break;
    default:
      return;
  }
  MP3_SendCmd(scmd, len);
}

#else
/*********************************************************************************************\
 * create the MP3 commands payload, and send it via serial interface to the MP3 player
 * data length is 6 = 6 bytes [FF 06 09 00 00 00] but not counting the start, end, and verification.
 * {start byte, version, length, command, feedback, para MSB, para LSB, chks MSB, chks LSB, end byte};
 * {cmd[0]    , cmd[1] , cmd[2], cmd[3] , cmd[4]  , cmd[5]  , cmd[6]  , cmd[7]  , cmd[8]  , cmd[9]  };
 * {0x7e      , 0xff   , 6     , 0      , 0/1     , 0       , 0       , 0       , 0       , 0xef    };
\*********************************************************************************************/

void MP3_CMD(uint8_t mp3cmd,uint16_t val) {
  uint8_t i       = 0;
  while(MP3Player->available()) {
    i = MP3Player->read();        // copy to trash - NOT SURE ABOUT IT
  }
  i               = 0;
  uint8_t cmd[10] = {0x7e,0xff,6,0,0,0,0,0,0,0xef}; // fill array
  cmd[3]          = mp3cmd;                         // mp3 command value
  cmd[4]          = 0;                              // feedback, 1=yes, 0=no, yet not use
  cmd[5]          = val>>8;                         // data value, shift 8 byte right
  cmd[6]          = val;                            // data value low byte
  uint16_t chks   = MP3_Checksum(&cmd[1]);          // see calculate the checksum
  cmd[7]          = chks>>8;                        // checksum. shift 8 byte right
  cmd[8]          = chks;                           // checksum low byte
  MP3Player->write(cmd, sizeof(cmd));               // write mp3 data array to player
  delay(1000);
  if (mp3cmd == MP3_CMD_RESET) {
    MP3_CMD(MP3_CMD_VOLUME, MP3_VOLUME);            // after reset set volume depending on the entry in the my_user_config.h
  }
  return;
}

#endif // USE_DY_SV17F

/*********************************************************************************************\
 * handle the return value from the DF-MP3
\*********************************************************************************************/

bool MP3PlayerHandleFeedback(){
  bool success    = true;                           // true disables possible repetition of commands, set to false only for debugging
  uint8_t i       = 0;
  uint8_t ret[MP3_MAX_RX_BUF] = {0};                // reset array with zeros
  // AddLog_P2(LOG_LEVEL_DEBUG, PSTR("%swaiting for response"),S_CONTROL_MP3);
  bool receive_data_message = false;                // special response with the format d,a, ...,a,d

  while(MP3Player->available()) {
    // delay(0);
    if(i<MP3_MAX_RX_BUF){
      ret[i] = MP3Player->read();
    }
    else{
      ret[MP3_MAX_RX_BUF-1] = MP3Player->read(); // flush the end of the buffer, we can not handle the data meaningful anyway
    }
    i++;
  }
  if (ret[0] == 0xd && ret[1] == 0xa){              // data String
      AddLog_P2(LOG_LEVEL_DEBUG, PSTR("%sfound d-a-message of length: %u"),S_CONTROL_MP3 ,i);
      bool charsLeft = true;
      i              = 2;
      while(charsLeft){
        if(ret[i] != 0xd && ret[i+1] != 0xa) {
          MP3_RX_STRING[i-2] = ret[i];
        }
        else{
          charsLeft        = false;
          MP3_RX_STRING[i] = 0;                     //terminate the string
        }
        if(i>33){
          AddLog_P2(LOG_LEVEL_DEBUG, PSTR("%sString gets too big ... stopping"),S_CONTROL_MP3);
          charsLeft = false;
        }
        i++;
      }
      AddLog_P2(LOG_LEVEL_DEBUG, PSTR("%sda-msg: %s"),S_CONTROL_MP3 ,MP3_RX_STRING);
  }
  if (ret[0] != 0x7e && ret[1] != 0xff && (uint32_t)ret[0] != 0) {     // do not really react to unexpected format, but always show for debugging
     AddLog_P2(LOG_LEVEL_DEBUG, PSTR("%sUnusual return message %x, %x, %x, %x, %x, %x, %x, %x, %x, %x"),S_CONTROL_MP3, ret[0], ret[1], ret[2], ret[3], ret[4], ret[5], ret[6], ret[7], ret[8], ret[9]);
     return true;
  }
  if (MP3_TASK_LIST[0][0] != TASK_MP3_NOTASK) {     // when any task runs, report everything
    AddLog_P2(LOG_LEVEL_DEBUG, PSTR("%sReturn message %x, %x, %x, %x, %x, %x, %x, %x, %x, %x"),S_CONTROL_MP3 ,ret[0], ret[1], ret[2], ret[3], ret[4], ret[5], ret[6], ret[7], ret[8], ret[9]);
  }
  else{
    if((uint32_t)ret[0] != 0) {                     // when in sniffing mode, do not report "nothing"
      AddLog_P2(LOG_LEVEL_DEBUG, PSTR("%sReturn message %x, %x, %x, %x, %x, %x, %x, %x, %x, %x"),S_CONTROL_MP3 ,ret[0], ret[1], ret[2], ret[3], ret[4], ret[5], ret[6], ret[7], ret[8], ret[9]);
    }
    if (ret[3] == MP3_CMD_R_STORAGE) {
      MP3State.Device = ret[6];
      AddLog_P2(LOG_LEVEL_DEBUG, PSTR("%sonline storage is: %u (might be an error)"),S_CONTROL_MP3 ,MP3State.Device);
      MP3_Launchtask(TASK_MP3_Q_STATUS,0,0);
    }
    else if (ret[3] == MP3_CMD_R_SD_FIN) {
      AddLog_P2(LOG_LEVEL_DEBUG, PSTR("%sSD track finished: %u"),S_CONTROL_MP3 ,ret[6]);
      switch(MP3State.PBMode) {
        case 0: // next
          MP3_Launchtask(TASK_MP3_NEXT,0,0);
          MP3State.PlayMode = 1;
          break;
        case 1: // folder repeat
          MP3_Launchtask(TASK_MP3_Q_TRACK,0,0);
          break;
        case 2: // repeat same
          MP3_Launchtask(TASK_MP3_PLAY,0,0);
          MP3State.PlayMode = 1;
          break;
        case 3: // shuffle
          MP3_Launchtask(TASK_MP3_Q_TRACK,0,0);
          break;
        case 4: // stop
          MP3_Launchtask(TASK_MP3_STOP,0,0);
          MP3State.PlayMode = 0;
          break; 
      }
    }
    else if (ret[3] == MP3_CMD_R_USB_FIN) {
      AddLog_P2(LOG_LEVEL_DEBUG, PSTR("%sUSB track finished: %u"),S_CONTROL_MP3 ,ret[6]);
      switch(MP3State.PBMode){
        case 0: // next
          MP3_Launchtask(TASK_MP3_NEXT,0,0);
          MP3State.PlayMode = 1;
          break;
        case 2: // repeat same
          MP3_Launchtask(TASK_MP3_PREV,0,0);
          MP3State.PlayMode = 1;
          break;
        case 4: // stop
          MP3_Launchtask(TASK_MP3_STOP,0,0);
          MP3State.PlayMode = 0;
          break;
      }
      MP3_GUI_NEEDS_UPDATE = true;
    }
    else if (ret[3] == MP3_CMD_R_PULLOUT) {
      AddLog_P2(LOG_LEVEL_DEBUG, PSTR("%sPull-out of: %u"),S_CONTROL_MP3 ,ret[6]);
      MP3State.Device=0;
      MP3_GUI_NEEDS_UPDATE = true;
    }
    else if (ret[3] == MP3_CMD_R_PUSHIN) {
      MP3State.Device=ret[6];
      AddLog_P2(LOG_LEVEL_DEBUG, PSTR("%sPush-in of: %u, will reset device"),S_CONTROL_MP3 ,MP3State.Device);
      MP3_Reset();
    }
  }
  // AddLog_P2(LOG_LEVEL_DEBUG, PSTR("%sLast task %x"),S_CONTROL_MP3 ,MP3_LAST_COMMAND);

  switch(ret[3]) {
    case MP3_CMD_Q_STAT:
      MP3State.PlayMode = ret[6];
      AddLog_P2(LOG_LEVEL_DEBUG, PSTR("%sOperation likely successful, will query to be sure and repeat command if neccessary"),S_CONTROL_MP3);       // this is "no respone = no error"
      if (MP3_LAST_COMMAND == TASK_MP3_PLAY && MP3State.PlayMode == 1) success = true;
      else if (MP3_LAST_COMMAND == TASK_MP3_STOP && MP3State.PlayMode == 0) success = true;
      else if (MP3_LAST_COMMAND == TASK_MP3_PAUSE && MP3State.PlayMode != 1) success = true;
      break;
    case MP3_CMD_Q_VOLUME:
      MP3State.Volume = (ret[6]*100)/30;
      AddLog_P2(LOG_LEVEL_DEBUG, PSTR("%sVolume is set to: %u"),S_CONTROL_MP3 ,MP3State.Volume);
      success = true;
      break;
    case MP3_CMD_Q_SD_TRACK:
      MP3State.Track = ret[6];
      AddLog_P2(LOG_LEVEL_DEBUG, PSTR("%sCurrent Track is set to: %u"),S_CONTROL_MP3 ,MP3State.Track);
      success = true;
      break;
    case MP3_CMD_Q_EQ:
      MP3State.EQ = ret[6];
      AddLog_P2(LOG_LEVEL_DEBUG, PSTR("%sEQ is set to: %u"),S_CONTROL_MP3 ,MP3State.EQ);
      success = true;
      break;
    case MP3_CMD_Q_SD_NUM:
      MP3State.tracksRootSD = ret[6];
      AddLog_P2(LOG_LEVEL_DEBUG, PSTR("%sTracks on SD: %u"),S_CONTROL_MP3 ,MP3State.tracksRootSD);
      success = true;
      break;
    case MP3_CMD_R_STORAGE:
      MP3State.Device = ret[6];
      AddLog_P2(LOG_LEVEL_DEBUG, PSTR("%sfollowing device online: %u"),S_CONTROL_MP3 ,MP3State.Device);
      success = true;
      break;
    case MP3_CMD_Q_PBMODE:
      if (ret[6] == 1 || ret[6] == 3) {     // only change for shuffle and folder repeat play, the rest is managed internally
        MP3State.PBMode = ret[6];
      }
      AddLog_P2(LOG_LEVEL_DEBUG, PSTR("%splaybackmode: %u"),S_CONTROL_MP3 ,MP3State.PBMode);
      success = true;
      break;
    case MP3_CMD_Q_VERSION:
      MP3State.Version = ret[6];
      AddLog_P2(LOG_LEVEL_DEBUG, PSTR("%sDevice SW version: %u"),S_CONTROL_MP3 ,MP3State.Version);
      success = true;
      break;
    case MP3_CMD_Q_FOLDERS:
      MP3State.foldersRootSD = ret[6];
      AddLog_P2(LOG_LEVEL_DEBUG, PSTR("%sNumber of Folders on SD: %u"),S_CONTROL_MP3 ,MP3State.foldersRootSD);
      success = true;
      break;
    case MP3_CMD_Q_TRACK_F:
      MP3State.tracksInActiveFolder = ret[6];
      MP3_GUI_NEEDS_UPDATE = true;
      AddLog_P2(LOG_LEVEL_DEBUG, PSTR("%sNumber of Tracks in Folder %u: %u"),S_CONTROL_MP3 ,MP3State.activeFolder, MP3State.tracksInActiveFolder);
      MP3_LOCK_SCAN = false;      // ready for new scan
      success = true;
      break;
    case  MP3_CMD_R_ERROR:
      AddLog_P2(LOG_LEVEL_DEBUG, PSTR("%sGot error message: %x"),S_CONTROL_MP3 ,ret[6]);
      switch(ret[6]) {
        case MP3_ERR_SLEEPING:
          AddLog_P2(LOG_LEVEL_DEBUG, PSTR("%sentered sleep mode"),S_CONTROL_MP3);
          success = true;
          break;
        case MP3_ERR_FILEINDEXOUT:                  // ??FW 5 ??
          if(MP3_LOCK_SCAN) {          
            MP3State.tracksInActiveFolder = 0;
            MP3_GUI_NEEDS_UPDATE = true;
            AddLog_P2(LOG_LEVEL_DEBUG, PSTR("%sNo Folder %02u"),S_CONTROL_MP3,MP3State.activeFolder);
            MP3_LOCK_SCAN = false;                  // ready for new scan
            success = true;
          }
        case MP3_ERR_FILEMISMATCH:                  // FW 8
          if(MP3_LOCK_SCAN) {
            MP3State.tracksInActiveFolder = 0;
            MP3_GUI_NEEDS_UPDATE = true;
            AddLog_P2(LOG_LEVEL_DEBUG, PSTR("%sNo Folder %02u"),S_CONTROL_MP3,MP3State.activeFolder);
            MP3_LOCK_SCAN = false;                  // ready for new scan
            success = true;
          }
          break;
      }
      break;
    default:
      // AddLog_P2(LOG_LEVEL_DEBUG, PSTR("%sUnknown or serial error - will retry"),S_CONTROL_MP3);     // TODO: if needed, add retry-counter
      if(MP3_LOCK_SCAN){
        MP3_Launchtask(TASK_MP3_FEEDBACK,0,0);
        MP3_GUI_NEEDS_UPDATE = false;
        }
  }
  return success;
}

/*********************************************************************************************\
 * execute the next Task
\*********************************************************************************************/

void MP3_TaskEvery100ms(){
  // AddLog_P2(LOG_LEVEL_DEBUG, PSTR("%sMP3_TASK to be done %u"),S_CONTROL_MP3,MP3_TASK_LIST[0][0]);
  if (MP3_CURRENT_TASK_DELAY == 0)  {
    uint8_t i = 0;
    bool runningTaskLoop = true;
    while (runningTaskLoop) {                                          // always iterate through the whole task list
      switch(MP3_TASK_LIST[i][0]) {                                 // handle the kind of task
        case TASK_MP3_FEEDBACK:
          AddLog_P2(LOG_LEVEL_DEBUG, PSTR("%s%sFeedback"),S_CONTROL_MP3,S_TASK_MP3);
          if(MP3_RETRIES>0) {
            if (MP3PlayerHandleFeedback()) {
              MP3_TASK_LIST[i][0] = TASK_MP3_DONE;                 // mark slot as handled if successful
              MP3_CURRENT_TASK_DELAY = MP3_TASK_LIST[i+1][1];      // assign the delay of the next slot to the current global delay
            }
            else {
              MP3_TASK_LIST[i][0] = MP3_LAST_COMMAND;           // reinsert unsuccessful task into the current slot
              MP3_CURRENT_TASK_DELAY++;
              MP3_RETRIES--;
            }
          }
          else {
            MP3_TASK_LIST[i][0] = TASK_MP3_DONE;                 // mark slot as handled even if not successful
            MP3_CURRENT_TASK_DELAY = MP3_TASK_LIST[i+1][1];      // assign the delay of the next slot to the current global delay
            MP3_RETRIES = 3;
          }
          runningTaskLoop = false;                               // return to main loop
          break;
        case TASK_MP3_PLAY:
          MP3_CMD(MP3_CMD_PLAY, 0);
          AddLog_P2(LOG_LEVEL_DEBUG, PSTR("%s%sPlay"),S_CONTROL_MP3,S_TASK_MP3);
          MP3State.PlayMode = 1;
          MP3_TaskReplaceInSlot(TASK_MP3_Q_TRACK,i);
          runningTaskLoop = false;
          break;
        case TASK_MP3_STOP:
          AddLog_P2(LOG_LEVEL_DEBUG, PSTR("%s%sStop"),S_CONTROL_MP3,S_TASK_MP3);
          MP3_CMD(MP3_CMD_STOP, 0);
          MP3State.PBMode = 0; //end shuffle and folder repeat mode
          MP3State.PlayMode = 0;
          MP3_TaskReplaceInSlot(TASK_MP3_Q_STATUS,i);
          runningTaskLoop = false;
          break;
        case TASK_MP3_PAUSE:
          AddLog_P2(LOG_LEVEL_DEBUG, PSTR("%s%sPause"),S_CONTROL_MP3,S_TASK_MP3);
          MP3_CMD(MP3_CMD_PAUSE, 0);
          MP3_TaskReplaceInSlot(TASK_MP3_Q_STATUS,i);
          MP3State.PlayMode = 2;
          runningTaskLoop = false;
          break;
        case TASK_MP3_NEXT:
          AddLog_P2(LOG_LEVEL_DEBUG, PSTR("%s%sNext"),S_CONTROL_MP3,S_TASK_MP3);
          MP3_CMD(MP3_CMD_NEXT, 0);
          MP3_TaskReplaceInSlot(TASK_MP3_Q_TRACK,i);
          MP3State.PlayMode = 1;
          runningTaskLoop = false;
          break;
        case TASK_MP3_PREV:
          AddLog_P2(LOG_LEVEL_DEBUG, PSTR("%s%sPrevious"),S_CONTROL_MP3,S_TASK_MP3);
          MP3_CMD(MP3_CMD_PREV, 0);
          MP3_TaskReplaceInSlot(TASK_MP3_Q_TRACK,i);
          MP3State.PlayMode = 1;
          runningTaskLoop = false;
          break;
        case TASK_MP3_Q_TRACK:
          AddLog_P2(LOG_LEVEL_DEBUG, PSTR("%s%squery track"),S_CONTROL_MP3,S_TASK_MP3);
          MP3_CMD(MP3_CMD_Q_SD_TRACK, 0);
          MP3_TaskReplaceInSlot(TASK_MP3_FEEDBACK,i);
          runningTaskLoop = false;
          break;
        case TASK_MP3_SHUFFLE:
          AddLog_P2(LOG_LEVEL_DEBUG, PSTR("%s%sShuffle"),S_CONTROL_MP3,S_TASK_MP3);
          MP3_CMD(MP3_CMD_SHUFFLE, 0);
          MP3_TaskReplaceInSlot(TASK_MP3_Q_PBMODE,i);
          MP3State.PlayMode = 1;
          runningTaskLoop = false;
          break;
        case TASK_MP3_ADVERT:
          AddLog_P2(LOG_LEVEL_DEBUG, PSTR("%s%sAdvert"),S_CONTROL_MP3,S_TASK_MP3);    // we could instead start a another track, if no actual track is playing
          MP3_CMD(MP3_CMD_TRACK_ADV, MP3_CURRENT_PAYLOAD);
          MP3_TaskReplaceInSlot(TASK_MP3_Q_TRACK,i);
          runningTaskLoop = false;
          break;
        case TASK_MP3_TRACK:
          AddLog_P2(LOG_LEVEL_DEBUG, PSTR("%s%sset Track"),S_CONTROL_MP3,S_TASK_MP3);
          if(MP3_CURRENT_PAYLOAD > 0 && MP3_CURRENT_PAYLOAD < MP3State.tracksRootSD+1) {
            MP3_CMD(MP3_CMD_TRACK,   MP3_CURRENT_PAYLOAD);
          }
          else {
            AddLog_P2(LOG_LEVEL_DEBUG, PSTR("%srequested track number out of bounds"),S_CONTROL_MP3);
          }
          MP3_TaskReplaceInSlot(TASK_MP3_Q_TRACK,i);
          MP3State.PlayMode = 1;
          runningTaskLoop = false;
          MP3_CURRENT_TASK_DELAY=5;
          break;
        case TASK_MP3_VOLUP:
          AddLog_P2(LOG_LEVEL_DEBUG, PSTR("%s%sVolume up"),S_CONTROL_MP3,S_TASK_MP3);
          MP3_CMD(MP3_CMD_VOLUP, 0);
          MP3_TaskReplaceInSlot(TASK_MP3_Q_VOLUME,i);
          runningTaskLoop = false;
          break;
        case TASK_MP3_VOLDOWN:
          AddLog_P2(LOG_LEVEL_DEBUG, PSTR("%s%sVolume down"),S_CONTROL_MP3,S_TASK_MP3);
          MP3_CMD(MP3_CMD_VOLDOWN, 0);
          MP3_TaskReplaceInSlot(TASK_MP3_Q_VOLUME,i);
          runningTaskLoop = false;
          break;
        case TASK_MP3_VOLUME:
          AddLog_P2(LOG_LEVEL_DEBUG, PSTR("%s%sset Volume"),S_CONTROL_MP3,S_TASK_MP3);
          MP3_CMD(MP3_CMD_VOLUME, MP3_CURRENT_PAYLOAD*30/100);
          MP3State.Volume = MP3_CURRENT_PAYLOAD;
          MP3_TaskReplaceInSlot(TASK_MP3_DONE,i);
          runningTaskLoop = false;
          break;
        case TASK_MP3_EQ:
          AddLog_P2(LOG_LEVEL_DEBUG, PSTR("%s%sset EQ"),S_CONTROL_MP3,S_TASK_MP3);
          MP3_CMD(MP3_CMD_EQ, MP3_CURRENT_PAYLOAD);
          MP3_TaskReplaceInSlot(TASK_MP3_Q_EQ,i);
          runningTaskLoop = false;
          break;
        case TASK_MP3_DEVICE:
          AddLog_P2(LOG_LEVEL_DEBUG, PSTR("%s%sset Device"),S_CONTROL_MP3,S_TASK_MP3);
          if(MP3_CURRENT_PAYLOAD == 1 || MP3_CURRENT_PAYLOAD == 2) {
            MP3_CMD(MP3_CMD_DEVICE,   MP3_CURRENT_PAYLOAD);
            AddLog_P2(LOG_LEVEL_DEBUG, PSTR("%Set device storage to: %u"),S_CONTROL_MP3, MP3_CURRENT_PAYLOAD);
          } 
          MP3_CMD(MP3_CMD_R_STORAGE, 0);              // TODO: does not work, maybe incompatible hardware
          MP3_TaskReplaceInSlot(TASK_MP3_FEEDBACK,i);
          break;
        case TASK_MP3_DAC:
          AddLog_P2(LOG_LEVEL_DEBUG, PSTR("%s%sDAC"),S_CONTROL_MP3,S_TASK_MP3);
          MP3_CMD(MP3_CMD_DAC, MP3_CURRENT_PAYLOAD);
          MP3_CMD(MP3_CMD_Q_STAT,   0);
          MP3_TaskReplaceInSlot(TASK_MP3_FEEDBACK,i);
          runningTaskLoop = false;
          break;
        case TASK_MP3_REPEAT_F:
          AddLog_P2(LOG_LEVEL_DEBUG, PSTR("%s%sRepeat play of folder %u"),S_CONTROL_MP3,S_TASK_MP3, MP3State.activeFolder);
          MP3_CMD(MP3_CMD_REPEAT_F, MP3State.activeFolder);
          MP3_TaskReplaceInSlot(TASK_MP3_Q_TRACK,i);
          runningTaskLoop = false;
          break;
        case TASK_MP3_Q_STATUS:        
          AddLog_P2(LOG_LEVEL_DEBUG, PSTR("%s%sread status"),S_CONTROL_MP3,S_TASK_MP3);
          MP3_CMD(MP3_CMD_Q_STAT, 0);
          MP3_TASK_LIST[i][0] = TASK_MP3_FEEDBACK;    // now request for feedback in the same slot
          runningTaskLoop = false;                    // do not change MP3_LAST_COMMAND
          break;
        case TASK_MP3_Q_VERSION:       
          AddLog_P2(LOG_LEVEL_DEBUG, PSTR("%s%sDevice SW Version"),S_CONTROL_MP3,S_TASK_MP3);
          MP3_CMD(MP3_CMD_Q_VERSION, 0);
          MP3_TASK_LIST[i][0] = TASK_MP3_FEEDBACK;    // now request for feedback in the same slot
          runningTaskLoop = false;                    // do not change MP3_LAST_COMMAND
          break;
        case TASK_MP3_Q_VERSION_DATE:       
          AddLog_P2(LOG_LEVEL_DEBUG, PSTR("%s%sDevice SW Date and Time"),S_CONTROL_MP3,S_TASK_MP3);
          MP3_CMD(MP3_CMD_Q_VERSION, 1);
          MP3_TASK_LIST[i][0] = TASK_MP3_FEEDBACK;    // now request for feedback in the same slot
          runningTaskLoop = false;                    // do not change MP3_LAST_COMMAND
          break;
        case TASK_MP3_Q_NUM_SD:
          AddLog_P2(LOG_LEVEL_DEBUG, PSTR("%s%sNumber of tracks  in root of SD"),S_CONTROL_MP3,S_TASK_MP3);
          MP3_CMD(MP3_CMD_Q_SD_NUM, 0);
          MP3_TASK_LIST[i][0] = TASK_MP3_FEEDBACK;    // now request for feedback in the same slot
          runningTaskLoop = false;                    // do not change MP3_LAST_COMMAND
          break;
        case TASK_MP3_Q_FOLDER_COUNT:
          AddLog_P2(LOG_LEVEL_DEBUG, PSTR("%s%sNumber of folders in root of SD"),S_CONTROL_MP3,S_TASK_MP3);
          MP3_CMD(MP3_CMD_Q_FOLDERS, 0);
          MP3_TaskReplaceInSlot(TASK_MP3_FEEDBACK,i);
          runningTaskLoop = false;
          break;
        case TASK_MP3_Q_TRACKS_IN_F:
          AddLog_P2(LOG_LEVEL_DEBUG, PSTR("%s%sNumber of tracks in active folder of SD"),S_CONTROL_MP3,S_TASK_MP3);
          MP3_CMD(MP3_CMD_Q_TRACK_F, MP3State.activeFolder);
          MP3_TaskReplaceInSlot(TASK_MP3_FEEDBACK,i);
          runningTaskLoop = false;
          break;
        case TASK_MP3_Q_EQ:
          AddLog_P2(LOG_LEVEL_DEBUG, PSTR("%s%sread EQ setting"),S_CONTROL_MP3,S_TASK_MP3);
          MP3_CMD(MP3_CMD_Q_EQ, 0);
          MP3_TASK_LIST[i][0] = TASK_MP3_FEEDBACK;    // now request for feedback in the same slot
          runningTaskLoop = false;                    
          break;
        case TASK_MP3_Q_PBMODE:
          AddLog_P2(LOG_LEVEL_DEBUG, PSTR("%s%sread Playback Mode"),S_CONTROL_MP3,S_TASK_MP3);
          MP3_CMD(MP3_CMD_Q_PBMODE, 0);
          MP3_TASK_LIST[i][0] = TASK_MP3_FEEDBACK;    // now request for feedback in the same slot
          runningTaskLoop = false;                    
          break;
        case TASK_MP3_Q_VOLUME:
          AddLog_P2(LOG_LEVEL_DEBUG, PSTR("%s%sread Volume"),S_CONTROL_MP3,S_TASK_MP3);
          MP3_CMD(MP3_CMD_Q_VOLUME, 0);
          MP3_TASK_LIST[i][0] = TASK_MP3_FEEDBACK;    // now request for feedback in the same slot
          runningTaskLoop = false;                    
          break;
        case TASK_MP3_COMMAND:
          AddLog_P2(LOG_LEVEL_DEBUG, PSTR("%s%sexecute directly injected cmd code"),S_CONTROL_MP3,S_TASK_MP3);
          MP3_CMD(MP3_CURRENT_PAYLOAD, MP3_DEBUG_PAYLOAD);
          MP3_TaskReplaceInSlot(TASK_MP3_FEEDBACK,i);
          runningTaskLoop = false;
          break;
        case TASK_MP3_PBMODE:
          AddLog_P2(LOG_LEVEL_DEBUG, PSTR("%s%sPlayback mode"),S_CONTROL_MP3,S_TASK_MP3);
          MP3_CMD(MP3_CMD_PBMODE, MP3_CURRENT_PAYLOAD);
          MP3_TaskReplaceInSlot(TASK_MP3_Q_PBMODE,i);
          runningTaskLoop = false;
          break;
        case TASK_MP3_RESET_VOLUME:
          AddLog_P2(LOG_LEVEL_DEBUG, PSTR("%s%sReset Volume"),S_CONTROL_MP3,S_TASK_MP3);
          MP3_CMD(MP3_CMD_VOLUME, MP3_VOLUME);
          MP3_TaskReplaceInSlot(TASK_MP3_Q_VOLUME,i);
          runningTaskLoop = false;
          break;
        case TASK_MP3_RESET_DEVICE:
          AddLog_P2(LOG_LEVEL_DEBUG, PSTR("%s%sReset Device"),S_CONTROL_MP3,S_TASK_MP3);
          MP3_CMD(MP3_CMD_RESET, MP3_CMD_RESET_VALUE);
          MP3_CURRENT_TASK_DELAY = MP3_TASK_LIST[i+1][1];      // set task delay
          MP3_TASK_LIST[i][0] = TASK_MP3_DONE;                 // no feedback for reset
          runningTaskLoop = false;                             // return to main loop
          break;
        case TASK_MP3_SEND_AJAX:
          AddLog_P2(LOG_LEVEL_DEBUG, PSTR("%s%sSend AJAX-JSON"),S_CONTROL_MP3,S_TASK_MP3);
          MP3SendAJAXJSON();
          MP3_CURRENT_TASK_DELAY = MP3_TASK_LIST[i+1][1];      // set task delay
          MP3_TASK_LIST[i][0] = TASK_MP3_DONE;                 // no feedback for reset
          runningTaskLoop = false;                             // return to main loop
        case TASK_MP3_DONE:                                    // this entry was already handled
          // AddLog_P2(LOG_LEVEL_DEBUG, PSTR("%sFound done MP3_TASK"),S_CONTROL_MP3);
          if(MP3_TASK_LIST[i+1][0] == TASK_MP3_NOTASK) {             // check the next entry and if there is none
            // AddLog_P2(LOG_LEVEL_DEBUG, PSTR("%s%sno Tasks left"),S_CONTROL_MP3,S_TASK_MP3);
            // AddLog_P2(LOG_LEVEL_DEBUG, PSTR("%sMP3_TASK_DONE current slot %u"),S_CONTROL_MP3, i);
            for (uint8_t j = 0; j < MP3_MAX_TASK_NUMBER+1; j++) {   // do a clean-up:
              // AddLog_P2(LOG_LEVEL_DEBUG, PSTR("%sMP3_TASK cleanup slot %u"),S_CONTROL_MP3, j);
              MP3_TASK_LIST[j][0] = TASK_MP3_NOTASK;                // reset all task entries
              MP3_TASK_LIST[j][1] = 0;                              // reset all delays
              MP3_GUI_NEEDS_UPDATE = true;                          // update web gui
            }
            runningTaskLoop = false;                                  // return to main loop
            // AddLog_P2(LOG_LEVEL_DEBUG, PSTR("%sUpdate GUI via AJAX"),S_CONTROL_MP3);
            // MP3_GUI_NEEDS_UPDATE = true;
            break; 
          }
      }
      i++;
    }
  }
  else {
    MP3_CURRENT_TASK_DELAY--;               // count down every 100 ms
  }
}

/*********************************************************************************************\
 * check the MP3 commands
\*********************************************************************************************/

bool MP3PlayerCmd(void) {
  char command[CMDSZ];
  bool serviced = true;
  uint8_t disp_len = strlen(D_CMND_MP3);

  if (!strncasecmp_P(XdrvMailbox.topic, PSTR(D_CMND_MP3), disp_len)) {  // prefix
    int command_code = GetCommandCode(command, sizeof(command), XdrvMailbox.topic + disp_len, kMP3_Commands);

    switch (command_code) {
      case CMND_MP3_TRACK:
      case CMND_MP3_VOLUME:
      case CMND_MP3_EQ:
      case CMND_MP3_DEVICE:
      case CMND_MP3_DAC:
      case CMND_MP3_PBMODE:
      case CMND_MP3_ADVERT:
      case CMND_MP3_COMMAND:
      case CMND_MP3_PAYLOAD:
        // play a track, set volume, select EQ, sepcify file device
        if (XdrvMailbox.data_len > 0) {
          MP3_CURRENT_PAYLOAD = XdrvMailbox.payload;
          if (command_code == CMND_MP3_TRACK)     MP3_Launchtask(TASK_MP3_TRACK,0,0);                                     
          if (command_code == CMND_MP3_VOLUME) {  MP3_CURRENT_PAYLOAD = XdrvMailbox.payload ;
                                                  MP3_Launchtask(TASK_MP3_VOLUME,0,0); }
          if (command_code == CMND_MP3_EQ)        MP3_Launchtask(TASK_MP3_EQ,0,0); 
          if (command_code == CMND_MP3_DEVICE)    MP3_Launchtask(TASK_MP3_DEVICE,0,0);
          if (command_code == CMND_MP3_DAC)       MP3_Launchtask(TASK_MP3_DAC,0,0);
#ifndef USE_DY_SV17F
          if (command_code == CMND_MP3_PBMODE)    MP3_Launchtask(TASK_MP3_PBMODE,0,0);
          if (command_code == CMND_MP3_ADVERT)    MP3_Launchtask(TASK_MP3_ADVERT,0,0);
          if (command_code == CMND_MP3_COMMAND)   MP3_Launchtask(TASK_MP3_COMMAND,0,0);
          if (command_code == CMND_MP3_PAYLOAD) { MP3_DEBUG_PAYLOAD = XdrvMailbox.payload;
                                                  AddLog_P2(LOG_LEVEL_DEBUG, PSTR("%sDebug-Payload set to 0x%x"),S_CONTROL_MP3, MP3_DEBUG_PAYLOAD);
                                                }
#endif // USE_DY_SV17F
        }
        Response_P(S_JSON_MP3_COMMAND_NVALUE, command, XdrvMailbox.payload);
        break;
#ifndef USE_DY_SV17F
      case CMND_MP3_PLAY:
#endif // USE_DY_SV17F
      case CMND_MP3_PAUSE:
      case CMND_MP3_STOP:
      case CMND_MP3_RESET:
#ifndef USE_DY_SV17F
      case CMND_MP3_STATUS:
      case CMND_MP3_NEXT:
      case CMND_MP3_PREV:
      case CMND_MP3_VOLUP:
      case CMND_MP3_VOLDOWN:
      case CMND_MP3_SHUFFLE:
#endif // USE_DY_SV17F
      MP3_CURRENT_PAYLOAD = 0;
        // play or re-play after pause, pause, stop,
        if (command_code == CMND_MP3_PLAY)     {MP3_Launchtask(TASK_MP3_PLAY,0,0);
                                                MP3_Launchtask(TASK_MP3_Q_TRACK,1,1);
                                                MP3_Launchtask(TASK_MP3_Q_PBMODE,2,1);
                                               }
        if (command_code == CMND_MP3_PAUSE)     MP3_Launchtask(TASK_MP3_PAUSE,0,0);
        if (command_code == CMND_MP3_STOP)      MP3_Launchtask(TASK_MP3_STOP,0,0);
        if (command_code == CMND_MP3_RESET)    {MP3_Reset();}                              // call reset
#ifndef USE_DY_SV17F
        if (command_code == CMND_MP3_STATUS)   {MP3_Launchtask(TASK_MP3_Q_STATUS ,0,0);
                                                MP3_Launchtask(TASK_MP3_Q_VOLUME,1,1);
                                                MP3_Launchtask(TASK_MP3_Q_PBMODE,2,1);
                                                MP3_Launchtask(TASK_MP3_Q_TRACK,3,1);
                                                MP3_Launchtask(TASK_MP3_Q_VERSION,4,1);
                                                MP3_Launchtask(TASK_MP3_Q_EQ,5,1);
                                                MP3_GUI_NEEDS_UPDATE = true;
                                               }
        if (command_code == CMND_MP3_NEXT)     {MP3_Launchtask(TASK_MP3_NEXT,0,0);
                                                MP3_Launchtask(TASK_MP3_Q_PBMODE,1,1);
                                                MP3_Launchtask(TASK_MP3_Q_STATUS ,2,1);
                                               }
        if (command_code == CMND_MP3_PREV)     {MP3_Launchtask(TASK_MP3_PREV,0,0);
                                                MP3_Launchtask(TASK_MP3_Q_PBMODE,1,1);
                                                MP3_Launchtask(TASK_MP3_Q_STATUS ,2,1);
                                               }
        if (command_code == CMND_MP3_VOLUP)     MP3_Launchtask(TASK_MP3_VOLUP,0,0);
        if (command_code == CMND_MP3_VOLDOWN)   MP3_Launchtask(TASK_MP3_VOLDOWN,0,0);
        if (command_code == CMND_MP3_SHUFFLE)  {MP3_Launchtask(TASK_MP3_SHUFFLE,0,0);
                                                MP3_Launchtask(TASK_MP3_Q_TRACK,1,1);
                                               }
#endif // USE_DY_SV17F        
        Response_P(S_JSON_MP3_COMMAND, command, XdrvMailbox.payload);
        break;
#ifdef USE_DY_SV17F
      case CMND_MP3_PLAY:
        if (XdrvMailbox.data_len > 0) {
          uint8_t scmd[64];
          scmd[0] = 0xAA;
          scmd[1] = 0x08;
          scmd[2] = XdrvMailbox.data_len + 1;
          scmd[3] = 2;
          char *cp = XdrvMailbox.data;
          scmd[4] = *cp;
          for (uint8_t i = 1; i < XdrvMailbox.data_len; i++) {
            if (cp[i]=='.') {
              scmd[i + 4] = '*';
            } else {
              scmd[i + 4] = toupper(cp[i]);
            }
          }
          MP3_SendCmd(scmd, XdrvMailbox.data_len + 4);
          Response_P(S_JSON_COMMAND_SVALUE, command, XdrvMailbox.data);
        } else {
          MP3_CMD(MP3_CMD_PLAY, 0);
          Response_P(S_JSON_MP3_COMMAND, command, XdrvMailbox.payload);
        }
        break;
#endif // USE_DY_SV17F
      default:
        // else for Unknown command
        serviced = false;
      break;
    }
  } else {
    return false;
  }
  return serviced;
}

/*********************************************************************************************\
 * Interface
\*********************************************************************************************/

bool Xdrv14(uint8_t function)
{
  bool result = false;

  if ((PinUsed(GPIO_RXD)) && (PinUsed(GPIO_MP3_DFR562))) { // new pix RX
    switch (function) {
      case FUNC_PRE_INIT:
        MP3PlayerInit();                              // init and start communication
        break;
      case FUNC_COMMAND:
        result = MP3PlayerCmd();                      // return result from mp3 player command
        break;
      case FUNC_EVERY_100_MSECOND:
        if (MP3_TASK_LIST[0][0] == TASK_MP3_NOTASK) {       // no task running 
          MP3PlayerHandleFeedback();                        // -> sniff for device feedback (i.e. track completion)
          break;
        }
        else {
          MP3_TaskEvery100ms();                             // something has to be done, we'll check in the next step
          break;
        }
      case FUNC_WEB_ADD_MAIN_BUTTON:
        WSContentSend_P(HTTP_BTN_MENU_MAIN_MP3);
        break;
      case FUNC_WEB_ADD_HANDLER:
        WebServer->on("/mp3", MP3HandleWebGui);
        break;
    }
  }
  return result;
}

#endif  // USE_MP3_PLAYER
