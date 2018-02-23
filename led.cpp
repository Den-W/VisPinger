/*  VisPinger v1.01 by VDG
 *  This project designed for ESP8266 chip. Use it to control up to 256 LED strip on base of WS2811 chip.
 *  Copyright (c) by Denis Vidjakin, 
 *  
 *  https://github.com/Den-W/
 *  https://mysku.ru/blog/aliexpress/52107.html
 *  https://www.instructables.com/id/VisPinger-Colorfy-Your-LAN-State
 *  
 *  LED control procedures
 */
#include "main.h"

//-------------------------------------------------------------------------------

void    CGlobalData::LedSetPxl( int PosLed, uByteLong clr ) // Set pixel color and select next LED  00RRGGBB
{ mLeds.SetPixelColor( PosLed, RgbColor( clr.uB[mLedR], clr.uB[mLedG], clr.uB[mLedB] ) );
}

//-------------------------------------------------------------------------------

void    CGlobalData::LedEffects()
{ int       i;
  byte      *pb;
  uByteLong c;
  uint32_t  ul = millis();  

  if( ul - mTmEffect < 20 ) return; // Slowdown
  mTmEffect = ul;

  for( TSecEffect *eff=mEffects; eff; eff=eff->mNext )
  { if( eff->mType != 3 ) continue; // Not a TSecEffect*

    switch( eff->mPhase )
    { case 0: // Start display
        if( eff->mVal >= eff->mLstSz ) eff->mVal = 0;
        if( !eff->mLstSz ) continue;

        pb = (byte*)eff;
        pb += sizeof(TSecEffect);
        pb += eff->mVal * 14; // <RgbStart(3)><RgbEnd(3)><TransitMs(4)><DelayMs(4)>
        eff->mTmStart = ul;
        eff->mClrStart[0] = eff->mClrCur[0] = pb[0]; //R
        eff->mClrStart[1] = eff->mClrCur[1] = pb[1]; //G
        eff->mClrStart[2] = eff->mClrCur[2] = pb[2]; //B
        eff->mClrEnd[0] = pb[3]; //R
        eff->mClrEnd[1] = pb[4]; //G
        eff->mClrEnd[2] = pb[5]; //B
        memcpy( &eff->mTmTransit, pb+6, 4 );
        memcpy( &eff->mTmDelay, pb+10, 4 );        
        eff->mVal++;
        eff->mPhase = 1;
        continue;
        
      case 1: // Transit
        ul = millis() - eff->mTmStart;
        if( ul >= eff->mTmTransit )
        { eff->mPhase = 2;
          eff->mTmStart = millis();
          eff->mClrCur[0] = eff->mClrEnd[0];
          eff->mClrCur[1] = eff->mClrEnd[1];
          eff->mClrCur[2] = eff->mClrEnd[2];      
          break;
        }
        
        eff->mClrCur[0] = map( ul, 0, eff->mTmTransit, eff->mClrStart[0], eff->mClrEnd[0] );
        eff->mClrCur[1] = map( ul, 0, eff->mTmTransit, eff->mClrStart[1], eff->mClrEnd[1] );
        eff->mClrCur[2] = map( ul, 0, eff->mTmTransit, eff->mClrStart[2], eff->mClrEnd[2] );
        break;
      
      case 2: // Final Delay
        if( millis() - eff->mTmStart > eff->mTmDelay ) 
            eff->mPhase = 0;
        break;
    }
  }

  for( TSecIp *v = mSectIps; v; v = v->mNext )
  { if( !v->mCurEff ) continue;
  
    if( v->mCurEff->mSignal ) // Need to send signal
    { if( (v->mVal&0x10) == 0 ) // Signal is not sended yet
      { v->mVal |= 0x10;
        if( !v->mCurEff->mSignal->mPhase ) v->mCurEff->mSignal->mPhase = 1; // Send signal
      }
    }    

    if( !v->mCurPin ) continue;

    c.uB[0] = v->mCurEff->mClrCur[0];
    c.uB[1] = v->mCurEff->mClrCur[1];
    c.uB[2] = v->mCurEff->mClrCur[2];
    
    for( i=v->mCurPin[0]; i > 0; i-- )
      LedSetPxl( v->mCurPin[i], c );
  }

  if( !mEffPins || !mEffPins[0] ) return;

  if( !mEff ) mEff = CfgFindEffect( mEffName );
  if( !mEff ) 
  { strncpy( mFailName, mEffName, sizeof(mFailName) );
    return;
  }

  c.uB[0] = mEff->mClrCur[0];
  c.uB[1] = mEff->mClrCur[1];
  c.uB[2] = mEff->mClrCur[2];
    
  for( i=mEffPins[0]; i > 0; i-- )
      LedSetPxl( mEffPins[i], c );
}

//-------------------------------------------------------------------------------

void    CGlobalData::Signals()
{ uint32_t ul = millis();
  
  for( TSecSignal *sg=mSignals; sg; sg=sg->mNext )
  { if( sg->mType != 2 ) continue; // Not a TSecSignal*

    switch( sg->mPhase )
    { default: // Waiting for signal;
        break;        
        
      case 1: // Send signal
        sg->mPhase = 2;
        sg->mStart = ul;
        if( sg->mVal ) digitalWrite( sg->mVal, sg->mLstSz ? HIGH:LOW );
    
        while( sg->mFilePlay ) 
        { if( mTmMute && millis()-mTmMute < 30*1000 ) break;
          mTmMute = 0;
          mp3_play( sg->mFilePlay ); // File to play              
          break;
        }
        break;

      case 2: // Send signal
        ul -= sg->mStart;
        if( ul < sg->mLength ) break;
        sg->mPhase = 3;
        sg->mStart = ul;
        if( sg->mVal ) digitalWrite( sg->mVal, sg->mLstSz ? LOW:HIGH );
    
      case 3: // Guard interval
        ul -= sg->mStart;
        if( ul < sg->mPause ) break;
        sg->mPhase = 0;
    }
  }
}

//-------------------------------------------------------------------------------

