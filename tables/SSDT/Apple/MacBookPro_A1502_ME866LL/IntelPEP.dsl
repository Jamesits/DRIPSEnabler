DefinitionBlock ("", "SSDT", 1, "APPLE ", "IntelPEP", 0x00001000)
{
    Scope (_SB)
    {
        Device (PEPD)
        {
            // Intel Power Engine
            Name (_HID, "INT33A1" )
            Name (_CID, EisaId ("PNP0D80"))
            Name (_UID, 1)

            // Device dependency for uPEP
            Name(DEVY, Package()
            {
                // CPUs (only non-HT cores needed)
                Package() {"\\_PR.CPU0", 0x1, Package() {Package() {0xFF, 0}}},
                Package() {"\\_PR.CPU1", 0x1, Package() {Package() {0xFF, 0}}},
                Package() {"\\_PR.CPU2", 0x1, Package() {Package() {0xFF, 0}}},
                Package() {"\\_PR.CPU3", 0x1, Package() {Package() {0xFF, 0}}},
                // iGPU
                Package() {"\\_SB.PCI0.IGPU", 0x1, Package() {Package() {0xFF,3}}},
                // XHCI
                // Package() {"\\_SB.PCI0.XHC1", 0x1, Package() {Package() {0xFF,3}}},
                // SSD
                Package() {"\\_SB.PCI0.RP06", 0x1, Package (0x02) {0x0, Package (0x03) { 0xFF, 0x00, 0x81 }}},
                // BATT, etc.
                Package() {"\\_SB.PCI0.RP05", 0x1, Package (0x02) {0x0, Package (0x03) { 0xFF, 0x00, 0x81 }}},
                // Wi-Fi
                Package() {"\\_SB.PCI0.RP03", 0x1, Package (0x02) {0x0, Package (0x03) { 0xFF, 0x00, 0x81 }}},
                // Camera
                Package() {"\\_SB.PCI0.RP02", 0x0, Package (0x02) {0x0, Package (0x03) { 0xFF, 0x00, 0x81 }}},
            })

            // BCCD crashdump information
            Name (BCCD, Package ()
            {
                Package ()
                {
                    "\\_SB.PCI0.RP06", // match your HDD 
                    Package ()
                    {
                        Package ()
                        {
                            Package ()
                            {
                                0x01, 
                                0x08, 
                                0x00, 
                                0x01, 
                                0xB2
                            }, 

                            Package (0x03)
                            {
                                0x00, 
                                0xCD, 
                                0x01
                            }, 

                            0x000186A0
                        }
                    }
                }
            })

            // Status: we ignore OSI
            Method (_STA, 0, NotSerialized)
            {
                Return (0xf)
            }

            // DSM: S0ix information (doesn't need any changes)
            Method(_DSM, 0x4, Serialized)
            {
                If ((Arg0 == ToUUID ("c4eb40a0-6cd2-11e2-bcfd-0800200c9a66")))
                {
                    // Index
                    If ((Arg2 == Zero))
                    {
                        Return (Buffer ()
                        {
                            0x7F
                        })
                    }
                    // Device constraints
                    If ((Arg2 == 0x01))
                    {
                        Return (DEVY)
                    }
                    // Crashdump
                    If ((Arg2 == 0x02))
                    {
                        Return (BCCD)
                    }
                    // Display on
                    If ((Arg2 == 0x03)) {}
                    // Display off
                    If ((Arg2 == 0x04)) {}
                    // Enter S0ix
                    If ((Arg2 == 0x05)) {}
                    // Exit S0ix
                    If ((Arg2 == 0x06)) {}
                }

                Return (Buffer (One)
                {
                    0x00
                })
            }
        }
    }
}