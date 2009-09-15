#!/usr/bin/env python
## ###############
## @author kidd
##
## Separate out Xerces constants
## so that they can be easily
## found by other apps.
import platform

Xerces = {
    'Darwin' : ('xerces-c-3.0.1-x86-macosx-gcc-4.0',
                'xerces-c-3.0.1-x86-macosx-gcc-4.0'),
    'Linux' : ('xerces-c-3.0.1-x86-linux-gcc-3.4',
               'xerces-c-3.0.1-x86_64-linux-gcc-3.4'),
    'Windows' : ('xerces-c-3.0.1-x86-windows-vc-9.0',
                'xerces-c-3.0.1-x86_64-windows-vc-9.0')
    }


(bits,linkage)  = platform.architecture()
sys             = platform.system()
IsZip           = sys == 'Windows'
Is64            = bits == '32bit'
Name = Xerces[sys][Is64]



