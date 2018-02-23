 /*  This project designed for ESP8266 chip. Use it to control up to 256 LED strip on base of WS2811 chip.
  *  Copyright (c) by Denis Vidjakin, 
  *  
  *  https://github.com/Den-W/
  *  https://mysku.ru/blog/aliexpress/52107.html
  *  https://www.instructables.com/id/VisPinger-Colorfy-Your-LAN-State
  *  
  *  Load and parce ini file(s)
  */
#include "main.h"

//                              0    1                  2                  3                 4             5                 6    7
static const char *secTp[] = {"**","MA",              "SG",              "EF",             "LV",         "IP",             "IR","IN"};
static const byte  secSz[] = {   0,   0,sizeof(TSecSignal),sizeof(TSecEffect),sizeof(TSecLevel),sizeof(TSecIp),sizeof(TSecIRDA),  0 };

//-------------------------------------------------------------------------------

class CCfgRd
{ public:
  
  TUSection   *mCfg;  
  byte        mBlk[1024], *mWPos;

  CCfgRd() : mCfg( (TUSection*)mBlk ), mWPos(mBlk) { memset( mBlk, 0, sizeof(mBlk) ); }
  
  void  Load( void )
    { for( int i=1; i<=6; i++ ) LoadFile( gD.mFlName, i ); // load

      // Set output pins
      for( TSecSignal *sg=gD.mSignals; sg; sg=sg->mNext )
      { if( sg->mType != 2 ) continue; // Not a TSecSignal*
        if( !sg->mVal ) continue;
        pinMode( sg->mVal, OUTPUT );   // Set PIN as an output
        digitalWrite( sg->mVal, sg->mLstSz ? LOW:HIGH );
      }
    }
    
//...-------------------------------------------------------------------------------

