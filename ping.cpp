/*  VisPinger v1.01 by VDG
 *  This project designed for ESP8266 chip. Use it to control up to 256 LED strip on base of WS2811 chip.
 *  Copyright (c) by Denis Vidjakin, 
 *  
 *  https://github.com/Den-W/
 *  https://mysku.ru/blog/aliexpress/52107.html
 *  https://www.instructables.com/id/VisPinger-Colorfy-Your-LAN-State
 *  
 *  PING related stuff
 */
#include "main.h"

#include "IPAddress.h"
#include <functional>

extern "C" 
{
  #include <lwip/ip.h>
  #include <lwip/sys.h>
  #include <lwip/raw.h>
  #include <lwip/icmp.h>
  #include <lwip/inet_chksum.h>
}

#define PING_DATA_SIZE  64

static struct raw_pcb *pcbraw = 0;

u8_t ping_recv( void *arg, raw_pcb *pcb, pbuf *pb, ip_addr *addr )
{ struct ip_hdr *ip = (struct ip_hdr*)pb->payload;
  if( pbuf_header( pb, -PBUF_IP_HLEN ) == 0 )
  { struct icmp_echo_hdr *iecho = (struct icmp_echo_hdr*)pb->payload;
    if( iecho->type == ICMP_ER ) 
    { gD.PingAsw( iecho->id, htons(iecho->seqno), ip->_ttl, ((byte*)iecho) + sizeof(struct icmp_echo_hdr) );
      pbuf_free( pb );
      return 1; /* eat the packet */
    }
  }
  pbuf_header( pb, PBUF_IP_HLEN );
  return 0; /* don't eat the packet */
}

//-------------------------------------------------------------------------------

struct raw_pcb *pcbGet( void )
{ if( !pcbraw ) 
  { pcbraw = raw_new(IP_PROTO_ICMP );
    raw_recv( pcbraw, ping_recv, 0 );
    raw_bind( pcbraw, IP_ADDR_ANY );
  }
  return pcbraw;
}

//-------------------------------------------------------------------------------

int   PingSend( byte *IpAddrB4, short Id, short Seq, TSecAddr *pStruct ) 
{ uint32_t  long  tm;
  int       i, Rc = 0,
            Sz = sizeof(struct icmp_echo_hdr) + PING_DATA_SIZE;

  struct pbuf *pb = pbuf_alloc( PBUF_IP, Sz, PBUF_RAM );
  if( !pb ) 
    return 0;

  if( (pb->len == pb->tot_len) && (pb->next == NULL) ) 
  { struct icmp_echo_hdr *iecho = (struct icmp_echo_hdr *)pb->payload;

    ICMPH_TYPE_SET( iecho, ICMP_ECHO );
    ICMPH_CODE_SET( iecho, 0 );
    iecho->chksum = 0;
    iecho->id = Id; // 0000-ffff
    iecho->seqno = htons( Seq ); // 0000-7fff
    
    /* fill the additional data buffer with some data */
    for(i = 0; i < PING_DATA_SIZE; i++)
      ((char*)iecho)[sizeof(struct icmp_echo_hdr) + i] = (char)i;

    tm = millis();
    memcpy( ((char*)iecho) + sizeof(struct icmp_echo_hdr), &pStruct, sizeof(void*) );
    memcpy( ((char*)iecho) + sizeof(struct icmp_echo_hdr)+sizeof(void*), &tm, sizeof(tm) );

//Serial.print( "#" ); Serial.print( iecho->id ); Serial.print( "-" ); Serial.print( Seq ); Serial.print( IpAddrB4[0] ); Serial.print( IpAddrB4[1] ); Serial.print( IpAddrB4[2] ); Serial.println( IpAddrB4[3] );

    iecho->chksum = inet_chksum(iecho, Sz );

    ip_addr_t addr;
    memcpy( &addr, IpAddrB4, 4 );    
    raw_sendto( pcbGet(), pb, &addr );
    Rc = 1;
  }
  pbuf_free(pb);
  return Rc;
}

//-------------------------------------------------------------------------------

void    CGlobalData::PingAsw( short Id, short SeqNo, short Ttl, const byte *Pkt  )
{   

  for( TSecIp *cp = mSectIps; cp; cp=cp->mNext )
  {   for( TSecAddr *tsa=cp->mAddr; tsa; tsa=tsa->mNext )
      { if( tsa->mType != 8 ) continue; // Not a TSecAddr*
        if( tsa->mPhase != 1 ) continue; // Not expecting answer
        if( tsa->mSeqNo != SeqNo ) continue; // Not expected pkt
//        if( memcmp( Pkt, tsa, sizeof(tsa) ) ) continue; // Asw Not for this block
        tsa->mTime = millis() - tsa->mTime;        
        tsa->mPhase = 2;
        return;  
      }
  }
}

//-------------------------------------------------------------------------------

