# Message Protocol

## Initial draft of protocol

Maskable bitstring format, see individual bit designations below split onto multiple lines (`<>` represents one byte):

```
 command (optional)          crc  stop
        ||                   ||    ||
  <>    <>    <> <> <> <>    <>    <>
  ||               |
address    parameter value (optional)
```

## Split out Command

Separated nibbles below

```
1b get/set (1=set, 0=get) 3b parameter
              |            |||
              ••••        •••• 
```

### Parameters

Listing format just gives 3 bits to specify parameter type

`•••` (set|get) - specific parameter

0. `000` (set)      - address
1. `001` (get)      - temperature
2. `010` (get|set)  - current
3. `011` (get|set)  - velocity
4. `100` (get|set)  - position
5. `101` (get|set)  - maximum current
6. `110` (get|set)  - `e` stop behavior
7. `111` (get)      - status register


## Special Commands

### Heartbeat

Requires no response, broadcasted to all connected devices with `11111111` address (address reserved for heartbeat)

### Command responses

Every command from the brain (except heartbeats) should be responded to
with a status packet (addr, status register byte, crc, stopbyte)
