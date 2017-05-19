/*  VisPinger v1.01 by VDG
 *  This project designed for ESP8266 chip. Use it to control up to 256  strip on base of WS2811 chip.
 *  Copyright (c) by Denis Vidjakin, 
 *  
 *  https://github.com/Den-W/VisPinger
 *  http://mysku.ru/blog/aliexpress/
 *  https://www.instructables.com/id/VisPinger-Colorfy-Your-LAN-State
 *  
 *  WEB related stuff and pages data
 */
#include "main.h"

const char P_Set[] PROGMEM = R"(
[rc:0k]
)";

const char P_Main[] PROGMEM = R"(
<!DOCTYPE html PUBLIC "-//W3C//DTD XHTML 1.0 Transitional//EN">
<head><title>Web Lights</title><meta http-equiv="Content-Type" content="text/html; charset=win1251">
<meta name=viewport content="width=device-width, initial-scale=1.1">
<style>
h4{text-align: center;background: orange;margin: -2px 0 1em 0;}
body {font-size:14px;}
label {float:left; padding-right:10px;}
.fld{clear:both; text-align:right; line-height:25px;}
.frm{float:left;border: 2px solid #634f36;background: #f3f0e9;padding: 5px;margin:3px;}
</style>
<script type="text/javascript">
 window.addEventListener("load",function(e)
 { document.getElementById("btWC").addEventListener("click",btc_WC);   
   document.getElementById("btU").addEventListener("click",btc_U);   
   document.getElementById("btD").addEventListener("click",btc_D);   
 })
 function btc_WC(){ TxRq("c","fWC" ); }
 function btc_U(){ TxRq("e","fU" ); }
 function btc_D(){ TxRq("e","fD" ); }
 
 function TxRq(url,fName,id)
 { var xhr = new XMLHttpRequest();
   xhr.open('POST',url,false);
   var fData = new FormData(document.forms.namedItem(fName));
   xhr.send(fData);
   if (xhr.status != 200) alert( "TxRq error - " + xhr.status + ': ' + xhr.statusText );
   else { if (id) document.getElementById(id).innerHTML = xhr.responseText; }
 }
</script></head><body>
<div class="frm"><h4>VisPinger info</h4>
<table width="100%" cellspacing="0" cellpadding="4">
<tr><td align="left" width="60">Address:</td><td>"@IP@"</td></tr>
<tr><td align="left">IR Code:</td><td>"@IRC@"</td></tr>
<tr><td align="left">INI File:</td><td>"@IFL@"</td></tr>
<tr><td align="left">Fail name:</td><td>"@FNM@"</td></tr>
"@BMP@"</table><hr/><h4>VisPinger config</h4>
<form name="fWC" method="POST">
 <div class="fld"><label for="cM">WiFi Mode</label><select name="cM">"@MA@"</select></div>
 <div class="fld"><label for="cN">Name</label><input type="text" name="cN" maxlength="15" value="@NAME@"></div>
 <div class="fld"><label for="cP">Password</label><input type="text" name="cP" maxlength="15" value="@PASS@"></div>
 <div class="fld"><label for="cL">LED order</label><select name="cL">"@ML@"</select></div><br/>
 <div style="float:right;"><button id="btWC">Set params</button></div>
</form> 
<br/><hr/><a href="pe">Edit</a>&emsp;<a href="pf">Files</a>"@PNB@"
</div>
</body></html>
)";

//-------------------------------------------------------------------------------------------

const char P_Edit[] PROGMEM = R"(
<!DOCTYPE html PUBLIC "-//W3C//DTD XHTML 1.0 Transitional//EN">
<head><title>Web Lights</title><meta http-equiv="Content-Type" content="text/html; charset=win1251">
<meta name=viewport content="width=device-width, initial-scale=1.0">
<style>
h4{text-align: center;background: orange;margin: -2px 0 1em 0;}
body {font-size:14px;}
label {float:left; padding-right:10px;}
.fld{clear:both; text-align:right; line-height:25px;}
.frm{float:left;border: 2px solid #634f36;background: #f3f0e9;padding: 5px;margin:3px;}
</style>
<script type="text/javascript">
 window.addEventListener("load",function(e)
 { document.getElementById("btSC").addEventListener("click",btc_SC);
 })
 function btc_SC(){ TxRq("s","fSC" ); }
 function TxRq(url,fName,id)
 { var xhr = new XMLHttpRequest();
   xhr.open('POST',url,false);
   var fData = new FormData(document.forms.namedItem(fName));   
   xhr.send(fData);
   if (xhr.status != 200) alert( "TxRq error - " + xhr.status + ': ' + xhr.statusText );
   else { if (id) document.getElementById(id).innerHTML = xhr.responseText; }
 }
</script></head><body>
<div class="frm" style="clear:both;"><h4>VisPinger Editor</h4>
<form name="fSC" method="POST">
<textarea style="width:99%" name="Edt" cols="60" rows="20">"@EDT@"</textarea><br/><br/>
<div style="float:left;"><a href="/">Config</a>&emsp;<a href="pf">Files</a></div>
<div style="float:right;">"@ENM@"&emsp;<button id="btSC">Save</button></div>
</form>
</div>
</body></html>
)";