void    CGlobalData::PingCheck( void )
{ TSecIp    *cp; 
  TSecAddr  *tsa;
  TSecLevel *plvl;
  int       i, n, bFin = 1;
  byte      *pb, *pt;
  uint16_t  us;
  uint32_t  ms = millis();

  if( !mPingCur ) mPingCur = mSectIps;  
  if( !mPingCur ) return;

  cp = mPingCur;
  mPingCur = mPingCur->mNext;

  switch( cp->mPhase )
  { case 0: // New cycle
      cp->mPhase++;   // Sending pings
      cp->mTime = ms;
      
    case 1: // Wait for idle
      if( ms - cp->mTime < cp->mInterval ) return; // Wait for interval before ping
      
      cp->mPhase++;
      cp->mTime = ms;      
      for( tsa=cp->mAddr; tsa; tsa=tsa->mNext ) // Clear data for new ping
      { if( tsa->mType != 8 ) continue; // Not a TSecAddr*
        tsa->mVal = 0;   // Reset fail counter
        tsa->mPhase = 0;
      }

    case 2: // Sending pings    
      bFin = 1;
      for( tsa=cp->mAddr; tsa; tsa=tsa->mNext )
      { if( tsa->mType != 8 ) continue; // Not a TSecAddr*
        switch( tsa->mPhase )
        { case 0: // Send new ping            
            bFin = 0;
            tsa->mPhase = 1; // Wait for data
            tsa->mTime = millis();
            if( ++mPingSeqNo & 0x8000 ) mPingSeqNo = 1;
            tsa->mSeqNo = mPingSeqNo;
            i = PingSend( tsa->mAddr, (long)tsa, tsa->mSeqNo, tsa );
            if( !i ) 
            {   tsa->mPhase = 4; // Failed to send
                continue;
            }
            
          case 1: // Waiting for PingAsw
            bFin = 0;
            if( ms - tsa->mTime < cp->mTTL ) continue; // Wait for PingAsw
            if( ++tsa->mVal >= cp->mTTLRetry )   // No answer
              tsa->mPhase = 3; // No answer received and all retries are spent
            else 
              tsa->mPhase = 0; // Retry to send ping
            continue;
        }     
      }      
  }

  if( !bFin ) return; // Some address is not answered yet

  // All mAddr processed. Display
  unsigned long tMin = ~0, tMax = 0, tAvg = 0;

  i = 0;
  bFin = 0;
  cp->mPhase = 0; // Display and new cycle
  cp->mCurEff = 0;
  cp->mCurPin = 0;
      
  for( tsa=cp->mAddr; tsa; tsa=tsa->mNext )
  { if( tsa->mType != 8 ) continue; // Not a TSecAddr*
//Serial.print( tsa->mAddr[0], HEX ); Serial.print( tsa->mAddr[1], HEX ); Serial.print( tsa->mAddr[2], HEX ); Serial.print( tsa->mAddr[3], HEX );
//Serial.print( " " ); Serial.print( tsa->mPhase ); Serial.print( " " ); Serial.println( tsa->mTime ); 
    if( tsa->mPhase >= 3 ) 
    { bFin++;
      continue;
    }
    tAvg += tsa->mTime;
    if( tMin > tsa->mTime ) tMin = tsa->mTime;
    if( tMax < tsa->mTime ) tMax = tsa->mTime;
    i++;
  }

  if( i ) tAvg /= i;
  cp->mVal = bFin ? 0x01:0x00; // Fail pings exists

  // pb-> <TSecLevel(4)><PinCntr(1)><Pins(PinCntr)>
  pb = ((byte*)cp) + sizeof(TSecIp);
  for( i=0; i<cp->mLstSz; i++ ) // Cycle by [IP]->LVLs
  { memcpy( &plvl, pb, sizeof(void*) );
    cp->mCurPin = pb + sizeof(void*);
    pb += sizeof(void*) + pb[sizeof(void*)]+1; // Skip PIN list
  
    // pt-> <Time(2)><TSecEffect*(4)>
    pt = ((byte*)plvl) + sizeof(TSecLevel);
    for( n=0; n<plvl->mLstSz; n++, pt+=6 ) // Cycle by [LVL*]->Time
    { memcpy( &us, pt, sizeof(us) );
      memcpy( &cp->mCurEff, pt+sizeof(us), sizeof(void*) );
      switch( plvl->mVal )
      { default: // Min
          if( tMin < us ) break;
          continue;
        case 1: // Max
          if( tMax < us ) break;
          continue;           
        case 2: // Avg
          if( tAvg < us ) break;
          continue;           
      }
      break;
    }
  }

  if( !bFin ) return; // No fail addr

  // some address(es) failed
  cp->mCurEff = cp->mFailEff;
  cp->mCurPin = cp->mFailPins;    
}

//-------------------------------------------------------------------------------

