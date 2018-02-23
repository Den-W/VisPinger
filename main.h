/*  VisPinger v1.01 by VDG
 *  This project designed for ESP8266 chip. Use it to control up to 256 LED strip on base of WS2811 chip.
 *  Copyright (c) by Denis Vidjakin, 
 *  
 *  https://github.com/Den-W/VisPinger
 *  https://mysku.ru/blog/aliexpress/52107.html
 *  https://www.instructables.com/id/VisPinger-Colorfy-Your-LAN-State
 *  
 */
 
#include <Arduino.h>
#include <stdio.h>
#include <EEPROM.h>
#include <OneButton.h>
#include <FS.h>
#include <ESP8266WiFi.h>
#include <ESP8266mDNS.h>
#include <ESP8266WebServer.h>
#include <IRremoteESP8266.h>
#include <DFPlayer_Mini_Mp3.h>
#include <DNSServer.h>

#include <NeoPixelBus.h>
#include <NeoPixelAnimator.h>

// PIN assignment. Relay may be connected to D0,D1,D2,D6,D7,D8
#define PIN_BUTTON  D3  // I GPIO0  - D3  Button
#define PIN_LED     D4  // O GPIO2  - D4  OnBoard LED
#define PIN_IRDA    D5  // I GPIO14 - D5  IRDA receiver

typedef unsigned long DWORD;

typedef union
{ long  uL;
  byte  uB[4];  // RGB?
} uByteLong;

typedef struct TSection_
{   byte      mType;      // Section type = 0:unknown.
                          // 1:Main, 2:Signal, 3:Effect, 4:Level, 5:Ip, 6:IRDA, 7:Include   
    byte      mLstSz;     // Section list counter
    byte      mPhase;     // Section process phase
    byte      mVal;
    struct TSection_  *mNext;    
} TSection;

typedef struct TSecSignal_
{   byte      mType;        // Section type = 2.
    byte      mLstSz;       // Section list counter. == Signal level 0\1
    byte      mPhase;       // Section process phase. 0:Idle, 1:Send signal, 2:Pulse, 3:GuardPause
    byte      mVal;         // PIN number    
    struct TSecSignal_  *mNext;
        
    uint32_t  mStart;       // Start millis()    
    uint32_t  mLength;      // Signal length millis()
    uint32_t  mPause;       // Signal period millis()
    char      mName[13];    // Section name
    short     mFilePlay;    // File to play
} TSecSignal;

typedef struct TSecEffect_ 
{   byte      mType;        // Section type = 3.
    byte      mLstSz;       // Section list counter
    byte      mPhase;       // Section process phase: 0:Transit, 1:Delay
    byte      mVal;         // Current Seq < mLstSz    
    struct TSecEffect_  *mNext;
    
    TSecSignal *mSignal;   // Send this signal
    uint32_t  mTmStart;    // Start millis()
    uint32_t  mTmTransit;  // RgbStart->RgbEnd time
    uint32_t  mTmDelay;    // delay in RgbEnd time
    char      mName[13];    // Section name
    byte      mClrStart[3]; // RGB - start
    byte      mClrEnd[3];   // RGB - end
    byte      mClrCur[3];   // RGB - current
} TSecEffect; // follows <RgbStart(3)><RgbEnd(3)><TransitMs(4)><DelayMs(4)>

typedef struct TSecLevel_
{   byte      mType;      // Section type = 4.
    byte      mLstSz;     // Section list counter
    byte      mPhase;     // Section process phase
    byte      mVal;       // 0:Min, 1:Max, 2:Avg
    struct TSecLevel_ *mNext;

    char      mName[13];  // Section name
} TSecLevel; // follows <Time(2)><TSecEffect*(4)>

typedef struct TSecAddr_
{   byte      mType;      // Section type = 8.
    byte      mLstSz;     // Section list counter
    byte      mPhase;     // Section process phase: 1:Send, 2:PingTime, 3:NoAnswer, 4:FailToSend
    byte      mVal;       // Fail counter
    struct TSecAddr_ *mNext;

    uint32_t  mTime;
    uint16_t  mSeqNo;
    byte      mAddr[4];
} TSecAddr;

typedef struct TSecIp_
{   byte      mType;      // Section type = 5
    byte      mLstSz;     // Section list counter    
    byte      mPhase;     // Section process phase: 0:Rst, 1:WaitForInterval, 2:SendPings, 3:ReadyToDisplay, 
    byte      mVal;       // 0x01:IsFailPings, 0x10:SignalSended
    struct TSecIp_   *mNext;
    
    TSecAddr   *mAddr;
    TSecEffect *mFailEff;   // -> fail effect
    byte       *mFailPins;  // -> <PinCntr(1)><Pins(PinCntr)>
    
    TSecEffect *mCurEff;    // Current effect
    byte       *mCurPin;    // Current pins for effect
      
    uint16_t  mTTL;        // Ping time, ms
    uint16_t  mTTLRetry;   // 
    uint16_t  mInterval;   // Section period, ms
    uint32_t  mTime;        
} TSecIp; // follows <TSecLevel(4)><PinCntr(1)><Pins(PinCntr)>

