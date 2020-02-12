#include <stdbool.h>
#include <efi.h>
#include <efilib.h>
#include "effy/src/acpi_dump.h"
#include "effy/src/acpi_checksum.h"
#include "effy/src/nstdlib.h"
#include "effy/src/xsdt.h"
#include "effy/src/dirtool.h"

#define INTEL_PEP_AML_PATH L"DRIPSEnabler\\IntelPEP.aml"
#define LPIT_BIN_PATH L"DRIPSEnabler\\LPIT.bin"

// Search this drive for a file, load it into the memory and return the buffer.
CHAR8* load_file(CHAR16* path, EFI_HANDLE ImageHandle)
{
	CHAR8* buf = NULL;
	DIRTOOL_STATE DirToolState;
	DirToolState.initialized = 0;
	EFI_STATUS status = dirtool_init(&DirToolState, ImageHandle);
	if (EFI_ERROR(status)) {
		return NULL;
	}

	DIRTOOL_DRIVE* drive = dirtool_get_current_drive(&DirToolState, 0);
	dirtool_open_drive(&DirToolState, drive);
	DIRTOOL_FILE* pwd = dirtool_cd_multi(&(drive->RootFile), path);
	if (pwd)
	{
		buf = dirtool_read_file(pwd);
		Print(L"%HFile %s found, file_size=%u%N\n", path, pwd->FileInfo->FileSize);
	}
#if defined(_DEBUG)
	pause();
#endif
	dirtool_close_drive(&DirToolState, drive);
	dirtool_deinit(&DirToolState);

	return buf;
}

