# demos_for_Linux_ARM

## Introduction
Demos targeting at Linux 4.9.x kernel and ARM platform to show    
a. How the exceptions and faults are triggerd and handled<br/>
b. TODO(I'll add more later)

Lists of modules<br/>
a. exp_trigger  
　　This module provides a stable and easy way to trigger exceptions supported<br/>
by ARMv8.0 AARCH64 mode(later we'll also support ARMv8.0 AARCH32 mode).<br/>
And we choose Amlogic Ampere at hand as the demo platform, which is also<br/>
carried by MiBox.

## Directory structure
.<br/>
+--　README.md　　　　　　 this file<br/>
+--　exp_trigger　　　　　　　includes all exp_trigger module related files<br/>
|　　+-- docs　　　　　　　　　　detail documentations<br/>
|　　|　 +-- CHS　　　　　　　　　　documentations in chinese<br/>
|　　|　 |   +-- exp_trigger.pdf<br/>
|　　|　 +-- ENG　　　　　　　　　　documentations in english<br/>
|　　+-- driver　　　　　　　　　 exp_trigger source codes<br/>
|　　|　 +-- src<br/>
|　　|　 +-- patches　　　　　　　　 patches needing to be applied to kernel<br/>
|　　|　 +-- README.md<br/>
|　　+-- tool　　　　　　　　　　samples to use exp_trigger<br/>
|　　|　 +-- src<br/>
|　　|　 +-- README.md<br/>

## Detail infos for each module
### exp_trigger
#### ARMv8 AARCH64 exceptions can be triggered 
<table>
	<tr>
		<td rowspan="7">Synchronous <br/>
			Exceptions</td>
		<td rowspan="6">MMU Faults</td>
		<td>Address Size Fault</td>
		<td>user space ttbr/level 2~3<br/>
			kernel space ttbr/level 2~3</td>
	</tr>
	<tr>
		<td>Translation Fault</td>
		<td>user space level 0~3<br/>
			kernel space level 0~3</td>
	</tr>
	<tr>
		<td>Access Flag Fault</td>
		<td>user space level 3<br/>
			kernel space level 2~3</td>
	</tr>
	<tr>
		<td>Alignment Fault</td>
		<td>kernel space data/pc/sp</td>
	</tr>
	<tr>
		<td>Permission Fault</td>
		<td>kernel space level 3 data accessing<br/>
		 kernel space level 2~3 intruction fetching</td>
	</tr>
	<tr>
		<td>Synchroronous external<br/>
			abort on translation<br/>
			table walk</td>
		<td>kernel space</td>
	</tr>
	<tr>
		<td >Other Faults</td>
		<td >Sychnorous External<br/>
			Abort</td>
		<td>kernel space</td>
	</tr>
	<tr>
		<td>Asynchronous<br/>
			Exceptions</td>
		<td colspan="2">SError</td>
		<td>kernel space</td>
	</tr>
</table>

#### How to run
　　The steps listed below are targeted at Amlogic Ampere platform, but I believe<br/>
for other ARMv8 platforms running Linux kernel, only a little bit modifications<br/>
are needed :smiley: <br/><br/>
step 1. Compiling kernel and dtb as exp_trigger/driver/README.md described<br/>
step 2. Compiling exp_trigger_demo as exp_trigger/tool/Readme.md described<br/><br/>
　　after step 1&&2 you'll have kernel image "Image", dtb image "gxl_p212_2g.dtb"<br/>
and a user space tool "exp_trigger_demo", then<br/><br/>
step 3. Booting to linux console via commands(in u-boot console): <br/> 
　　　mmcinfo;fatload mmc 0 1080000 Image;<br/>
　　　fatload mmc 0 20000000 rootfs.cpio.uboot_64;<br/>
　　　fatload mmc 0 30000000 gxl_p212_2g.dtb;<br/>
　　　booti 1080000 20000000 30000000;"<br/>
step 4. Execute exp_trigger_demo and selects the supported exceptions you wanna<br/>
　　　trigger  