//-------------------------------------------------------------------------------------------

const char P_File[] PROGMEM = R"(
<!DOCTYPE html PUBLIC "-//W3C//DTD XHTML 1.0 Transitional//EN">
<head><title>Web Lights</title><meta http-equiv="Content-Type" content="text/html; charset=win1251">
<meta name=viewport content="width=device-width, initial-scale=1.0">
<style>
h4{text-align: center;background: orange;margin: -2px 0 1em 0;}
body {font-size:14px;}
label {float:left; padding-right:10px;}
.fld{clear:both; text-align:right; line-height:25px;}
.frm{float:left;border: 2px solid #634f36;background: #f3f0e9;padding: 5px;margin:3px;}
</style>
<script type="text/javascript">
 window.addEventListener("load",function(e)
 { document.getElementById("btFL").addEventListener("click",btc_FL);
 })
 function btc_FL(){ TxRq("f","fFL" ); }
 
 function TxRq(url,fName,id)
 { var xhr = new XMLHttpRequest();
   xhr.open('POST',url,false);
   var fData = new FormData(document.forms.namedItem(fName));   
   xhr.send(fData);
   if (xhr.status != 200) alert( "TxRq error - " + xhr.status + ': ' + xhr.statusText );
   else { if (id) document.getElementById(id).innerHTML = xhr.responseText; }
 }
</script></head><body>
<div class="frm"><h4>VisPinger Files "@FR@"</h4>
<form name="fFL" method="POST"><table>"@FILE@"
<tr><td colspan=2><hr />
 <div class="fld"><label for="fN">New filename</label><input type="text" name="fN" maxlength="32" ></div><br/>
 <div class="fld"><label for="fB">Upload file</label><input type="file" name="fB" formenctype="multipart/form-data"></div><br/>
 <div class="fld">
  <input type="radio" name="fO" value="S">Select<input type="radio" name="fO" value="E">Edit<input type="radio" name="fO" value="R">Rename
  <input type="radio" name="fO" value="D">Delete<input type="radio" name="fO" value="U">Upload
 </div></td></tr>
<tr><td><a href="/">Config</a>&emsp;<a href="pe">Edit</a></td>
    <td align="right"><button id="btFL">Exec</button>
</td></tr></table>
</form></div>
</body></html>
)";

//-----------------------------------------------------------------------------

void  ShowArgs( const char *Msg )
{   Serial.print("\nUrl:" ); Serial.print( gD.mSrv.uri() ); Serial.print("  " ); Serial.print( Msg );
    for ( int n = 0; n < gD.mSrv.args(); n++ ) 
     { Serial.print("\n"); Serial.print( n ); Serial.print(":"); 
       Serial.print(gD.mSrv.argName(n)); Serial.print("="); Serial.print(gD.mSrv.arg(n)); 
     }
}

//-----------------------------------------------------------------------------