// Application entrypoint (must be set to 'efi_main' for gnu-efi crt0 compatibility)
EFI_STATUS efi_main(EFI_HANDLE ImageHandle, EFI_SYSTEM_TABLE* SystemTable)
{
#if defined(_GNU_EFI)
	InitializeLib(ImageHandle, SystemTable);
#endif

	Print(L"\n%HDRIPSEnabler%N\n");
	Print(L"%Hhttps://github.com/Jamesits/DRIPSEnabler%N\n");
	Print(L"Firmware %s Rev %d\n\n", SystemTable->FirmwareVendor, SystemTable->FirmwareRevision);

	// load file
	UINT8 tmp_chksum = 0;
	Print(L"%HLoading LPIT from disk, please wait...%N\n");
	EFI_ACPI_DESCRIPTION_HEADER* new_lpit_table_buf = load_file(LPIT_BIN_PATH, ImageHandle);
	if (new_lpit_table_buf == NULL) {
		Print(L"%EFile not found%N\n");
		return EFI_UNSUPPORTED;
	}
	tmp_chksum = AcpiChecksum(new_lpit_table_buf, new_lpit_table_buf->Length);
	if (tmp_chksum)
	{
		Print(L"%EChecksum error 0x%x%N\n", tmp_chksum);
		return EFI_UNSUPPORTED;
	}
	Print(L"%HLPIT loaded%N\n");

	Print(L"%HLoading SSDT from disk, please wait...%N\n");
	EFI_ACPI_DESCRIPTION_HEADER* new_ssdt_table_buf = load_file(INTEL_PEP_AML_PATH, ImageHandle);
	if (new_ssdt_table_buf == NULL) {
		Print(L"%EFile not found%N\n");
		return EFI_UNSUPPORTED;
	}
	tmp_chksum = AcpiChecksum(new_ssdt_table_buf, new_ssdt_table_buf->Length);
	if (tmp_chksum)
	{
		Print(L"%EChecksum error 0x%x%N\n", tmp_chksum);
		return EFI_UNSUPPORTED;
	}
	Print(L"%HSSDT loaded%N\n");

#if defined(_DEBUG)
	pause();
#endif

	// craft a new LPIT table
	EFI_ACPI_DESCRIPTION_HEADER* new_lpit_table = malloc_acpi(new_lpit_table_buf->Length);
	if (new_lpit_table == NULL)
	{
		Print(L"%ELPIT table memory allocation failed%N\n");
		return EFI_OUT_OF_RESOURCES;
	}
	memcpy8(new_lpit_table, new_lpit_table_buf, new_lpit_table_buf->Length);
	memcpy8(new_lpit_table->OemId, (CHAR8*)"YJSNPI", 6);
	new_lpit_table->Checksum -= AcpiChecksum(new_lpit_table, new_lpit_table->Length);
	Print(L"%ELPIT table: length=%u%N\n", new_lpit_table->Length);

	// craft a new SSDT table
	EFI_ACPI_DESCRIPTION_HEADER* new_ssdt_table = malloc_acpi(new_ssdt_table_buf->Length);
	if (new_ssdt_table == NULL)
	{
		Print(L"%ESSDT table memory allocation failed%N\n");
		return EFI_OUT_OF_RESOURCES;
	}
	memcpy8(new_ssdt_table, new_ssdt_table_buf, new_ssdt_table_buf->Length);
	memcpy8(new_ssdt_table->OemId, (CHAR8*)"YJSNPI", 6);
	new_ssdt_table->Checksum -= AcpiChecksum(new_ssdt_table, new_ssdt_table->Length);
	Print(L"%ESSDT table: length=%u%N\n", new_ssdt_table->Length);

	Print(L"%HTable is crafted%N\n");

#if defined(_DEBUG)
	pause();
#endif

	EFI_CONFIGURATION_TABLE* ect = SystemTable->ConfigurationTable;
	EFI_GUID AcpiTableGuid = ACPI_TABLE_GUID;
	EFI_GUID Acpi2TableGuid = ACPI_20_TABLE_GUID;
	EFI_ACPI_2_0_ROOT_SYSTEM_DESCRIPTION_POINTER* rsdp = NULL;
	EFI_ACPI_SDT_HEADER* Xsdt = NULL;

	UINT64 ret = EFI_SUCCESS;

	// locate RSDP (Root System Description Pointer) 
	for (UINTN SystemTableIndex = 0; SystemTableIndex < SystemTable->NumberOfTableEntries; SystemTableIndex++)
	{
		Print(L"Table #%d/%d: ", SystemTableIndex + 1, SystemTable->NumberOfTableEntries);

		if (!CompareGuid(&SystemTable->ConfigurationTable[SystemTableIndex].VendorGuid, &AcpiTableGuid) && !CompareGuid(
			&SystemTable->ConfigurationTable[SystemTableIndex].VendorGuid, &Acpi2TableGuid))
		{
			Print(L"Not ACPI\n");
			goto next_table;
		}

		if (strncmp8((unsigned char*)"RSD PTR ", (CHAR8*)ect->VendorTable, 8))
		{
			Print(L"Not RSDP\n");
			goto next_table;
		}

		rsdp = (EFI_ACPI_2_0_ROOT_SYSTEM_DESCRIPTION_POINTER*)ect->VendorTable;
		Print(L"RSDP Rev %u @0x%x | ", rsdp->Revision, rsdp);

		// check if we have XSDT
		if (rsdp->Revision < EFI_ACPI_2_0_ROOT_SYSTEM_DESCRIPTION_POINTER_REVISION)
		{
			Print(L"%ENo XSDT\n");
			rsdp = NULL;
			goto next_table;
		}

		// validate XSDT signature
		Xsdt = (EFI_ACPI_SDT_HEADER*)(rsdp->XsdtAddress);
		if (strncmp8((CHAR8*)"XSDT", Xsdt->Signature, 4))
		{
			Print(L"%EInvalid XSDT\n");
			Xsdt = NULL;
			goto next_table;
		}

		// yeah we got XSDT!
		CHAR16 OemStr[20];
		Ascii2UnicodeStr((CHAR8*)(Xsdt->OemId), OemStr, 6);
		UINT32 EntryCount = (Xsdt->Length - sizeof(EFI_ACPI_SDT_HEADER)) / sizeof(UINT64);
		Print(L"%HXSDT OEM ID: %s Tables: %d%N\n", OemStr, EntryCount);

		// break if we found the XSDT but there is no BGRT
		break;

	next_table:
		ect++;
	}

#define NEW_TABLE_COUNT 2

	if (rsdp && Xsdt) // things found, load 2 tables
	{
		// create new XSDT with 2 more entries
		XSDT* newXsdt = malloc_acpi(Xsdt->Length + NEW_TABLE_COUNT * sizeof(UINT64));
		if (newXsdt == NULL)
		{
			Print(L"%EXSDT table memory allocation failed%N\n");
			return EFI_OUT_OF_RESOURCES;
		}

		// copy over old entries
		memcpy8((CHAR8*)newXsdt, (CHAR8*)Xsdt, Xsdt->Length);

		// insert entry
		newXsdt->Header.Length += NEW_TABLE_COUNT * sizeof(UINT64);
		UINT32 EntryCount = (newXsdt->Header.Length - sizeof(EFI_ACPI_SDT_HEADER)) / sizeof(UINT64);
		newXsdt->Entry[EntryCount - 2] = (UINT64)new_lpit_table;
		newXsdt->Entry[EntryCount - 1] = (UINT64)new_ssdt_table;

		// debug mark; use rweverything (http://rweverything.com/) to look for it under Windows
		memcpy8((CHAR8*)&(newXsdt->Header.CreatorId), (CHAR8*)"YJSNPI", 6);

		// re-calculate XSDT checksum
		const CHAR8 new_xsdt_checksum_diff = AcpiChecksum((UINT8*)newXsdt, newXsdt->Header.Length);
		newXsdt->Header.Checksum -= new_xsdt_checksum_diff;

		// invalidate old XSDT table signature and checksum
		memcpy8((CHAR8*)Xsdt, (CHAR8*)"FUCK", 4);

		// replace old XSDT
		rsdp->XsdtAddress = (UINT64)newXsdt;

		// re-calculate RSDP extended checksum
		const CHAR8 new_rsdt_checksum_diff = AcpiChecksum((UINT8*)rsdp, rsdp->Length);
		rsdp->ExtendedChecksum -= new_rsdt_checksum_diff;

		Print(L"%HNew tables inserted%N\n");
	}
	else if (rsdp == NULL)
	{
		Print(L"%EERROR: RSDP is not found%N\n");
		ret = EFI_UNSUPPORTED;
	}
	else if (Xsdt == NULL)
	{
		Print(L"%EERROR: XSDT is not found%N\n");
		ret = EFI_UNSUPPORTED;
	}
	else
	{
		Print(L"%EError: Something happened%N\n");
		ret = EFI_UNSUPPORTED;
	}

#if defined(_DEBUG)
	Print(L"%EDRIPSEnabler done, press any key to continue.%N\n\n");

	pause();

	// If running in debug mode, use the EFI shut down call to close QEMU
	/*Print(L"%EResetting system%N\n\n");
	SystemTable->RuntimeServices->ResetSystem(EfiResetShutdown, EFI_SUCCESS, 0, NULL);*/
#else
	// if we are running as an EFI driver, then just quit and let other things load
	Print(L"%EDRIPSEnabler done%N\n\n");
#endif

	return ret;
}

