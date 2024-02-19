%2\tools\srec_cat.exe -o .\Objects\info_sbl.hex -I %2\soc\arm_cm\le501x\bin\bram.bin -Bin -of 0x18000300 -crc32-l-e -max-a %2\soc\arm_cm\le501x\bin\bram.bin -Bin -of 0x18000300 %2\tools\le501x\info.bin -Bin -of 0x18000000 -gen 0x1800001c 0x18000020 -const-l-e 0x18000300 4 -gen 0x18000020 0x18000024 -const-l-e -l %2\soc\arm_cm\le501x\bin\bram.bin -Bin 4 -gen 0x18000024 0x18000028 -const-l-e %3 4 -gen 0x1800002c 0x18000030 -const-l-e 0x1807c000 4 -gen 0x18000030 0x18000036 -rep-d 0xff 0xff 0xff 0xff 0xff 0xff 
fromelf --bin --output=.\Objects\%1.ota.bin .\Objects\%1.axf
fromelf --i32 --output=.\Objects\%1.hex .\Objects\%1.axf
if "%4" NEQ "" (
%2\tools\srec_cat.exe -Output .\Objects\%1_production_temp.hex -Intel .\Objects\info_sbl.hex -Intel .\Objects\%1.hex -Intel %2\%4 -Intel
) else (
%2\tools\srec_cat.exe -Output .\Objects\%1_production_temp.hex -Intel .\Objects\info_sbl.hex -Intel .\Objects\%1.hex -Intel
)
fromelf -c -a -d -e -v -o .\Objects\%1.asm .\Objects\%1.axf
%2\tools\srec_cat.exe -Output .\Objects\%1_production.hex -I .\Objects\%1_production_temp.hex -I %2\tools\le501x\SMART_S9.ota(data1).bin -Bin -of 0x18057F00 -gen 0x1807BE00 0x1807BE04 -const-l-e 1 4 -gen 0x1807BE04 0x1807BE08 -const-l-e 1 4 -gen 0x1807BE08 0x1807BE0c -const-l-e 0x18034000 4 -gen 0x1807BE0c 0x1807BE10 -const-l-e 0xFFFFFFFF 4 -gen 0x1807BE10 0x1807BE14 -const-l-e 0xFFFFFFFF 4 -gen 0x1807BE14 0x1807BE18 -const-l-e 0xFFFFFFFF 4 -gen 0x1807BE18 0x1807BE1c -const-l-e 0x00000000 4 -gen 0x1807BE1c 0x1807BE20 -const-l-e 0xFFFFFFFF 4 -gen 0x18057E00 0x18057E01 -const-l-e 0x00 1 
