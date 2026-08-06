The code here is released under the terms of the GNU GPL license.

Copyright is retained by Dr. Derrick Greenspan and the University of Central 
Florida (UCF). (C) 2020-2026

============================================================================

Caveat Utilitor:  

	The code provided herin has the capability to IRREVOCABLY DESTROY 
	DATA. This code is provided to you in the hope that it will be 
	useful, but WITHOUT ANY WARRANTY; not even even the implied warranty
	of MERCHANTABILITY or FITNESS FOR A  PARTICULAR PURPOSE. 
	
	I.e., no one is liable if the code causes you to lose data, or if
	it causes you to enter into a different dimension, or anything else.

============================================================================
To understand how PMOs operate, please read the following papers:
[Improving the Security and Programmability of Persistent Memory Objects (SEED 2022)](https://ieeexplore.ieee.org/document/9935011/)
[LOaPP: Improving the Performance of Persistent Memory Objects via Low-Overhead at-Rest PMO Protection (SEED 2024)](https://ieeexplore.ieee.org/document/10738362)
[Persistent Memory Objects on the Cheap (ICS 2025)](https://dl.acm.org/doi/10.1145/3721145.3734533)

If you use this work in your research, please cite our ICS 2025 paper:
```bibtex
@inproceedings{greenspan2025Persistent,
  title={Persistent Memory Objects on the Cheap},
  author={Greenspan, Derrick and Mustafa, Naveed Ul and Choi, Jongouk and Heinrich, Mark and Solihin, Yan},
  booktitle={ACM International Conference on Supercomputing 2025},
  year={2025},
  organization={ACM}
}
```


============================================================================

Build guide:

To build the library:

	cd pmo_lib
	make all 

To build the kernel:

	To build, you'll have to copy the .config file (or use make 
	meunuconfig). Your distribution most likely has a config file you 
	can use, for example, in Arch Linux, you can find your running 
	config living in /proc/config.gz.

	There is a helper script, linux-5.15.167-PMO/compile.sh that
	you may use to help build the kernel; as its last step, it will 
	reboot the computer. 

	Make sure to specify to the bootloader the new kernel; if you're 
	using GRUB, you can create or edit an entry in /boot/loader/entries.

	If your devdax device is not in /dev/dax0.0, you'll need to edit 
	pmo/pmo.h, and change the macro DAX_NAME to whatever the name of 
	your dax device in /dev/ is. In the future, this may be a kernel 
	flag, or even better, the kernel may be able to detect the name of 
	the dax device.

To configure your NVMM for PMOs:

	Make sure you have configured the NVMM via:
		ndctl create-namespace --mode=devdax

	cd pmo_mkpmo
	sudo ./mkpmo /dev/dax0.0 "Example"

Don't forget to include the pmo.h header you build in pmo_lib for your code!

============================================================================