typedef struct TSecIRDA_
{   byte      mType;      // Section type = 6
    byte      mLstSz;     // Section list counter    
    byte      mPhase;     // Section process phase
    byte      mVal;       // Fail counter
    struct TSecIRDA_   *mNext;
        
    uint16_t  mCode;      // IRDA Code
    TSecSignal  *mSig;    //
} TSecIRDA; 

typedef union 
{ TSection    mSect;
  TSecSignal  mSectSignal;
  TSecEffect  mSectEffect;
  TSecLevel   mSectLevel;
  TSecAddr    mSectAddr;  
  TSecIp      mSectIp;
  TSecIRDA    mSectIRDA; 
} TUSection;


class CGlobalData
{   
  public:
    
    byte    _St;
    byte      mLedOrder;        // 0-7
    byte      mWF_Mode;         // 0: AccessPoint, 1:Client    
    char      mWF_Id[16];       // WiFi SSID
    char      mWF_Pwd[16];      // WiFi password    
    char      mFlName[32];      // Current file
    byte    _Wr;                // Data between _St and _Wr stored in flash memory. _Wr == crc8 of block

    uint16_t  mPingInterval;
    uint16_t  mPingTTL;
    uint16_t  mPingTTLRetry;
    byte      mLedR, mLedG, mLedB; // LED offsets 0,1,2

    bool      mbShowedErr;        // 
    volatile bool mbOnLine;    
    
    byte      mKey;             // Pressed key    
    long      mFlSize;
    long      mToLp;            // LongPress start    
    byte      mbFirstPass;      // true on first pass
    byte      mBlinkMode;
    byte      mBlinkLed;
    uint16_t  mIrCommand;       // Last IR command.
    uint32_t  mBlinkMs;
    uint16_t  mPingSeqNo;       // 0000-7fff

    uint32_t  mTmMute;
    uint32_t  mTmEffect;
    uint32_t  mPingDelay;
    TSecIp    *mPingCur;

    char      mEffName[16];     // Working effect name
    byte      mEffPins[16];     // Working pins
    TSecEffect *mEff;           // Working effect ptr.
    
    char      mEdtName[32];     // Name of current edit file;
    char      mFailName[32];    // Name of failed element. (no secton with this name)
    byte    _En;
        
    File              mFl;      // upload file handler
    OneButton         mBt;      // Button handlers
    IRrecv            mIrda;
    decode_results    mIrdaRes;
    ESP8266WebServer  mSrv;     // http server
    IPAddress         mIP;
    
    TSecSignal        *mSignals;
    TSecEffect        *mEffects;
    TSecLevel         *mLevels;
    TSecIp            *mSectIps;
    TSecIRDA          *mSectIRDA;
    DNSServer         mDns;
    
    NeoPixelBus<NeoGrbFeature, Neo800KbpsMethod>  mLeds;

    CGlobalData() : mSrv(80), mBt(PIN_BUTTON, true), mIrda(PIN_IRDA), mLeds(256)
    { Defaults();
    }
              
    void    Start(void);
    void    Run(void);
    void    Blinker(void);
    void    BlinkerSet( int Ms, int bOn );
    void    FlashRd(void);
    void    FlashWr(void);        
    
    void    WebInit( void );    
    void    WebTxPage( int b200, PGM_P content );
    void    Pgm2Str( String &s, PGM_P content );
    void    PingCheck( void );
    void    PingAsw( short Id, short SeqNo, short Ttl, const byte *Pkt );
    
    void    CfgLoad( void );
    void    Signals();
    void    LedSetPxl( int PosLed, uByteLong clr ); // Set pixel color and select next LED  00RRGGBB
    void    LedEffects();
    
    TSecSignal *CfgFindSignal( const char *Name )
                { for( TSecSignal *v = mSignals; v; v = v->mNext ) if( !strcmpi( v->mName, Name ) ) return v;
                  return 0;
                }

    TSecEffect *CfgFindEffect( const char *Name )
                { for( TSecEffect *v = mEffects; v; v = v->mNext ) if( !strcmpi( v->mName, Name ) ) return v;
                  return 0;
                }

    void  Defaults( void )
          { memset( &_St, 0, &_En-&_St );
            strcpy( mWF_Id,  "VisPinger" );   // default WiFi SSID
            strcpy( mWF_Pwd, "vispinger" );   // default WiFi password
            strcpy( mFlName, "main.ini" );    // Ini file to use            

            mPingTTL = 500;
            mPingTTLRetry = 3;
            mPingInterval = 15*1000;
            mbFirstPass = true;
          }
};

extern CGlobalData gD;

int   GetWord( char *To, char ToSz, const char *&PtrFrom );
int   PingSend( byte *IpAddrB4, short Id, short Seq, TSecAddr *pStruct ) ;