void  CGlobalData::Pgm2Str( String &sPg, PGM_P content )
{   int     n;
    char    contentUnit[400], Tb[64], *p, *e;
    PGM_P   contentNext;

    BlinkerSet( 500, 1 );
    while( true )
    {   // due to the memccpy signature, lots of casts are needed
        memset( contentUnit, 0, sizeof(contentUnit) );
        contentNext = (PGM_P)memccpy_P((void*)contentUnit, (PGM_VOID_P)content, 0, sizeof(contentUnit)-1 );
        
        content += sizeof(contentUnit)-1;

        while( 1 )
        { p = strstr( contentUnit, "\"@" );
          if( p ) *p = 0;          
          sPg += contentUnit;
          if( !p ) break;
          p += 2;
          e = strstr( p, "@\"" );
          if( !e ) break;
          n = e - p;
          if( n > sizeof(Tb)-1 ) n = sizeof(Tb)-1;
          memcpy( Tb, p, n ); Tb[n] = 0;
          strcpy( contentUnit, e+2 );

          if( !strcmp( Tb, "NAME" ) ) // WiFi network SSID
          {   sPg += mWF_Id;
              continue;
          }
          if( !strcmp( Tb, "PASS" ) ) // WiFi password
          {   sPg += mWF_Pwd; 
              continue;
          }
          if( !strcmp( Tb, "IP" ) ) // Current TCP address
          {   sPg += WiFi.localIP().toString();
              continue;
          }
          if( !strcmp( Tb, "IRC" ) )  // Last IR command
          {   sprintf( Tb, "%04X", mIrCommand );
              sPg += Tb;
              continue;
          }
          if( !strcmp( Tb, "IFL" ) )  // Current ini file
          {   sPg += mFlName;
              continue;
          }
          if( !strcmp( Tb, "FNM" ) )  // Fail name
          {   sPg += mFailName;
              continue;
          }
          if( !memcmp( Tb, "MA", 2 ) )
          { const char *Nm[] = { "Access Point","Client",0 };
            for( n=0; Nm[n]; n++ )
            { sprintf( Tb, "<option value=\"%d\" %s>%s</option>", n, n==gD.mWF_Mode ? "selected":"", Nm[n] );
              sPg += Tb;
            }
            continue;
          }
          if( !memcmp( Tb, "ML", 2 ) )
          { const char *Nm[] = { "GRB","GBR","RGB","RBG","BRG","BGR",0};
            for( n=0; Nm[n]; n++ )
            { sprintf( Tb, "<option value=\"%d\" %s>%s</option>", n, n==gD.mLedOrder ? "selected":"", Nm[n] );
              sPg += Tb;
            }
              continue;
          }
          if( !strcmp( Tb, "ENM" ) )  // Edit name
          {   sprintf( Tb, "[%s]", gD.mEdtName );
              sPg += Tb;
              continue;
          }
          if( !strcmp( Tb, "EDT" ) && gD.mEdtName[0] )
          {  File  f = SPIFFS.open( gD.mEdtName, "r");
             if( f )
             { sPg += f.readString();
               f.close();
             }
             continue;
          }
          if( !strcmp( Tb, "FR" ) )  // Fileinfo data
          {   FSInfo fs_info;
              SPIFFS.info(fs_info);
              sprintf( Tb, "(free %d)", fs_info.totalBytes - fs_info.usedBytes );
              sPg += Tb;
              continue;
          }
          if( !strcmp( Tb, "FILE" ) )  // File list
          { n = 0;
            Dir dir = SPIFFS.openDir("/");
            while( dir.next() )
            { if( dir.fileName().length() < 2 ) continue;
              File f = dir.openFile("r");             
              if( n++ == 0 ) sPg += "<tr>";
              sPg += "<td><input type=\"radio\" name=\"fS\" value=\"" + dir.fileName().substring(1) + "\">";
              sPg += dir.fileName().substring(1) + " / " + f.size() + "</td>";
              if( n >= 2 ) { sPg += "</tr>"; n = 0; }
            }
            continue;
          }
        }
        if( contentNext ) break;
    }
}

//-------------------------------------------------------------------------------

void CGlobalData::WebTxPage( int b200, PGM_P content )
{   String  sPg;
    Pgm2Str( sPg, content );
    mSrv.send( b200 ? 200:404, "text/html", sPg );
}

//-------------------------------------------------------------------------------

void handle_root() 
{ //ShowArgs( "- Root" ); // Debug WEB output
  gD.WebTxPage( true, P_Main );
}

void handle_edt() 
{ // ShowArgs( "- scr" );  // Debug WEB output
  gD.WebTxPage( true, P_Edit );
}

void handle_files() 
{ // ShowArgs( "- files" );  // Debug WEB output
  gD.WebTxPage( true, P_File );
}

//-------------------------------------------------------------------------------

int   strcpymax( char *To, const String &From, int Max )
{ int i = From.length();
  if( !i ) return 0;
  if( i >= Max ) i = Max-1;
  memcpy( To, From.c_str(), i );  
  To[i] = 0;
  return i;
}

void handle_cf() 
{ gD.mWF_Mode = atoi( gD.mSrv.arg("cM").c_str() );
  gD.mLedOrder = atoi( gD.mSrv.arg("cL").c_str() );    
  strcpymax( gD.mWF_Id,   gD.mSrv.arg("cN"), sizeof(gD.mWF_Id) );
  strcpymax( gD.mWF_Pwd,  gD.mSrv.arg("cP"), sizeof(gD.mWF_Pwd) );

  gD.FlashWr();
  // ShowArgs( "-WebCfg" );  // Debug WEB output
  gD.WebTxPage( true, P_Set );
  delay( 2000 );
  ESP.restart();
}

//-------------------------------------------------------------------------------

