# Communication

All communication with avocados is initiated by the master computer (heretofore referred to as the brain). The brain has a static address, each individual avocado has it's own user settable address. All messages fit in a maximum of 8 bytes to maximize bandwidth and throughput. The standard message (a few exceptions are discussed in detail later in the guide) has an address byte, a command byte, optionally four bytes specifying a parameter value in the case of sets, a crc byte and a stop byte. These components are discussed further in the protocol section. Below is a depiction of the protocol (`<>` represents one byte, `?` means optional):

```text
address   command?            crc  stop
  ||        ||                 ||   ||
  <>   <>   <>   <> <> <> <>   <>   <>
       ||             |
       id       parameter value?
```

## Multiple Avocados

Each message is prefixed with an byte address. This allows specification of addresses well past the limits of our electronics or communication bandwidth. Once an avocado's address has been set communicating with it is as simple as specifying the correct address in each message.

To set an address one must connect the brain and avocado one wishes to set with no other avocados on the bus. One can then send a set address message (see specific messages listed later in the guide for details) with the new address. This message is broadcasted to all avocados on the bus which is why it's important to only have one connected at a time while setting addresses.

## Protocol

The standard message has an address byte specifying which device the message is intended for, a command byte specifying the action to take, a crc byte for corruption checking and a stop byte indicating the end of the message. Messages generally fall into two categories of gets and sets. If the command is a set then the new value is given by 4 bytes following the command byte. These four bytes give the new float value for the parameter specified in the command byte.

The address byte simply specifies an integer address corresponding to a specific avocado. The crc byte is computed using a CRC-8 implementation included in our source code. The stop byte is currently the ASCII character `"!"`. The command byte is explained in detail in the next section.

### Split out Command

The commmand byte contains two pieces of information. The first bit specifies if the command is a `get` (a 0) or `set` (a 1). The middle four bits are currently unused. These bits can be expanded too as functionality is added. Finally, the last three bits specify which parameter to `get` or `set`. The specific bitmasks corresponding to each parameter are given in the next section. If the message is a `set` command the the four bytes following the command byte specify the float value with which to set the given parameter.

Below shows the byte separated into nibbles with their different information:

```text
1b get/set (1=set, 0=get)
         |
         ••••   ••••
                 |||
            3b parameter
```

#### Parameters

Listing format just gives 3 bits to specify parameter type

`•••` (set|get) - specific parameter

0. `000` (set)      - address
1. `001` (get)      - temperature
2. `010` (get|set)  - current
3. `011` (get|set)  - velocity
4. `100` (get|set)  - position
5. `101` (get|set)  - maximum current
6. `110` (get|set)  - E-Stop behavior
7. `111` (get)      - status register

## Command responses

Every command from the brain (aside from the exceptions given in the following section) receives a response specifiying E-Stop behavior, status of the most recent command and whether or not the avocado is being performance limited. If the E-Stop behavior bit is a 1 that means the avocado will hold position until a safety threshold is reached, while a 0 means it will kill motor power. A 1 for the performance limiting bit indicates the avocado is currently being limited while a 0 indicates it's running freely. Finally, a 1 for the status bit means the last command succeeded, a 0 means it failed.

```text
1b performance limiting (1=limited, 0=free) 1b E-Stop behavior (1=hold position, 0=kill motor)
                                          | |
                           ••••          ••••
                            ||           | |
                      4b unused  1b unused 1b status of command (1=success, 0=failure)
```

### Special Commands

#### Heartbeat

Requires no response. Broadcasted to all connected devices using address `11111111` (address reserved for heartbeat).

#### Setting Address

Requires no response. Broadcasted to all connected devices using address `11111110` (address reserved for setting avocado addresses).

