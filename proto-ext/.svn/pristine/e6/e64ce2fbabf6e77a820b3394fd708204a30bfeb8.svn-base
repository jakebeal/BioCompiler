/* 
Copyright (C) 2005-2008, Jonathan Bachrach, Jacob Beal, and contributors 
listed in the AUTHORS file in the MIT Proto distribution's top directory.

This file is part of MIT Proto, and is distributed under the terms of
the GNU General Public License, with a linking exception, as described
in the file LICENSE in the MIT Proto distribution's top directory. */

#define MAX_HOOD_MSGS 4
#define MAX_DEBUG_PKT 13

struct HoodMsg{
  uint8_t  messageLength;
  uint8_t  messageType;
  uint8_t  id;
  uint8_t  n;  
  uint8_t  version;
  uint8_t  timeout;
  COM_DATA buf[MAX_HOOD_MSGS];
};

struct DigestMsg {
  uint8_t  messageLength;
  uint8_t  messageType;
  uint8_t  id;
  uint8_t  version;
  uint16_t n;   
  uint8_t  digest[MAX_DIGEST_PKT];
};

struct ScriptMsg {
  uint8_t  messageLength;
  uint8_t  messageType;
  uint8_t  id;
  uint8_t  pkt_num; 
  uint16_t n; 
  uint8_t  version;
  uint8_t  script[MAX_SCRIPT_PKT];
};

struct DebugMsg {
  uint8_t  messageLength;
  uint8_t  messageType;
  uint8_t  id;
  uint8_t  debug[MAX_DEBUG_PKT];
};

typedef struct HoodMsg HOOD_MSG;
typedef struct DigestMsg DIGEST_MSG;
typedef struct ScriptMsg SCRIPT_MSG;
typedef struct DebugMsg DEBUG_MSG;