  int  LoadFile( const char *FlNm, int RdType )
    { int         i, n, SecType = 0;
      char        Tb[32], NmSec[13], NmPrm[13], *e;
      const char  *p;
      byte        *cw;
      String      s, s2;
      File        Fl;

      Tb[0] = '/';
      strncpy( Tb+1, FlNm, sizeof(Tb)-2 );
      Tb[ sizeof(Tb)-1 ] = 0;

      Fl = SPIFFS.open( Tb, "r");  
      if( Fl == 0 ) 
      {   strncpy( gD.mFailName, Tb, sizeof(gD.mFailName) );
          return -1; // No file
      }

      while( 1 )
      { if( Fl.position() == Fl.size() ) break; // EOF
                  
        s = Fl.readStringUntil('\n');
        if( s.length() <= 0 ) continue; // Empty Str

        i = s.indexOf( "\r" );
        if( i >= 0 ) s.remove( i );

        // Clip comment
        i = s.indexOf( ";" );
        if( i >= 0 ) s.remove( i );

        s.trim();   
        if( s.length() <= 0 ) continue; // Empty Str

        if( s[0] == '[' ) 
        { // New section. Get name.
          SaveParam();

          i = s.indexOf("]") - 1;
          if( i <= 0 ) continue; // Invalid section
          if( i >= sizeof(NmSec) ) i = sizeof(NmSec)-1;
          memcpy( NmSec, s.c_str()+1, i );                  
          NmSec[i] = 0;

          for( i=1; i<sizeof(secTp)/sizeof(secTp[0]); i++ )
            if( !memcmp( NmSec, secTp[i], 2 ) ) break;

          if( i >= sizeof(secTp)/sizeof(secTp[0]) )
          { i = 0; // Invalid section name          
            continue;            
          }

          SecType = i;
          mWPos = mBlk + secSz[i];

          switch( i )
          { case 2: // TSecSignal;
              strncpy( mCfg->mSectSignal.mName, NmSec, sizeof(mCfg->mSectSignal.mName) );
              mCfg->mSectSignal.mName[sizeof(mCfg->mSectSignal.mName)-1] = 0;
              break;

            case 3: // TSecEffect
              strncpy( mCfg->mSectEffect.mName, NmSec, sizeof(mCfg->mSectEffect.mName) );
              mCfg->mSectEffect.mName[sizeof(mCfg->mSectEffect.mName)-1] = 0;
              break;

            case 4: // TSecLevel
              strncpy( mCfg->mSectLevel.mName, NmSec, sizeof(mCfg->mSectLevel.mName) );
              mCfg->mSectLevel.mName[sizeof(mCfg->mSectLevel.mName)-1] = 0;
              break;
              
            case 5: // IP address defaults
              mCfg->mSectIp.mTTL = gD.mPingTTL;        // Ping time
              mCfg->mSectIp.mTTLRetry = gD.mPingTTLRetry;
              mCfg->mSectIp.mInterval = gD.mPingInterval;   // Section period
              break;
          }
          continue; 
        }

        // it's parameter of some section
        i = s.indexOf("=");
        if( i < 0 ) continue; // Invalid param

        for( n=i; n > 0 && strchr( " \t=", s[n] ); n-- ); // End of name        
        if( ++n >= sizeof(NmPrm) ) n = sizeof(NmPrm)-1;
        memcpy( NmPrm, s.c_str(), n );
        NmPrm[ n ] = 0;

        for( n=i; n < s.length() && strchr( " \t=", s[n] ); n++ ); // Start of value

        p = s.c_str() + n;

        if( mCfg->mSect.mType == 7 ) // Include 
        { if( !stricmp( NmPrm, "FILE" ) ) LoadFile( p, RdType ); // Process Include
          continue;     
        }

        if( RdType != SecType ) continue; // Expecting another section

        mCfg->mSect.mType = SecType;

        if( mWPos-mBlk >= sizeof(mBlk)-64 ) continue; // No space. ignore

        switch( mCfg->mSect.mType )
        { case 1: // Main
            if( !stricmp( NmPrm, "TTL" ) )           gD.mPingTTL = atoi( p );
            if( !stricmp( NmPrm, "TTLRETRY" ) )      gD.mPingTTLRetry = atoi( p );
            if( !stricmp( NmPrm, "PINGINTERVAL" ) )  gD.mPingInterval = atoi( p );
            if( !stricmp( NmPrm, "PINGINTERVAL" ) )  gD.mPingInterval = atoi( p );
            if( !stricmp( NmPrm, "EFFECT" ) )        
            { i = GetWord( Tb, sizeof(Tb), p ); 
              strncpy( gD.mEffName, Tb, sizeof(gD.mEffName) );
              for( i=0; i<sizeof(gD.mEffPins)-1; i++ )
              { if( !GetWord( Tb, sizeof(Tb), p ) ) break;
                gD.mEffPins[ ++gD.mEffPins[0] ] = atoi( Tb );
              }
            }
            continue;
    
          case 2: // Signal
            if( !stricmp( NmPrm, "FILENO" ) ) mCfg->mSectSignal.mFilePlay = atoi( p );
            if( !stricmp( NmPrm, "LEVEL" ) )  mCfg->mSectSignal.mLstSz = atoi( p );
            if( !stricmp( NmPrm, "LENGTH" ) ) mCfg->mSectSignal.mLength = atoi( p );            
            if( !stricmp( NmPrm, "PERIOD" ) ) mCfg->mSectSignal.mPause = 1000*atoi( p );
            if( !stricmp( NmPrm, "PIN" ) )    
            { // Relay may be connected to D0,D1,D2,D6,D7,D8
              mCfg->mSectSignal.mVal = 0;
              if( !stricmp( p, "D0" ) ) mCfg->mSectSignal.mVal = D0;
              if( !stricmp( p, "D1" ) ) mCfg->mSectSignal.mVal = D1;
              if( !stricmp( p, "D2" ) ) mCfg->mSectSignal.mVal = D2;
              if( !stricmp( p, "D6" ) ) mCfg->mSectSignal.mVal = D6;
              if( !stricmp( p, "D7" ) ) mCfg->mSectSignal.mVal = D7;
              if( !stricmp( p, "D8" ) ) mCfg->mSectSignal.mVal = D8;
            }            
            continue;
          
          case 3: // Effect
            if( !stricmp( NmPrm, "SEQ" ) ) 
            { // <RgbFrom(3)>,<RgbTo(3)>,<TransitTime(4)>,<Delay(4)>
              i = GetWord( Tb, sizeof(Tb), p ); // <RgbFrom(3)>
              if( i ) i = strtol( Tb, &e, 16 ); else i = 0x00FFFFFF;
              *mWPos++ = i >> 16; // R
              *mWPos++ = i >> 8;  // G
              *mWPos++ = i;       // B

              i = GetWord( Tb, sizeof(Tb), p ); // <RgbTo(3)>
              if( i ) i = strtol( Tb, &e, 16 ); else i = 0x00FFFFFF;
              *mWPos++ = i >> 16; // R
              *mWPos++ = i >> 8;  // G
              *mWPos++ = i;       // B

              i = GetWord( Tb, sizeof(Tb), p ); // <TransitTime(4)>
              if( i ) i = strtol( Tb, &e, 16 ); else i = 1000;
              memcpy( mWPos, &i, 4 ); 
              mWPos += 4;

              i = GetWord( Tb, sizeof(Tb), p ); // <Delay(4)>
              if( i ) i = atoi( Tb ); else i = 1000;
              memcpy( mWPos, &i, 4 );
              mWPos += 4;
              mCfg->mSectEffect.mLstSz++;
              continue;
            }
            if( !stricmp( NmPrm, "SIG" ) ) 
            { mCfg->mSectEffect.mSignal = gD.CfgFindSignal( p );
              if( !mCfg->mSectEffect.mSignal ) strncpy( gD.mFailName, p, sizeof(gD.mFailName) );
            }
            continue;
            
          case 4: // Level. follows <Time(2)><TSecEffect*(4)>
            if( !stricmp( NmPrm, "MODE" ) ) 
            { if( !stricmp( p, "AVG" ) ) mCfg->mSectLevel.mVal = 2;
              else if( !stricmp( p, "MAX" ) ) mCfg->mSectLevel.mVal = 1;
                   else mCfg->mSectLevel.mVal = 0;
            }
            
            if( !stricmp( NmPrm, "TIME" ) )
            { i = GetWord( Tb, sizeof(Tb), p ); // <TimeMs>
              if( i ) i = atoi( Tb ); else i = 100;
              memcpy( mWPos, &i, 2 ); 
              mWPos += 2;
              TSecEffect *ef = gD.CfgFindEffect( p );
              if( !ef ) strncpy( gD.mFailName, p, sizeof(gD.mFailName) );
              memcpy( mWPos, &ef, 4 ); 
              mWPos += 4;
              mCfg->mSectLevel.mLstSz++;
            }            
            continue;

          case 5: // IPs. follows <TSecLevel(4)><PinCntr(1)><Pins(PinCntr)>
            if( !stricmp( NmPrm, "TTL" ) )          mCfg->mSectIp.mTTL = atoi( p );
            if( !stricmp( NmPrm, "TTLRETRY" ) )     mCfg->mSectIp.mTTLRetry = atoi( p );
            if( !stricmp( NmPrm, "PINGINTERVAL" ) ) mCfg->mSectIp.mInterval = atoi( p );            
            if( !stricmp( NmPrm, "ADDR" ) )
            {  IPAddress ia;
            
               if( !GetWord( Tb, sizeof(Tb), p ) ) continue;
               if( !ia.fromString( Tb ) ) 
               { // Resolve
                 if (!WiFi.hostByName( Tb, ia ) ) 
                 {  strncpy( gD.mFailName, Tb, sizeof(gD.mFailName) );
                    continue; // Failed to resolve          
                 }
               }

               TSecAddr *va = (TSecAddr*)malloc( sizeof(TSecAddr) );
               if( !va ) continue;

               memset( va, 0, sizeof(TSecAddr) );
               va->mType = 8;
               va->mAddr[0] = ia[0]; 
               va->mAddr[1] = ia[1]; 
               va->mAddr[2] = ia[2]; 
               va->mAddr[3] = ia[3];
               
               if( !mCfg->mSectIp.mAddr ) mCfg->mSectIp.mAddr = va;
               else for( TSecAddr *vb=mCfg->mSectIp.mAddr; 1; vb = vb->mNext )
                    { if( vb->mNext ) continue;
                      vb->mNext = va;
                      break;
                    }
               continue;
            }
            if( !stricmp( NmPrm, "LVL" ) )
            { // Lvl = LV_Typical,3 -> <TSecLevel(4)><PinCntr(1)><Pins(PinCntr)>
              TSecLevel *lv = 0; 

              i = GetWord( Tb, sizeof(Tb), p );
              if( i ) 
              { for( TSecLevel *v = gD.mLevels; v; v = v->mNext ) 
                  if( !strcmpi( v->mName, Tb ) ) 
                  { lv = v;
                    break;
                  }
              }
              if( !lv ) 
              { strncpy( gD.mFailName, Tb, sizeof(gD.mFailName) );
                continue;
              }
              memcpy( mWPos, &lv, 4 ); 
              mWPos += 4;  
              cw = mWPos;  
              *mWPos++ = 0;
              while( GetWord( Tb, sizeof(Tb), p ) )
              { (*cw)++;
                *mWPos++ = atoi( Tb );
              }
              mCfg->mSectIp.mLstSz++;
              continue;              
            }
            if( !stricmp( NmPrm, "FAIL" ) )
            { // Lvl = LV_Typical,3 -> <TSecLevel(4)><PinCntr(1)><Pins(PinCntr)>
              byte  pins[64];
               
              i = GetWord( Tb, sizeof(Tb), p );
              if( i ) 
              { mCfg->mSectIp.mFailEff = gD.CfgFindEffect( Tb );
                if( !mCfg->mSectIp.mFailEff ) strncpy( gD.mFailName, Tb, sizeof(gD.mFailName) );
              }
              pins[0] = 0;              
              while( GetWord( Tb, sizeof(Tb), p ) )
                pins[ ++pins[0] ] = atoi( Tb );

              mCfg->mSectIp.mFailPins = (byte*)malloc( pins[0]+1 );
              memcpy( mCfg->mSectIp.mFailPins, pins, pins[0]+1 );
              continue;              
            }
            continue;

          case 6: // IRDA. 
            mCfg->mSectIRDA.mSig = gD.CfgFindSignal( NmPrm );
            if( !mCfg->mSectIRDA.mSig )
            { if( !stricmp( NmPrm, "MUTE" ) )  mCfg->mSectIRDA.mSig = (TSecSignal*)1;
              if( !stricmp( NmPrm, "RESET" ) ) mCfg->mSectIRDA.mSig = (TSecSignal*)2;
            }
            if( !mCfg->mSectIRDA.mSig ) strncpy( gD.mFailName, NmPrm, sizeof(gD.mFailName) );

            i = GetWord( Tb, sizeof(Tb), p );
            i = strtol( Tb, &e, 16 );
            mCfg->mSectIRDA.mCode = i;
            continue;
        }
      }
      SaveParam();
    }

//...-------------------------------------------------------------------------------