void handle_sc() 
{ String s = gD.mSrv.arg("Edt"); 
  if( s.length() > 0 && gD.mEdtName[0] ) 
  {   SPIFFS.remove( gD.mEdtName );
      File  f = SPIFFS.open( gD.mEdtName, "w");
      if( f ) 
      { f.write( (byte*)s.c_str(), s.length() );
        f.close();
      }
  }
  // ShowArgs( "- Script" );  // Debug WEB output
  gD.WebTxPage( true, P_Set );  
}

//-------------------------------------------------------------------------------

void handle_frx() 
{ HTTPUpload& upload = gD.mSrv.upload();  
  switch( upload.status )
  { case UPLOAD_FILE_START:
      { String  s = "/" + gD.mSrv.arg("fS");
        if( s.length() < 2 ) s = "/" + gD.mSrv.arg("fN");
        if( s.length() < 2 ) s = "/" + upload.filename;
        gD.mFl.close();
        SPIFFS.remove( s );
        gD.mFl = SPIFFS.open( s, "w" );
        gD.mFlSize = 0;
        break;
      }
    case UPLOAD_FILE_WRITE:
        if( !gD.mFl ) break;
        gD.mFlSize += upload.currentSize;
        gD.mFl.write( upload.buf, upload.currentSize );
        break;
    case UPLOAD_FILE_END:
        gD.mFl.close();
        break;    
  }
  yield();
}
    
void handle_fl() 
{  bool   bWr = false;
   File   f;
   String s = gD.mSrv.arg("fO") + " ",
          sSel = "/" + gD.mSrv.arg("fS"),
          sNm = "/" + gD.mSrv.arg("fN");

  switch( s[0] )
  { case 'S': // Select file
      if( sSel.length() < 2 ) break;
      strcpy( gD.mFlName, sSel.c_str()+1 );
      bWr = true;
      break;        
    case 'E': // Edit file
      if( sSel.length() < 2 ) break;
      strcpy( gD.mEdtName, sSel.c_str() );
      break;
    case 'D': // Delete file
      if( sSel.length() < 2 ) break;
      SPIFFS.remove( sSel );
      break;
    case 'R': // Rename file      
      if( sNm.length() < 2 ) break;
      if( sSel.length() < 2 ) break;
      SPIFFS.remove( sNm );
      SPIFFS.rename( sSel, sNm );
      break;
  }
  // ShowArgs( "-File" );  // Debug WEB output
  gD.WebTxPage( true, P_Set );  

  if( !bWr ) return;
  
  gD.FlashWr();
  yield();
  delay( 1000 );
  ESP.restart();
}

//-------------------------------------------------------------------------------

void  CGlobalData::WebInit( void )
{ char    Tb[128];

  mBlinkMode = 0x01;
  BlinkerSet( 500, 1 );
  wifi_station_set_hostname("vispinger.com");

  sprintf( Tb, "\nSSID:%s, Pwd:%s, ", mWF_Id, mWF_Pwd );
  Serial.print( Tb );  
  
  WiFi.softAPdisconnect();
  WiFi.disconnect();
  WiFi.mode(WIFI_STA);

  delay(100);
  // Connect to WiFi network
  if( !mWF_Mode )
  { // Create access point
    IPAddress ip(192, 168, 1, 1);
    IPAddress gateway(192, 168, 1, 1);  // set gateway to match your wifi network
    IPAddress subnet(255, 255, 255, 0); // set subnet mask to match your wifi network
    WiFi.config(ip, gateway, subnet);
    Serial.print( "Mode:AccessPoint 192.168.1.1/255.255.255.0, IP:" );
    WiFi.softAP( mWF_Id, mWF_Pwd );
    mIP = WiFi.softAPIP();
    mDns.setTTL(300);
    mDns.start( 53, "*", mIP);
  } else 
  { Serial.print( "Mode:Client, IP:" );    
    WiFi.begin( mWF_Id, mWF_Pwd);
        
    // Wait for connection   
    while (WiFi.status() != WL_CONNECTED) 
    { Blinker();
      delay( 5 );
    }
    mIP = WiFi.localIP();
  }

  Serial.print( mIP );

   mbOnLine = true;
   mBlinkMode = 0x02;
   mSrv.onNotFound(handle_root); //send main page to any request
   mSrv.on("/c", handle_cf); // set config   
   mSrv.on("/f", HTTP_POST, handle_fl, handle_frx ); //handle file uploads
   mSrv.on("/pf", handle_files);  // page-file
   mSrv.on("/pe", handle_edt);    // page-scr   
   mSrv.begin();
}

//-------------------------------------------------------------------------------

