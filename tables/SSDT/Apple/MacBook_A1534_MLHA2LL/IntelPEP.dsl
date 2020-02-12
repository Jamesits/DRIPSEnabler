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
                // CPUs
                Package() {"\\_PR.CPU0", 0x1, Package() {Package() {0xFF, 0}}},
                Package() {"\\_PR.CPU1", 0x1, Package() {Package() {0xFF, 0}}},
                Package() {"\\_PR.CPU2", 0x1, Package() {Package() {0xFF, 0}}},
                Package() {"\\_PR.CPU3", 0x1, Package() {Package() {0xFF, 0}}},
                // iGPU
                Package() {"\\_SB.PCI0.PEG0.GFX0", 0x1, Package() {Package() {0xFF,3}}},
                Package() {"\\_SB.PCI0.IGPU", 0x1, Package() {Package() {0xFF,3}}},
                // UART
                Package() {"\\_SB.URT0", 0x1, Package() {Package() {0xFF,3}}},
                Package() {"\\_SB.URT2", 0x1, Package() {Package() {0xFF,3}}},
                // SPI
                Package() {"\\_SB.SPI0", 0x1, Package() {Package() {0xFF,3}}},
                Package() {"\\_SB.SPI1", 0x1, Package() {Package() {0xFF,3}}},
                // I2C
                Package() {"\\_SB.I2C2", 0x1, Package() {Package() {0xFF,3}}},
                // XHCI
                Package() {"\\_SB.PCI0.XHC1", 0x1, Package() {Package() {0xFF,3}}},
                // PCIe Root Bridges
                // RP01 is NVMe
                Package()
                {
                    "\\_SB.PCI0.RP01", 
                    0x1, 
                    Package (0x02)
                    {
                        0x0, 
                        Package (0x03)
                        {
                            0xFF, 
                            0x00, 
                            0x81
                        }
                    }
                },
                // RP09 is Wi-Fi
                Package()
                {
                    "\\_SB.PCI0.RP09", 
                    0x1, 
                    Package (0x02)
                    {
                        0x0, 
                        Package (0x03)
                        {
                            0xFF, 
                            0x00, 
                            0x81
                        }
                    }
                },
                // RP10 is Camera
                Package()
                {
                    "\\_SB.PCI0.RP10", 
                    0x0, 
                    Package (0x02)
                    {
                        0x0, 
                        Package (0x03)
                        {
                            0xFF, 
                            0x00, 
                            0x81
                        }
                    }
                },
            })

            // BCCD crashdump information
            Name (BCCD, Package ()
            {
                Package ()
                {
                    "\\_SB.PCI0.RP01", 
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

            // DSM: S0ix information
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