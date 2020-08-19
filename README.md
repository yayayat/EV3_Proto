# EV3_Proto
EV3 UART Protocol parser

## Input

.txt with hex chars separated with '\n'

## Example

    40
    42
    FD

parses as:

    #0 Header: 0x40 Length: 1 Payload: 0x42 CRC: (calc/read:0xFD/0xFD) Passed!
    [CMD TYPE: 66]
  
