# efibootwin
efibootwin is a Windows-Console program for Read/Set UEFI Variable (BootNext, BootOrder,..) a little bit the linux efibootmgr

for help : efibootwin -? the output is
efibootwin create by J. Funk, Ver 0.8.5

The syntax of efibootwin [command command]:
 The commands can be begin with '-' or '/'
 The commands are:

   ?            Help
   n            Get the BootNext Value
   N  idx       Set the BootNext Value with idx (hex)
   e  Name      Set the BootNext Value over the Name
   E            Remove the BootNext Value
   c            Get the BootCurrent Value
   o            Get the BootOrder
   O  x,y,zzzz  Set the BootOrder (hex)
   r            Get the BootOrder (Name)
   R            Remove the BootOrder
   v            Get the DriverOrder
   V  x,y,zzzz  Set the DriverOrder (hex)
   a            Get the DriverOrder (Name)
   A            Remove the DriverOrder
   t            Get the Timeout Value
   T  idx       Set the Timeout Value with idx (hex)
   I            Remove the Timeout Value
   b            List the BootXXXX
   B            List all the BootXXXX (have wait...)
   d            List the DriverXXXX
   D            List all the DriverXXXX (have wait...)
   f  idx       Toggle the Active-Flag of Boot idx (hex)
   F  Name      Toggle the Active-Flag with Boot decription
   d  idx  Des  Change the description of Boot idx (hex)
   D  Name Des  Change the description (Des) with Boot decription (Name)

Without commands you get all the available Uefi-variables

I using this UEFI-Spec https://uefi.org/sites/default/files/resources/UEFI_Spec_2_8_final.pdf for develop

