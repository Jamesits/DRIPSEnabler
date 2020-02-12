# DRIPSEnabler

This UEFI program enables Intel PEP (Power Engine Plug-in) and adds support for DRIPS if your firmware didn't support it natively.

> 这个好像不是所有大学生都能写出来的玩意了——[Ben Wang](https://github.com/imbushuo)，2020-02-12

## WARNING

This is not an end-user product. This is pure experimental.

S0ix might not be fully supported by your hardware, firmware or operating system. Enabling S0ix might overheat your device and cause fire in extreme environments. The developer is not responsible for anything that happen to you or your computer.

Loading incorrect ACPI tables might cause harm to your hardware, render your OS unable to boot, and cause data damage. Loading untrusted ACPI table might harm your data security.

## Usage

### Requirements

* UEFI-enabled firmware
* Windows 10 1903 (19H1) or later (earlier versions of Windows doesn't support IntelPEP if it is not available during OS installation, but your mileage may vary)
* S0ix is enabled (natively or use [S0ixEnabler](https://github.com/Jamesits/S0ixEnabler))

### Preparation

* [Find](tables/LPIT) or [write](https://github.com/Jamesits/DRIPSEnabler/wiki/LPIT-Table) a LPIT table that fits your hardware
* [Find](tables/SSDT) or [write](https://github.com/Jamesits/DRIPSEnabler/wiki/SSDT-Table) a SSDT table that fits your hardware

LPIT table should already be binary, name it `LPIT.bin`. SSDT is usually written in [ACPI Source Language](https://acpica.org/sites/acpica/files/asl_tutorial_v20190625.pdf), now compile it using [Intel iasl](https://github.com/acpica/acpica) or equivalent:

```shell
# assume under Debian-based Linux distro
sudo apt-get install acpica-tools
iasl -tc IntelPEP.dsl
```

and name the generated binary table file `IntelPEP.aml`.

### Installation

Put `DRIPSEnabler.efi` to a location where it will be executed before Windows starts. If you are using rEFInd, put it under `ESP:\EFI\refind\drivers_x64`. You can also load it using UEFI shell's `startup.nsh`.

Create a directory on the root of the same volume where you put `DRIPSEnabler.efi`, name it `DRIPSEnabler`. Put `LPIT.bin` and `IntelPEP.dsl` into that directory.

Now reboot.

### Verification

See [SSDT Debugging / Verification](https://github.com/Jamesits/DRIPSEnabler/wiki/SSDT-Table#debugging--verification).