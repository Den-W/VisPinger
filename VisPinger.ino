/*  VisualPinger v1.01 by VDG
 *  This project designed for ESP8266 chip. 
 *  Its purpoise - to help sysadmins monitor network state with one glance.
 *  Copyright (c) by Denis Vidjakin, 
 *  Courtseu for help and idea to Mikhail
 *  
 *  https://github.com/Den-W/VisPinger
 *  https://mysku.ru/blog/aliexpress/52107.html
 *  https://www.instructables.com/id/VisPinger-Colorfy-Your-LAN-State
 
 *  01.04.2017 v1.01 created by VDG
 *  
 *  Main module
 */
 
#include "main.h"
 
CGlobalData gD;

//-----------------------------------------------------------------------------

// Button handlers
void hBtSingle()  { gD.mKey = 0x01; }
void hBtDouble()  { gD.mKey = 0x02; }
void hBtLongSt()  { gD.mToLp = millis(); }
void hBtLongEn()  { gD.mKey = 0x03; }

void setup(void) 
{ gD.Start();
}

void loop(void) 
{ gD.Run();
}

//-----------------------------------------------------------------------------
 
void  CGlobalData::Start(void) 
{ 
  FlashRd();  // Read data from eeprom
  Serial.begin( 9600 );     // USB port  
  mp3_set_serial( Serial );  //set Serial for DFPlayer-mini mp3 module 
  
  mBt.setClickTicks( 300 );
  mBt.attachClick( hBtSingle );
  mBt.attachDoubleClick( hBtDouble );
  mBt.attachLongPressStart( hBtLongSt );  
  mBt.attachLongPressStart( hBtLongEn );  
  
  pinMode( PIN_LED, OUTPUT );  // Set OnBoad LED as an output
  mBlinkMode = 0x03;
  BlinkerSet( 100, 1 );
  
  { bool res = SPIFFS.begin();    // Try to load FS
    Serial.print("\nVisPinger v1.01. SPIFFS:" ); Serial.print( res ? "Ok":"Fail" );    

    if( res )
    { for( int i=millis()+2000; i > millis(); ) { mBt.tick(); Blinker(); }
      if( gD.mToLp ) res = false; // current button signal.    
    }

    if( res == false )  // First start probably. Try to format.
    { Serial.print("->formatting...");
      BlinkerSet( 500, 1 );
      res = SPIFFS.format();
      Serial.print( res ? "Ok":"Fail" );
    }
  }

  mIrda.enableIRIn(); // Start the IR receiver
  mLeds.Begin();
  CfgLoad();    
  WebInit();
  mp3_set_volume( 20 );       // 0-30
}

//-----------------------------------------------------------------------------

void  CGlobalData::Run(void) 
{ mKey = 0;
  mBt.tick();

  if( !mbShowedErr && mFailName[0] ) // Error in config exists. Blink first 3 led RED!
  { uByteLong c;
    for( int i=0; i < 10; i++ )
    { c.uL = 0;
      if( i & 0x01 ) c.uB[0] = 0xFF;
      LedSetPxl( 0, c );
      LedSetPxl( 1, c );
      LedSetPxl( 2, c );
      mLeds.Show();
      delay( 200 );
    }
    c.uL = 0;      
    LedSetPxl( 0, c );
    LedSetPxl( 1, c );
    LedSetPxl( 2, c );
    mbShowedErr = 1;
  }

  if ( mIrda.decode(&mIrdaRes) ) 
  { mIrCommand = mIrdaRes.value & 0xFFFF;
    mIrda.resume(); // Receive the next value

    for( TSecIRDA *v = mSectIRDA; v; v = v->mNext ) 
    { if( v->mCode != mIrCommand ) continue;
      if( v->mSig ) continue;
      if( v->mSig == (TSecSignal*)1 )
      { // MUTE
        mp3_stop();
        mTmMute = millis();
        break;
      }

      if( v->mSig == (TSecSignal*)2 )
      { // RESET
        delay( 500 );
        ESP.restart();
        break;
      }

      if( !v->mSig->mPhase ) v->mSig->mPhase = 1; // Send signal
      break;
    }
  }

  switch( mKey )
  { case 0x01: // Button press - Down
      break;
    case 0x02: // Button Double press - Up
      break;
    case 0x03: // Button Long press
      { int l = millis() - mToLp;
        if( l > 10*1000 ) break; // Too long
        if( l < 4*1000 ) break; // Too short
        BlinkerSet( 100, 1 );
        Defaults();
        FlashWr();
        break;    
      }
  }

  PingCheck();
  LedEffects();
  Signals();
  mLeds.Show();
  mSrv.handleClient();
  if( !mWF_Mode ) mDns.processNextRequest(); 
  Blinker();
}

//-----------------------------------------------------------------------------

void  CGlobalData::BlinkerSet( int Ms, int bOn )
{ 
  mBlinkMs = millis() + Ms;
  mBlinkLed = bOn;
  digitalWrite( PIN_LED, mBlinkLed ? LOW:HIGH );  
}

void  CGlobalData::Blinker(void) 
{ static byte  BlinkOn[4] =  {0, 9, 20, 9};
  static byte  BlinkOff[4] = {0, 6, 50, 1};

  if( millis() < mBlinkMs ) return;
  
  if( mBlinkLed ) 
  { BlinkerSet( BlinkOff[mBlinkMode&0x03]*100L, 0 );
  } else BlinkerSet( BlinkOn[mBlinkMode&0x03], 1 );
}

//-----------------------------------------------------------------------------

byte crc8( byte crc, byte ch ) 
{  
    for (uint8_t i = 8; i; i--) {
      uint8_t mix = crc ^ ch;
      crc >>= 1;
      if (mix & 0x01 ) crc ^= 0x8C;
      ch >>= 1;
  }
  return crc;
}

//-----------------------------------------------------------------------------

void CGlobalData::FlashRd(void)
{ int   i;
  
  EEPROM.begin(&_Wr-&_St+8);
  
  _Wr = 0;
  for( i=0; i < &_Wr - &_St; i++ )
  { (&_St)[i] = EEPROM.read( i );
    _Wr = crc8( _Wr, (&_St)[i] );
     yield();
  }
  
  i = EEPROM.read( i ) & 0xFF;
  if( _Wr != i ) Defaults(); // CRC failed. Reset data.
  _Wr = 0;
  EEPROM.end();

  switch( mLedOrder & 0x07 )
  { default:  mLedR = 1; mLedG = 0; mLedB = 2; break;
    case 2:   mLedR = 0; mLedG = 2; mLedB = 1; break;
    case 3:   mLedR = 0; mLedG = 1; mLedB = 2; break;    
    case 4:   mLedR = 1; mLedG = 2; mLedB = 0; break;
    case 5:   mLedR = 2; mLedG = 1; mLedB = 0; break;
  }  
}

//-----------------------------------------------------------------------------

void CGlobalData::FlashWr(void)
{ int   i;
  EEPROM.begin(&_Wr-&_St+8);
  _Wr = 0;
  for( i=0; i < &_Wr - &_St; i++ )
  { EEPROM.write( i, (&_St)[i] );
    _Wr = crc8( _Wr, (&_St)[i] );
    yield();
  }
  EEPROM.write( i, _Wr );
  EEPROM.commit();
  EEPROM.end();
  _Wr = 0;
}

//-----------------------------------------------------------------------------

