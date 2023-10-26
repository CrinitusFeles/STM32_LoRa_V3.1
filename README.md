>Flash module - single bank organization

| Flash area        | Flash memory addresses                                                                                                                                                                                                     | Size (bytes)                                              | Name                                                                                                                            |
|-------------------|----------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------|-----------------------------------------------------------|---------------------------------------------------------------------------------------------------------------------------------|
| Main memory       | 0x0800 0000 - 0x0800 07FF<br>0x0800 0800 - 0x0800 0FFF<br>0x0800 1000 - 0x0800 17FF<br>0x0800 1800 - 0x0800 1FFF<br>-<br>0x0801 F800 - 0x0801 FFFF<br>-<br>0x0803 F800 - 0x0803 FFFF<br>-<br>0x0807 F800 - 0x0807 FFFF<br> | 2K<br>2K<br>2K<br>2K<br>-<br>2K<br>-<br>2K<br>-<br>2K<br> | Page 0<br>Page 1<br>Page 2<br>Page 31<br>-<br>Page 63 (128K FLASH) <br>-<br>Page 127 (256K FLASH)<br>-<br>Page 255 (512K FLASH) |
| Information block | 0x1FFF 0000 - 0x1FFF 6FFF<br>0x1FFF 7000 - 0x1FFF 73FF<br>0x1FFF 7800 - 0x1FFF 780F                                                                                                                                        | 28K<br>1K<br>16                                           | Sysyem memory<br>OTP area<br>Option bytes                                                                                       |