   void  SaveBlk( TSection **To )
   {  int   n = mWPos - mBlk;
      if( n <= 0 || !mCfg->mSect.mType ) return;

      TSection *m = (TSection*)malloc( n );
      
      if( !m ) return;
      memcpy( m, mBlk, n );

      if( !*To ) // First element
      { *To = m;
        return;
      }

      TSection *v = *To;
      while( v->mNext ) v = v->mNext;
      v->mNext = m;
   }
   
//...-------------------------------------------------------------------------------

   void  SaveParam( void )
   { 
      switch( mCfg->mSect.mType )
      { case 2: SaveBlk( (TSection**)&gD.mSignals ); break; // TSecSignal
        case 3: SaveBlk( (TSection**)&gD.mEffects ); break; // TSecEffect
        case 4: SaveBlk( (TSection**)&gD.mLevels );  break; // TSecLevel
        case 5: SaveBlk( (TSection**)&gD.mSectIps ); break; // TSecIp
        case 6: SaveBlk( (TSection**)&gD.mSectIRDA ); break; // TSecIRDA
      }
      mWPos = mBlk;
      mCfg->mSect.mType = 0;
      memset( mBlk, 0, sizeof(mBlk) );
   }
};

//-------------------------------------------------------------------------------

void  CGlobalData::CfgLoad( void )
{ CCfgRd  c;
  c.Load();
}
    
//-------------------------------------------------------------------------------

int   GetWord( char *To, char ToSz, const char *&PtrFrom )
{ int n = 0;

  *To = 0;
  if( !PtrFrom ) return 0;
  
  while( *PtrFrom && strchr( " \t", *PtrFrom ) ) PtrFrom++;
  while( *PtrFrom && !strchr( ", \t", *PtrFrom ) ) 
  { *To++ = *PtrFrom++;
    n++;
    if( n >= ToSz )
    { To--;
      break;    
    }
  }
  *To = 0;
  while( *PtrFrom && *PtrFrom != ',' ) PtrFrom++;  
  if( *PtrFrom == ',' ) PtrFrom++;  
  return n;
}

//-------------------------------------------------------------------------------

